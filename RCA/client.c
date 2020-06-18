#include <stdio.h> 
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "ascii.h"
#include "aux.h"
#define MAXARG 3 //num maximo de argumentos do comando (o comando em si tb eh um arg)
#define DEFAULT_PORT 7777
#define BUFF_SIZE 32

typedef struct Op {
    uint8_t num;
    uint8_t argc;
    char ** argv;
} Op;

void print_help(){
    printf("Sintaxe:\n\tcomando    < parametro > ...     [ < opicional > ]\n");
    printf("Usos:\n\tconectar   < nome do servidor >  [ < porta > ]\n");
    printf("\tlistar     \n");
    printf("\treceber    < arquivo remoto >    [ < arquivo local > ]\n");
    printf("\tenviar     < arquivo local >     [ < arquivo remoto > ]\n");
    printf("\tencerrar   \n\n");
}

Op * input(){ //pega a input e retorna uma Op struct
    Op * ret = (Op *) malloc(sizeof(Op));
    char in[256], * tok_hndl, * token[MAXARG];
    short int op=-1;
    uint8_t count, i;

    while(op<0){
        printf("cliente_psta > ");
        fgets(in, 256, stdin);
        //in[strlen(in)-1]='\0';//NAO ACEITA \n
        memset(in+strlen(in)-1,'\0',256-strlen(in));
        if (strlen(in) > 0){
            tok_hndl = strtok(in," ");
            for(i=0;tok_hndl[i];i++) tok_hndl[i]=tolower(tok_hndl[i]);//tolower
            if(strcmp(tok_hndl,"conectar")==0 && in[9]!='\0')
                op=0;
            else if(strcmp(tok_hndl,"listar")==0)
                op=1;
            else if(strcmp(tok_hndl,"receber")==0)
                op=2;
            else if(strcmp(tok_hndl,"enviar")==0)
                op=3;
            else if(strcmp(tok_hndl,"encerrar")==0)
                op=4;
            else {
                printf("Unknown input, try again..\n\n");
                print_help();
            }
        }
    }
    ret->num = (uint8_t)op;
    count=0;
    while(tok_hndl && strcmp(tok_hndl,"")!=0 && count < MAXARG){
        token[count]=tok_hndl;
        count++;
        tok_hndl = strtok(NULL, " ");//percorro todos os tokens
    }
    ret->argc = count;
    //separo as inputs em argc e argv
    char ** argv = (char **)malloc(sizeof(char *)*count);
    for(i=0;i<count;i++){
        argv[i] = (char *) malloc(sizeof(char)*(strlen(token[i])+1));
        strcpy(argv[i],token[i]);
    }
    ret->argv = argv;
    return ret;
}

void libera(Op * op){
    int i;
    for(i=0;i<op->argc;i++)
        free(op->argv[i]);
    free(op->argv);
    free(op);
}

//funcoes do cliente
int conectar(struct hostent * hostnm, short int port, int * s, struct sockaddr_in * sck);
int listar(int * ctl, int * s, int * ns, int * namelen,
            struct sockaddr_in * addr_ns, int op, unsigned short port);
int receber(char * f, char * fsave, int * ctl, int * s, int * ns, int * namelen,
            struct sockaddr_in * addr_ns, int op, unsigned short port);
int enviar(char * f, char *fsave, int * ctl, int * s, int * ns, int * namelen,
            struct sockaddr_in * addr_ns, int op, unsigned short port);
//- - - - - - - - - - - - - - - - -
unsigned short bind_listen(struct hostent * hostnm, int * s,
                           struct sockaddr_in * addr_s, int * namelen);

void send_ctl(int * ctl, int op, unsigned short port);

