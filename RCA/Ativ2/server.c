#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#define USR_SIZE 19
#define MSG_SIZE 79
#define MSG_NUM 10 //max 99 devido a qntd de bytes enviados
#define BUFF_LEN 128

typedef struct msg{
    char usuario[USR_SIZE];
    char mensagem[MSG_SIZE];
} msg;

void log_req(char * request, char * ip, int port){
    fprintf(stdout,"(%s:%d > SERVER) : %s\n",ip,port,request);
}//after recieve

void log_resp(char * response, char * ip, int port){
    fprintf(stdout,"(SERVER > %s:%d) : %s\n",ip,port,response);
}//after send

//GLOBAL VARIAVEIS
msg MSGS[MSG_NUM];
int QTD; //quantidade de msgs cadastradas

int find_empty_msg(){
    if(QTD<MSG_NUM){
        int i;
        for(i=0;i<MSG_NUM;i++){
            if(MSGS[i].usuario[0] == '\1')
                return i;
        }
    }
    return -1;
}

int count_user_msgs(char * user){
    int i, ret=0;
    for(i=0;i<MSG_NUM;i++){
        if(strcmp(MSGS[i].usuario,user)==0)
            ret++;
    }
    return ret;
}

// - - - - - - -
void cadastro(int * s, char * ip, int port);
void leitura(int * s,char * ip, int port);
void apaga(int * s, char * ip, int port);
// - - - - - - -

int main(int argc, char **argv){
    int i;
    for(i=0;i<MSG_NUM;i++)
        memset(MSGS[i].usuario,'\1',USR_SIZE);
    QTD = 0;
    char RCV_BUFF[BUFF_LEN];

    unsigned short port;
    struct sockaddr_in client, server;
    int s, ns;  //s = aceitar conexoes, ns = conexao com o cliente
    int namelen;

    if (argc != 2){
        fprintf(stderr, "Use: %s porta\n", argv[0]);
        exit(1);
    }

    port = (unsigned short) atoi(argv[1]);

    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        perror("Socket()");
        exit(2);
    }

    server.sin_family = AF_INET;
    server.sin_port   = htons(port);
    server.sin_addr.s_addr = INADDR_ANY; //faz com que o server ligue em todos end. IP

    // Bind do servidor na porta definida anteriormente
    if (bind(s, (struct sockaddr *)&server, sizeof(server)) < 0){
       perror("Bind()");
       exit(3);
    }

    //Prepara socket c/ fila de conexoes pendentes
    if (listen(s, 1) != 0){
        perror("Listen()");
        exit(4);
    }

    char *client_ip;
    int client_port, op, status;

    while(1){
        namelen = sizeof(client);
        if ((ns = accept(s, (struct sockaddr *)&client, (socklen_t *)&namelen)) == -1){
            perror("Accept()");
            exit(5);
        }
        client_ip = inet_ntoa(client.sin_addr);
        client_port = ntohs(client.sin_port);

        do{
            memset(RCV_BUFF,'\0',BUFF_LEN);
            status = recv(ns, RCV_BUFF, 3, 0);
            if(status == -1){
                perror("Recv()");
                exit(6);
            }
            else if(status == 0){
                printf("(CLIENTE %s:%d) : FOI DESCONECTADO\n",client_ip,client_port);
                break;
            }
            log_req(RCV_BUFF,client_ip,client_port);
            op = atoi(&RCV_BUFF[2]);

            switch(op){
                case 1:
                    cadastro(&ns,client_ip,client_port);
                    break;
                case 2:
                    leitura(&ns,client_ip,client_port);
                    break;
                case 3:
                    apaga(&ns,client_ip,client_port);
                    break;
                case 4:
                    break;
                default:
                    log_resp("UNKNOWN OP",client_ip,client_port);
                    break;
            }
        }while(op!=4);
    close(ns);  //socket conectado ao cliente
    }
    close(s);   //socket de conexoes
    printf("Servidor terminou com sucesso.\n");
    free(client_ip);
    exit(0);
}

// ------------------------------------------------------------------------
void cadastro(int * s, char * ip, int port){
    char BUFF[BUFF_LEN];
    if(QTD>=MSG_NUM){
        strcpy(BUFF,"XX");
        if (send(*s, BUFF, 2, 0) < 0){
            perror("Send()");
            exit(7);
        }
        log_resp(BUFF,ip,port);
        return;
    }

    strcpy(BUFF,"OK");
    if (send(*s, BUFF, 2, 0) < 0){
        perror("Send()");
        exit(7);
    }
    log_resp(BUFF,ip,port);
    
    msg M;
    if (recv(*s, &M, sizeof(msg), 0) == -1){
        perror("Recv()");
        exit(6);
    }
    int i = find_empty_msg();
    if(i==-1){
        printf(">ERRO nao ha mais lugares livres no vetor MSGS\n");
        return;
    }
    strcpy(MSGS[i].usuario,M.usuario);
    strcpy(MSGS[i].mensagem,M.mensagem);
    QTD++;

    sprintf(BUFF,"msg_%d @%s > %s",i,M.usuario,M.mensagem);
    log_req(BUFF,ip,port);
}

void leitura(int * s, char * ip, int port){
    char BUFF[BUFF_LEN];
    //envia response de OK
    strcpy(BUFF,"OK");
    if (send(*s, BUFF, 2, 0) < 0){
        perror("Send()");
        exit(7);
    }
    log_resp(BUFF,ip,port);
    //response com QTD (int)
    if (send(*s, &QTD, sizeof(int), 0) < 0){
        perror("Send()");
        exit(7);
    }
    sprintf(BUFF,"%d",QTD);
    log_resp(BUFF,ip,port);
    if(QTD>0){
        int i;
        for(i=0;i<MSG_NUM;i++){
            //1 response para cada mensagem existente
            if(MSGS[i].usuario[0] != '\1'){
                if (send(*s, &MSGS[i], sizeof(msg), 0) < 0){
                    perror("Send()");
                    exit(7);
                }
                sprintf(BUFF,"msg_%d @%s > %s",i,MSGS[i].usuario,MSGS[i].mensagem);
                log_resp(BUFF,ip,port);
            }
        }
    }
}

void apaga(int * s, char * ip, int port){
    //request: usuario
    char user[USR_SIZE];
    char TMP_BUFF[BUFF_LEN];
    memset(user,'\0',USR_SIZE);
    if (recv(*s, user, USR_SIZE, 0) == -1){
        perror("Recv()");
        exit(6);
    }
    int count = count_user_msgs(user);
    //response: count
    if (send(*s, &count, sizeof(int), 0) < 0){
        perror("Send()");
        exit(7);
    }
    sprintf(TMP_BUFF,"%d",count);
    log_resp(TMP_BUFF,ip,port);
    if(count > 0){
        int i;
        for(i=0;i<MSG_NUM;i++){
            //response MSGS apagadas
            if(strcmp(MSGS[i].usuario,user)==0){
                if (send(*s, &MSGS[i], sizeof(msg), 0) < 0){
                    perror("Send()");
                    exit(7);
                }
                //log
                sprintf(TMP_BUFF,"msg_%d @%s > %s",i,MSGS[i].usuario,MSGS[i].mensagem);
                log_resp(TMP_BUFF,ip,port);
                //apaga msgs
                memset(MSGS[i].usuario,'\1',USR_SIZE);
                QTD--;
            }
        }
    }
}