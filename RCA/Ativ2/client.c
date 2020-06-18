#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define USR_SIZE 19
#define MSG_SIZE 79
#define BUFF_LEN 128

void read_input(char * str, int size){
    char * tmp = (char *)malloc(sizeof(char)*(size+2));
    fgets(tmp, size+2, stdin);
    tmp[strlen(tmp)-1]='\0';
    strcpy(str,tmp);
    free(tmp);
    //falta exception fgets leu mais do que dava....!!!
}

typedef struct msg{
    char usuario[USR_SIZE];
    char mensagem[MSG_SIZE];
} msg;

char BUFF[BUFF_LEN];
char NET_BUFF[BUFF_LEN];

void print_menu(){
    printf("Opcoes:\n");
    printf("1 - Cadastrar mensagem\n");
    printf("2 - Ler mensagens\n");
    printf("3 - Apagar mensagens\n");
    printf("4 - Sair da Aplicacao\n");
}

void print_msgs(msg * M, int n){
    int i;
    for(i=0;i<n;i++){
        printf("Usuario: %s\t\tMensagem: %s\n",M[i].usuario,M[i].mensagem);
    }
}

void op1_cadastro(int * s, msg * MSG){
    //envia request de op1
    strcpy(NET_BUFF,"op1");
    if (send(*s, NET_BUFF, 3, 0) < 0){
        perror("Send()");
        exit(5);
    }
    //receebe response se qntd de msgs < 10 OK
    memset(NET_BUFF,'\0',BUFF_LEN);
    if (recv(*s, NET_BUFF, 2, 0) == -1){
        perror("Recv()");
        exit(6);
    }

    if(strcmp(NET_BUFF,"OK")!=0){
        printf("Impossivel cadastrar novas mensagens, servidor cheio.\n");
        return;
    }

    printf("Insira os dados de cadastro:\n");
    printf("Usuario: ");
    read_input(MSG->usuario,USR_SIZE);

    printf("Mensagem: ");
    read_input(MSG->mensagem,MSG_SIZE);
    
    if (send(*s, MSG, sizeof(msg), 0) < 0){
        perror("Send()");
        exit(5);
    }
}

void op2_leitura(int * s/*, msg * MSG*/){
    //envia request de op2
    strcpy(NET_BUFF,"op2");
    if (send(*s, NET_BUFF, 3, 0) < 0){
        perror("Send()");
        exit(5);
    }
    //request OK
    memset(NET_BUFF,'\0',BUFF_LEN);
    if (recv(*s, NET_BUFF, 2, 0) == -1){
        perror("Recv()");
        exit(6);
    }
    if(strcmp(NET_BUFF,"OK")==0){
        //request QTD
        int qtd;
        if (recv(*s, &qtd, sizeof(int), 0) == -1){
            perror("Recv()");
            exit(6);
        }
        if(qtd>0){
            //request cada msg p/ quantidade
            msg * M = (msg *) malloc(sizeof(msg)* qtd);
            int i;
            for(i=0;i<qtd;i++){
                if (recv(*s, &M[i], sizeof(msg), 0) == -1){
                    perror("Recv()");
                    exit(6);
                }
            }
            //aprestacao dos resultados
            printf("Mensagens cadastradas: %d\n",qtd);
            print_msgs(M,qtd);
            free(M);
        }
        else
            printf("Nao existem mensagens cadastradas no servidor\n");
    }
}

void op3_apaga(int * s/*, msg * MSG*/){
    //envia de op3
    strcpy(NET_BUFF,"op3");
    if (send(*s, NET_BUFF, 3, 0) < 0){
        perror("Send()");
        exit(5);
    }
    //input usuario
    printf("Insira o nome de usuario para o qual as mensagens serao apagadas\n");
    printf("Usuario: ");
    read_input(BUFF,USR_SIZE);
    //envia usuario
    if (send(*s, BUFF, USR_SIZE, 0) < 0){
        perror("Send()");
        exit(5);
    }
    //recebe count
    int count;
    if (recv(*s, &count, sizeof(int), 0) == -1){
        perror("Recv()");
        exit(6);
    }
    if(count>0){
        //request cada msg p/ quantidade
        msg * M = (msg *) malloc(sizeof(msg)* count);
        int i;
        for(i=0;i<count;i++){
            if (recv(*s, &M[i], sizeof(msg), 0) == -1){
                perror("Recv()");
                exit(6);
            }
        }
        //aprestacao dos resultados
        printf("Mensagens apagadas: %d\n",count);
        print_msgs(M,count);
        free(M);
    }
    else{
        printf("Nao foram encontradas mensagens para o usuario \"%s\" no servidor..\n",BUFF);
    }
    //se count>0 
    //recebe todas as msgs apagadas
    //printa listagem 
}

int main(int argc, char **argv)
{
    unsigned short port;
    char sendbuf[12];
    char recvbuf[12];
    struct hostent *hostnm;
    struct sockaddr_in server;
    int s;

    //argv[1] = hostname (aceita localhost)
    //argv[2] = porta
    if (argc != 3){
        fprintf(stderr, "Use: %s hostname porta\n", argv[0]);
        exit(1);
    }

    hostnm = gethostbyname(argv[1]);
    if (hostnm == (struct hostent *) 0){
        fprintf(stderr, "Gethostbyname failed\n");
        exit(2);
    }
    port = (unsigned short) atoi(argv[2]);

    server.sin_family      = AF_INET;
    server.sin_port        = htons(port);
    server.sin_addr.s_addr = *((unsigned long *)hostnm->h_addr);
    //TCP = SOCK_STREAM
    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        perror("Socket()");
        exit(3);
    }
    //Conecta ao servidor
    if (connect(s, (struct sockaddr *)&server, sizeof(server)) < 0){
        perror("Connect()");
        exit(4);
    }

    
    int op;
    msg MSG;
    while(1){
        print_menu();
        memset(BUFF,'\0',BUFF_LEN);
        read_input(BUFF,1);
        op = atoi(BUFF);
        switch(op){
            case 1:
                op1_cadastro(&s,&MSG);
                break;
            case 2:
                op2_leitura(&s);
                break;
            case 3:
                op3_apaga(&s);
                break;
            case 4:
                close(s);
                printf("Cliente terminou com sucesso.\n");
                exit(0);
                break;
        }
    }
}