//----------------------------------------------------------------------------
int validaIp(char * str);
//----------------------------------------------------------------------------
int main(int argc, char ** argv){
    int ctl, s, ns;   //SOCKETS IDS
    struct sockaddr_in addr_ctl, addr_s, addr_ns;   //ADDR
    struct hostent * hostnm_ctl;
    int namelen;
    Op * op;
    int status = 0;
    unsigned short port = 0;

    while(1){
        op = input();
        switch(op->num){
            case 0: //conectar-se
                if(status<1){
                    if(validaIp(op->argv[1])==0){
                        fprintf(stderr, "O endereco inserido nao pode ser resolvido, tente novamente.\n");
                        break;
                    }
                    hostnm_ctl = gethostbyname(op->argv[1]);
                    if (hostnm_ctl == (struct hostent *) 0 ){
                        fprintf(stderr, "O endereco inserido nao pode ser resolvido, tente novamente.\n");
                        break;
                    }
                    printf("Tentando conexao com %s, por favor aguarde...\n",op->argv[1]);
                    if(op->argc == 2)
                        status = conectar(hostnm_ctl, DEFAULT_PORT, &ctl,&addr_ctl);
                    else if(op->argc == 3)
                        status = conectar(hostnm_ctl, (unsigned short)atoi(op->argv[2]), &ctl,&addr_ctl);
                    if(status>=0)
                        port = bind_listen(hostnm_ctl, &s, &addr_s, &namelen);
                }
                else
                    printf("Você ja esta conectado, use \"encerrar\" para finalizar a conexao.\n");
                break;

            case 1: //listar dir
                if(status<1){
                    printf("Conecte-se a um servidor antes de realizar tal operacao.\n");
                    break;
                }
                listar(&ctl,&s,&ns,&namelen,&addr_ns,op->num,port);
                break;

            case 2: //download, receber
                if(status<1){
                    printf("Conecte-se a um servidor antes de realizar tal operacao.\n");
                    break;
                }
                if(op->argc == 2)
                    receber(op->argv[1],NULL,&ctl,&s,&ns,&namelen,&addr_ns,op->num,port);
                else if(op->argc == 3)
                    receber(op->argv[1],op->argv[2],&ctl,&s,&ns,&namelen,&addr_ns,op->num,port);
                else
                    printf("Insira o nome do arquivo desejado!\n");
                break;

            case 3: //upload, enviar
                if(status<1){
                    printf("Conecte-se a um servidor antes de realizar tal operacao.\n");
                    break;
                }
                if(op->argc == 2)
                    enviar(op->argv[1],NULL,&ctl,&s,&ns,&namelen,&addr_ns,op->num,port);
                else if(op->argc == 3)
                    enviar(op->argv[1],op->argv[2],&ctl,&s,&ns,&namelen,&addr_ns,op->num,port);
                else
                    printf("Insira o nome do arquivo desejado!\n");
                break;

            case 4: //exit
                if(status==1){
                    send_ctl(&ctl,4,port);
                    close(s);
                    printf("Voce foi desconectado\n");
                    status=-1;
                }
                else if(status==-1)
                    status=0;
                break;

        }
        if(op->num==4 && status==0){
            libera(op);
            exit(1);
        }
        libera(op);
    }
    return 0;
}
//----------------------------------------------------------------------------
int accept_connection(int * s, int * ns, int * namelen, struct sockaddr_in * addr_ns);
//----------------------------------------------------------------------------
int conectar(struct hostent * hostnm, short int port, int * s, struct sockaddr_in * sck){
    sck->sin_family      = AF_INET;
    sck->sin_port        = htons(port);
    sck->sin_addr.s_addr = *((unsigned long *)hostnm->h_addr);

    if ((*s = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        perror("Socket()");
        return -1;
    }

    if (connect(*s, (struct sockaddr *)sck, sizeof(*sck)) < 0){
        perror("Connect()");
        return -2;
    }

    return 1;
}

int listar(int * ctl, int * s, int * ns, int * namelen,
            struct sockaddr_in * addr_ns, int op, unsigned short port){
    //enviar comando de CTL + PORTA
    send_ctl(ctl,op,port);
    //esperar OK
    char RCV[2]={'\0','\0'};
    if (recv(*ctl, RCV, 1, 0) == -1){
        perror("Recv()");
        return -1;
    }
    if(RCV[0]!='+'){
        printf("Ocorreu um erro, o servidor nao conseguiu conectar-se a voce.\n");
        return -1;
    }
    accept_connection(s, ns, namelen, addr_ns); //espera bloqueante, aceita conexao de dados
    char * buff, pbuff[256];
    unsigned short tam=0;
    while((buff=recv_datablock(ns,&tam)) != NULL){
        strncpy(pbuff,buff,tam);
        pbuff[tam]='\0';
        printf("%s",pbuff);
        free(buff);
    }
    printf("\n");
    close(*ns);
}

int receber(char * f, char * fsave, int * ctl, int * s, int * ns, int * namelen,
            struct sockaddr_in * addr_ns, int op, unsigned short port){
    
    if(strlen(f)>(4096+255)){
        printf("O nome do arquivo eh muito grande.\n");
        return -1;
    }
    //enviar comando de CTL + PORTA
    send_ctl(ctl,op,port);
    //esperar OK
    char RCV[2]={'\0','\0'};
    if (recv(*ctl, RCV, 1, 0) == -1){
        perror("Recv()");
        return -1;
    }
    if(RCV[0]!='+'){
        printf("Ocorreu um erro, o servidor nao conseguiu conectar-se a voce.\n");
        return -1;
    }
    accept_connection(s, ns, namelen, addr_ns); //espera bloqueante, aceita conexao de dados
    send_data(ns,f,strlen(f));
    send_data_end(ns);
    //espero OK para arquivo validO!!!
    memset(RCV,'\0',2);
    if (recv(*ctl, RCV, 1, 0) == -1){
        perror("Recv()");
        return -1;
    }
    if(RCV[0]!='+'){
        printf("Arquivo \"%s\" nao encontrado.\n",f);
        return -1;
    }
    //TODO: Resolver casos que o caminho ao arquivo nao eh simples, criando pastas quando necessario!
    // uma solucao= talvez usar o strtok!!
    FILE * fd;
    if(!fsave)
        fd = fopen(f,"wb");
    else
        fd = fopen(fsave,"wb");
    if(!fd){
        printf("LOCAL FILE ACESS PROBLEM\n");
        perror("fopen()");
        return -1;
    }
    char * buff;
    unsigned short tam=0;
    while((buff=recv_datablock(ns,&tam)) != NULL){
        fwrite(buff, 1, tam, fd);
        free(buff);
    }
    fclose(fd);
    close(*ns);
}

int enviar(char * f, char * fsave, int * ctl, int * s, int * ns, int * namelen,
            struct sockaddr_in * addr_ns, int op, unsigned short port){
    
    if(strlen(f)>(4096+255)){
        printf("O nome do arquivo eh muito grande.\n");
        return -1;
    }

    FILE * local;
    local = fopen(f,"rb");
    if(!local){
        printf("Arquivo \"%s\" nao encontrado.",f);
        return -1;
    }//checa se arquivo existe
    //enviar comando de CTL + PORTA
    send_ctl(ctl,op,port);
    //esperar OK
    char RCV[2]={'\0','\0'};
    if (recv(*ctl, RCV, 1, 0) == -1){
        perror("Recv()");
        return -1;
    }
    if(RCV[0]!='+'){
        printf("Ocorreu um erro, o servidor nao conseguiu conectar-se a voce.\n");
        return -1;
    }
    accept_connection(s, ns, namelen, addr_ns); //espera bloqueante, aceita conexao de dados
    printf("CONEXAO FOI ACEITA!\n");
    if(!fsave)
        send_data(ns,f,strlen(f));
    else
        send_data(ns,fsave,strlen(fsave));
    send_data_end(ns);
    
    //espero OK para arquivo validO!!!
    memset(RCV,'\0',2);
    if (recv(*ctl, RCV, 1, 0) == -1){
        perror("Recv()");
        return -1;
    }
    if(RCV[0]!='+'){
        if(!fsave)
            printf("Escrita no arquivo \"%s\" remoto negada.\n",f);
        else
            printf("Escrita no arquivo \"%s\" remoto negada.\n",fsave);
        return -1;
    }
    //ARQUIVO ENCONTRADO, FAZER UPLOAD JA!

    char buff[255];
    unsigned int tam=0;
    do{
        memset(buff,'\0',255);
        tam = fread(buff,sizeof(unsigned char),255,local);
        send_data(ns, buff, tam);
    }while(!feof(local));
    send_data_end(ns);
    fclose(local);
    close(*ns);
}




//- - - - - - - - - - - - - - - - -

unsigned short bind_listen(struct hostent * hostnm, int * s,
                           struct sockaddr_in * addr_s, int * namelen){
    if ((*s = socket(AF_INET, SOCK_STREAM, 0)) < 0){
       perror("socket()");
       return -1;
    }
    addr_s->sin_family      = AF_INET;
    addr_s->sin_port        = 0;
    addr_s->sin_addr.s_addr = *((unsigned long *)hostnm->h_addr);
    if (bind(*s, (struct sockaddr *)addr_s, sizeof(*addr_s)) < 0){
       perror("bind()");
       return -2;
    }
    * namelen = sizeof(*addr_s);
    if (getsockname(*s, (struct sockaddr *) addr_s, (socklen_t *) namelen) < 0){
       perror("getsockname()");
       return -3;
    }
    if (listen(*s, 1) != 0){
        perror("Listen()");
        return -4;
    }
    printf("CONECTADO - LISTENING ON PORT %d\n",ntohs(addr_s->sin_port));
    return ntohs(addr_s->sin_port);
}

void send_ctl(int * ctl, int op, unsigned short port){
    char * p = ushort2ASCII(port);//tam=4+NULL
    char sendbuff[6];
    sprintf(sendbuff,"%c%s",0x30+op,p);
    if (send(*ctl, sendbuff, 6, 0) < 0){
        perror("Send()");
        exit(5);
    }
    free(p);
}

int accept_connection(int * s, int * ns, int * namelen, struct sockaddr_in * addr_ns){
    if ((*ns = accept(*s, (struct sockaddr *)addr_ns, (socklen_t *)namelen)) == -1){
        perror("Accept()");
        return 0;
    }
    return 1;
}

int validaIp(char * str){
    if(!str)
        return 0;
    if(!isdigit(str[0]))
        return 1;// PODE SER UM HOSTNAME VALIDO, DEIXA O GETBYHOSTNAME VERIFICAR!
    if(!strchr(str,'.'))
        return 0;
    char * tok, * tmp = (char *) malloc(sizeof(char)*strlen(str));
    strcpy(tmp,str);
    if((tok = strtok(tmp,".")) == NULL){
        free(tmp);
        return 0;
    }
    int val=atoi(tok);
    if(val<0 || val > 255){
        free(tmp);
        return 0;
    }
    int i;
    for(i=0;i<3;i++){
        if((tok = strtok(NULL,".")) == NULL){
            free(tmp);
            return 0;
        }
        val=atoi(tok);
        if(val<0 || val > 255){
            free(tmp);
            return 0;
        }
    }
    free(tmp);
    return 1;
}