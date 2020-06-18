#include <stdio.h> 
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "lib/regs.h"
#include "lib/ui.h" //contem contatos
#include "lib/messenger.h" //biblioteca comunicacao rede
#include <pthread.h>
#include <signal.h>

pthread_t pthread_0, pthread_n;
void * thread_f0(void * arg);
void * thread_fn(void * arg);
typedef struct thread0_arg{
    int s;
    struct sockaddr * ns_addr;
}thread0_arg;

char lines[19][77]; //VARIAVEL GLOBAL
Contatos * C;       //VARIAVEL GLOBAL

pthread_mutex_t mutex_contatos;
pthread_mutex_t mutex_lines;



void interface(Contatos * C, int * central, char * user_telefone);
/***funcoes auxiliares***********************************************************************/
void read_input(char * str, int size);
bool numeric_string(char * str);
//--------------------------------------------------------------------------------------------
int main(int argc, char ** argv){
    if (pthread_mutex_init(&mutex_contatos, NULL) != 0) { 
        printf("\n mutex init has failed (contatos)\n"); 
        return 1; 
    } 
    if (pthread_mutex_init(&mutex_lines, NULL) != 0) { 
        printf("\n mutex init has failed (lines)\n"); 
        return 1; 
    } 
    int i;
    
    if (argc != 3){
        fprintf(stderr, "Use: %s ip porta\n", argv[0]);
        exit(1);
    }
    char user_telefone[13];
    do{
        printf("Por favor, insira seu numero de telefone:\n");
        read_input(user_telefone, 32);
    }while(!numeric_string(user_telefone));

    struct hostent * hostnm = gethostbyname(argv[1]);
    if (hostnm == (struct hostent *) 0){
        fprintf(stderr, "Gethostbyname failed\n");
        exit(2);
    }
    
    struct sockaddr_in central_addr, s_addr, ns_addr;
    int                central_sock, s,      ns;
    unsigned short serv_port = (unsigned short) atoi(argv[2]);

    central_addr.sin_family      = AF_INET;
    central_addr.sin_port        = htons(serv_port);
    central_addr.sin_addr.s_addr = *((unsigned long *)hostnm->h_addr);
  
    
    if ((central_sock = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        perror("Socket()");
        exit(3);
    }

    //Conecta ao servidor
    if (connect(central_sock, (struct sockaddr *)&central_addr, sizeof(central_addr)) < 0){
        perror("Connect()");
        exit(4);
    }

    //Prepara socket de servidor para escutar (receber) por mensagens
    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        perror("Socket()");
        exit(2);
    }

    s_addr.sin_family = AF_INET;
    s_addr.sin_port   = 0;
    s_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(s, (struct sockaddr *)&s_addr, sizeof(s_addr)) < 0){
       perror("Bind()");
       exit(2);
    }

    int namelen = sizeof(s_addr);
    if (getsockname(s, (struct sockaddr *) &s_addr, (socklen_t *) &namelen) < 0){
        perror("getsockname()");
        exit(1);
    }

    printf("Listening to port %d\n",ntohs(s_addr.sin_port));
    
    if (listen(s, 1) != 0){
        perror("Listen()");
        exit(4);
    }
    //THREAD CALLS PARA OUVIR POR MENSAGENS
    thread0_arg * arg = (thread0_arg *) malloc(sizeof(thread0_arg));
    arg->s = s;
    arg->ns_addr = (struct sockaddr *)&ns_addr;
    //arg->namelen = sizeof(*s_addr);

    if (pthread_create(&pthread_0, NULL, thread_f0, (void *)arg) != 0){
        perror("pthread_create");
        free(arg);
        exit(0);
    }

    reg tmp;
    memset(tmp.telefone,'\0',13);
    strcpy(tmp.telefone,user_telefone);
    tmp.ip = 0; //NAO COLOCO IP POIS O SERVER CONSEGUE VER O MEU IP, QUE ENVIEI A MSG
    tmp.port = s_addr.sin_port;
    //Envio Meu Numero Para o Servidor
    if (send(central_sock, &tmp, sizeof(reg), 0) < 0){
        perror("Send()");
        exit(5);
    }
    //ESPERO RESPOSTA DE OK
    bool ok_bool = 0;
    if (recv(central_sock, &ok_bool, sizeof(bool), 0) == -1){
        perror("Recv()");
        exit(6);
    }
    if(ok_bool==0){
        printf("Cadastro de Registro no Servidor - INVALIDO\n");
        return 0;
    }
    
    ////////////////////////////////////////////////
    C = InicializaContatos();
    FileLoadContatos(C);
    interface(C,&central_sock,user_telefone);

    LiberaContatos(C);
    pthread_mutex_destroy(&mutex_contatos); 
    pthread_mutex_destroy(&mutex_lines); 
    if ((pthread_kill(pthread_0, SIGTERM)) != 0) {
        /* Handle Error */
    }
    return 0;
}
//--------------------------------------------------------------------------------------------

