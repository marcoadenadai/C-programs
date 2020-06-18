//espera por conexoes de clientes em uma porta conhecida
//servidor espera por comandos de clientes conectados
//termina conexao quando o cliente envia msg de encerramento
//servidor concorrente, atende diversos clientes simultaneamente
/* Replies from the server are
   always a response character followed immediately by an ASCII message
   string terminated by a <NULL>.  A reply can also be just a response
   character and a <NULL>.

   <response> : = <response-code> [<message>] <NULL>

      <response-code> : =  + | - |   | !
      <message> can contain <CRLF>*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "ascii.h"
#include "aux.h"
#define DEFAULT_PORT 7777
#define BUFF_LEN 256

typedef struct pthread_arg{
    unsigned long ip;//ip de com quem se conversa
    int socket;
}pthread_arg;

pthread_t pthread; //VARIAVEL GLOBAL
void * thread_func(void * arg);

void op_listar(int * data);
void op_enviar(int * ns, int * data);
void op_receber(int * ns, int * data);
//funcs
unsigned short recv_ctl(int * ns, int * op);
int conectar(int * s, struct sockaddr_in * sck);
//------------------------------------------------------------------------------------------------
int main (int argc, char ** argv){
    unsigned short port;
    struct sockaddr_in client, server;
    int s, ns;  //s = aceitar conexoes, ns = conexao com o cliente
    int namelen;

    if(argc == 1)
        port = DEFAULT_PORT;
    else if(argc == 2){
        port = (unsigned short) atoi(argv[1]);
        if(port == 0)
            port = DEFAULT_PORT;
    }
    else{
        printf("ERROR: UNKNOWN ARGS  -  Usage: %s  [optional port]\n",argv[0]);
        exit(1);
    }

    printf("Starting Servidor PSTA, binding to port %d.\n", port);

    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        perror("Socket()");
        exit(2);
    }

    server.sin_family = AF_INET;
    server.sin_port   = htons(port);
    server.sin_addr.s_addr = INADDR_ANY; //faz com que o server ligue em todos end. IP

    if (bind(s, (struct sockaddr *)&server, sizeof(server)) < 0){
       perror("Bind()");
       exit(2);
    }

    //Prepara socket c/ fila de conexoes pendentes
    if (listen(s, 1) != 0){
        perror("Listen()");
        exit(4);
    }

    char *client_ip;
    int client_port;
    while(1){
        namelen = sizeof(client);
        if ((ns = accept(s, (struct sockaddr *)&client, (socklen_t *)&namelen)) == -1){
            perror("Accept()");
            exit(5);
        }

        pthread_arg * arg = (pthread_arg *) malloc(sizeof(pthread_arg));
        arg->socket = ns;
        arg->ip = client.sin_addr.s_addr;

        if (pthread_create(&pthread, NULL, thread_func, (void *)arg) != 0){
            perror("pthread_create");
            free(arg);
            continue;
        }
    }
    close(s);   //socket de conexoes
    printf("Servidor terminou com sucesso.\n");
    free(client_ip);
    exit(0);
    return 0;
}
//------------------------------------------------------------------------------------------------

void * thread_func(void * arg){
    pthread_arg * arg_tmp = (pthread_arg *)arg;
    int data;//socket de dados, connect to client
    struct sockaddr_in addr_data;
    addr_data.sin_family = AF_INET;
    addr_data.sin_addr.s_addr = arg_tmp->ip;
    int op;
    unsigned short port;

    do{
        port = recv_ctl(&arg_tmp->socket,&op);
        if(port == -1){
            perror("Recv()");
            exit(6);
        }
        else if(port == 0){
            printf("(CLIENTE DESCONECTADO)\n");
            break;
        }
        addr_data.sin_port   = htons(port);
        if(op != 4 && conectar(&data, &addr_data)>0){//INICIA CONEXAOA DE DADOS
            printf("CONECTADO AO SERVER DO CLIENT!!\n");
            if (send(arg_tmp->socket, "+", 1, 0) < 0){
                perror("Send()");
                exit(5);
            }//envio confirmacao pelo NS de que conectei
            //realizar reqsts client e devolver o resultado (via conexao de dados)
            switch (op){
                case 1://listar
                op_listar(&data);
                break;
                case 2://receber
                op_receber(&arg_tmp->socket, &data);//cliente faz download
                break;
                case 3://enviar
                op_enviar(&arg_tmp->socket, &data);//cliente faz upload
                break;
                default://UNKNOWN OP
                break;
            }
           close(data);//ENCERRAR CONEXAO DE DADOS
        }
        else if (op!=4){
            printf("Nao foi possivel conectar no socket de dados do cliente, verifique o port forwarding..\n");
            if (send(arg_tmp->socket, "-", 1, 0) < 0){
                perror("Send()");
                exit(5);
            }
        }
    }while(op!=4);
    //OPERACAO 4 request para encerrar
    printf("Encerrando conexao de controle com o cliente!\n");
    close(arg_tmp->socket);  //encerra ns socket de controle
    free(arg_tmp);
}
//********************************************************************************************
void op_listar(int * data){
    FILE * fp = popen("ls","r");
    if(!fp){
        perror("popen()");
        return;
    }
    char buff[255];
    unsigned int tam=0;
    do{
        memset(buff,'\0',255);
        tam = fread(buff,sizeof(unsigned char),255,fp);
        send_data(data, buff, tam);
    }while(!feof(fp));
    pclose(fp);
    send_data_end(data);
}

void op_receber(int * ns, int * data){//DOWNLOAD
    char vet[4096+255]; //max linux filepath+name
    memset(vet,'\0',4096+255);
    byte * buff = NULL;
    unsigned short tam=0;
    while((buff=recv_datablock(data,&tam)) != NULL){
        strncpy(vet,buff,tam);
        buff[tam]='\0';
        free(buff);
    }//recebo o nome do arquivo
    FILE * fd = fopen(vet,"rb");//vet=filename
    if(!fd){//se nao eh valido, devolve - no CTL
        printf("FILE %s NOT FOUND\n",vet);
        if (send(*ns, "-", 1, 0) < 0){
            perror("Send()");
            return;
        }
        return;
    }
    if (send(*ns, "+", 1, 0) < 0){
            perror("Send()");
            return;
    }
    printf("client_download: %s\n",vet);
    //se arquivo eh valido!
    do{
        memset(vet,'\0',255);
        tam = fread(vet,sizeof(unsigned char),255,fd);
        send_data(data, vet, tam);
    }while(!feof(fd));
    fclose(fd);
    send_data_end(data);
}

void op_enviar(int * ns, int * data){//UPLOAD
    char vet[4096+255]; //max linux filepath+name
    memset(vet,'\0',4096+255);
    byte * buff = NULL;
    unsigned short tam=0;
    while((buff=recv_datablock(data,&tam)) != NULL){
        strncpy(vet,buff,tam);
        buff[tam]='\0';
        free(buff);
    }//recebo o nome do arquivo
    //TODO: Resolver casos que o caminho ao arquivo nao eh simples, criando pastas quando necessario!
    // uma solucao= talvez usar o strtok!
    FILE * fd = fopen(vet,"wb");//vet=filename
    if(!fd){//se nao eh valido, devolve - no CTL
        if (send(*ns, "-", 1, 0) < 0){
            perror("Send()");
            return;
        }
        return;
    }
    if (send(*ns, "+", 1, 0) < 0){
        perror("Send()");
        return;
    }//CTL= + se arquivo eh valido!
    printf("client_upload: %s\n",vet);
    
    while((buff=recv_datablock(data,&tam)) != NULL){
        fwrite(buff, 1, tam, fd);
        free(buff);
    }
    fclose(fd);
}



//********************************************************************************************
//*******************************


int conectar(int * s, struct sockaddr_in * sck){
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

unsigned short recv_ctl(int * ns, int * op){
    char recvbuff[6];
    memset(recvbuff,'\0',6);
    unsigned short ret = recv(*ns, recvbuff, 6, 0);
    if (ret==-1)
        return -1;
    else if (ret==0)
        return 0;
    
    *op = recvbuff[0] - 0x30;
    return 4096*getHEX(recvbuff[1]) + 256*getHEX(recvbuff[2]) + 16*getHEX(recvbuff[3]) + getHEX(recvbuff[4]);
}