void read_input(char * str, int size){
    char * tmp = (char *)malloc(sizeof(char)*(size+2));
    fgets(tmp, size+2, stdin);
    int tam = strlen(tmp);
    if(tam>size){
        free(tmp);
        printf("Input invalida, maximo permitido: %d caracteres.\n\n",size);
        return;
    }
    tmp[tam-1]='\0';
    strcpy(str,tmp);
    free(tmp);
    //falta exception fgets leu mais do que dava....!!!
}

bool numeric_string(char * str){
    int i;
    for(i=0;i<strlen(str);i++){
        if(str[i] < 0x30 && str[i] != 0x00 || str[i] > 0x39)
            return 0;
    }
    return 1;
}
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//INTERFACE
void interface(Contatos * C, int * central, char * user_telefone){ 
    //funcao principal que chama todas e fica dentro de um while,
    initscr();
    clear();
    noecho();
    cbreak();
    curs_set(0);
    
    WINDOW * menu_frame = newwin(4,20,1,1);
    WINDOW * menu_frame2 = newwin(4,20,1,16);
    WINDOW * popbox_frame = newwin(5,38,9,21);
    WINDOW * poptxt_frame = newwin(10,50,6,15);
    WINDOW * popmenu_frame = newwin(18,50,4,15);
    WINDOW * main_frame = newwin(21,79,1,1);

    char *options_f1[] = { 
			"Novo Contato",
			"Apagar Contato"
		  };
    char *options_f2[] = { 
			"Enviar Texto",
			"Enviar Imagem"
		  };

    FILE * fp;
    char * fname=NULL;
    reg * aux;
    Regs * REGS;
    char ** numeros;
    char ** options;
    char tmp_buff[256];
    unsigned char snd_buff[256];
    int c, x, tmp, i, sz = 0;
    menu_options * multi = NULL;
    contato * tmp_contato;
    bool exit_flag = 0;
    keypad(stdscr, TRUE);
    do{
        clear();
        refresh();
        mvprintw(0, 0, "[F1] Contatos - [F2] Enviar Mensagem - [F3] Sair");
        mvprintw(22, 0,  "Dicas:");
        mvprintw(23, 7, "Confirmar [ENTER], Selecao Multipla* [ESPACO],  Navegação [CIMA/BAIXO].");

        timeout(240);
        do{
            draw_messenger(main_frame, lines);
            c=getch();
        }while(c==ERR);
        timeout(-1);

        switch(c){
            case KEY_F1:
                x=draw_menu(menu_frame, "F1", options_f1, 2, KEY_F1);
                if(x==-1)
                    break;
                else if(x==0){//F1->Novo Contato   : 2x draw_inputbox
                    tmp_contato = draw_novocontato(popbox_frame);///PROBLEMINHA AQUI DE INPUT!!!!!!!
                    if(tmp_contato){
                        pthread_mutex_lock(&mutex_contatos);
                        InsereContato(C,tmp_contato->nome, tmp_contato->telefone);
                        pthread_mutex_unlock(&mutex_contatos);
                        FileNew_Contato(tmp_contato->nome, tmp_contato->telefone);
                        free(tmp_contato);
                    }
                    else
                        draw_msgbox(popbox_frame, "Dados INVALIDOS", "Os dados inseridos nao sao validos");
                }
                else if(x==1){//F1->Apagar Contato : draw_menu -> draw_msgbox
                    pthread_mutex_lock(&mutex_contatos);
                    options = getContatosOptions(C);
                    pthread_mutex_unlock(&mutex_contatos);
                    tmp = draw_menu(popmenu_frame, "Apagar Contato", options, C->total, KEY_F1);
                    free(options);
                    if(tmp>=0){
                        pthread_mutex_lock(&mutex_contatos);
                        tmp_contato=getContatoPos(C,tmp);
                        pthread_mutex_unlock(&mutex_contatos);
                        if(tmp_contato){
                            memset(tmp_buff,'\0',256);
                            sprintf(tmp_buff,"Tem certeza que deseja excluir      \"%s\" (%s)?",
                            tmp_contato->nome, tmp_contato->telefone);
                            tmp = draw_msgbox(popbox_frame, "Tem certeza?", tmp_buff);
                            if(tmp==1){
                                pthread_mutex_lock(&mutex_contatos);
                                RemoveContato(C,tmp_contato->telefone);
                                pthread_mutex_unlock(&mutex_contatos);
                                FileRmv_Contato(tmp_contato->nome);
                            }
                        }
                    }
                }
                break;
            //---------------------------------------------//
            case KEY_F2:
                x=draw_menu(menu_frame2, "F2", options_f2, 2, KEY_F2);
                if(x==-1)
                    break;
                else{
                    pthread_mutex_lock(&mutex_contatos);
                    options = getContatosOptions(C);
                    multi = draw_menu_multi(popmenu_frame, "Enviar Mensagem Para", options, C->total, KEY_F2);
                    pthread_mutex_unlock(&mutex_contatos);
                    free(options);
                    if(multi){
                        
                        if(multi->total >0){
                            memset(snd_buff,'\0',256);
                            if(x==0){     //TXT ********************************88
                                if(multi->total == 1){
                                    pthread_mutex_lock(&mutex_contatos);
                                    tmp_contato=getContatoPos(C,multi->selected_id[0]);
                                    pthread_mutex_unlock(&mutex_contatos);
                                    draw_send_txt(poptxt_frame,tmp_contato->nome,snd_buff);
                                }
                                else
                                    draw_send_txt(poptxt_frame,"Grupo",snd_buff);
                                sz=strlen(snd_buff);
                            }
                            else if(x==1){//IMG ********************************88
                                fname = NULL;
                                if(multi->total == 1){
                                    pthread_mutex_lock(&mutex_contatos);
                                    tmp_contato=getContatoPos(C,multi->selected_id[0]);
                                    pthread_mutex_unlock(&mutex_contatos);
                                    fp=draw_send_img(popmenu_frame, tmp_contato->nome, KEY_F2, &fname);
                                }
                                else
                                    fp=draw_send_img(popmenu_frame, "Grupo", KEY_F2, &fname);
                                draw_msgbox(popbox_frame, "DRAW_SEND", fname);
                            }
                            else
                                break;//??? precisa ??

                            numeros = (char ** ) malloc(sizeof(char *)*multi->total);
                            for(i=0;i<multi->total;i++){
                                pthread_mutex_lock(&mutex_contatos);
                                tmp_contato=getContatoPos(C,multi->selected_id[i]);
                                pthread_mutex_unlock(&mutex_contatos);
                                numeros[i] = (char * ) malloc(sizeof(char)*13);
                                memset(numeros[i],'\0',13);
                                strcpy(numeros[i],tmp_contato->telefone);
                            }
                            REGS = inicializa_reg();
                            tmp=consulta_central(central, REGS, numeros, multi->total);
                            if(tmp!=multi->total){
                                sprintf(tmp_buff,"%d de %d destinatario(s) online.",tmp,multi->total);
                                draw_msgbox(popbox_frame, "AVISO!", tmp_buff);
                            }                           
                            for(i=0;i<multi->total;i++)
                                free(numeros[i]);
                            free(numeros);
                            //ENVIA MENSAGEM DE FATO
                            for(aux=REGS->R;aux;aux=aux->prox){
                                if(x==0){//TXT
                                    envia_msg(user_telefone, aux, snd_buff, sz, (bool)x, NULL);
                                    pthread_mutex_lock(&mutex_contatos);
                                    pthread_mutex_lock(&mutex_lines);
                                    print_enviada(aux->telefone, C, lines, snd_buff);
                                    pthread_mutex_unlock(&mutex_lines);
                                    pthread_mutex_unlock(&mutex_contatos);
                                }
                                else{//IMG
                                    envia_msg(user_telefone, aux, fp, 0, (bool)x, fname);
                                    pthread_mutex_lock(&mutex_contatos);
                                    pthread_mutex_lock(&mutex_lines);
                                    print_enviada(aux->telefone, C, lines, fname);
                                    pthread_mutex_unlock(&mutex_lines);
                                    pthread_mutex_unlock(&mutex_contatos);
                                }    
                                    
                            }
                            if(x==1 && fname)
                                free(fname);
                            libera_reg(REGS);
                        }
                        free(multi);
                    }
                }
                break;
            //---------------------------------------------//
            case KEY_F3: //F3: draw_msgbox
                if(draw_msgbox(popbox_frame, "Atencao", "Tem certeza que deseja Sair?")==1)
                    exit_flag=TRUE;
                exit_central(central);
                break;
        }

    }while(!exit_flag);
    clrtoeol();
	refresh();
	endwin();
    LiberaContatos(C);
}
//////////////////////////////////////////////////////////////////////////////////////////////
void * thread_f0(void * arg){
    //thread ZERO, chama as outras threads
    thread0_arg * param = (thread0_arg *) arg;
    int namelen;
    while(1){
        namelen = sizeof(  (*(param->ns_addr))  );
        int * ns = (int *) malloc(sizeof(int));
        if ((*ns = accept(param->s, param->ns_addr, (socklen_t *)&namelen)) == -1){
            perror("Accept()");
            exit(5);
        }
        if (pthread_create(&pthread_n, NULL, thread_fn, (void *)ns) != 0){
            perror("pthread_create");
            free(ns);
            continue;
        }
    }
    free(param);
}

void * thread_fn(void * arg){
    int * ns = (int *) arg;
    byte * rcv_buff;
    int size; 
    char sender_tel[13];
    rcv_buff=recebe_msg(ns,&size,sender_tel);
    //MUTEX!!!!
    pthread_mutex_lock(&mutex_contatos);
    pthread_mutex_lock(&mutex_lines);
    print_recebida(sender_tel, C, lines, rcv_buff);
    pthread_mutex_unlock(&mutex_lines);
    pthread_mutex_unlock(&mutex_contatos);
    close(ns);
    free(ns);
}