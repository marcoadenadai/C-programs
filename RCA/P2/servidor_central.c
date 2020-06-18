#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
#include "lib/regs.h"
#define DEFAULT_PORT 7777

typedef struct pthread_arg{
    int socket;
    unsigned long  ip;      //ip   do modulo de usuario
    unsigned short port;    //port do modulo de usuario    
}pthread_arg;

pthread_t pthread;
void * thread_func(void * arg);

pthread_mutex_t lock;

Regs * REGS;
////////////////////////////////////////////////////////////////////////
void _ntoa (unsigned long in, char * buffer);

//-----------------------------------------------------------------------------------------------
int main (int argc, char ** argv){
    REGS = inicializa_reg();

    if (pthread_mutex_init(&lock, NULL) != 0) { 
        printf("\n mutex init has failed\n"); 
        return -1; 
    }

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

    printf("Starting Servidor WhatsAp2p, binding to port %d.\n", port);

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

    while(1){
        namelen = sizeof(client);
        if ((ns = accept(s, (struct sockaddr *)&client, (socklen_t *)&namelen)) == -1){
            perror("Accept()");
            exit(5);
        }
        pthread_arg * arg = (pthread_arg *) malloc(sizeof(pthread_arg));
        arg->socket = ns;
        arg->ip = client.sin_addr.s_addr;   // inet_ntoa( client..) para transformar em array
        arg->port = ntohs(client.sin_port);

        if (pthread_create(&pthread, NULL, thread_func, (void *)arg) != 0){
            perror("pthread_create");
            free(arg);
            continue;
        }
    }
    libera_reg(REGS);
    close(s);   //socket de conexoes
    printf("Servidor terminou com sucesso.\n");
    pthread_mutex_destroy(&lock); 
    exit(0);
    return 0;
}
//------------------------------------------------------------------------------------------------

void * thread_func(void * arg){
    pthread_arg * _arg = (pthread_arg *)arg;
    int i;

    reg registro;
    memset(registro.telefone,'\0',13);
    if (recv(_arg->socket, &registro, sizeof(registro), 0) == -1){
        perror("Recv()");
        exit(6);
    }

    bool sucesso = 0;
    pthread_mutex_lock(&lock);
    if (!insere_reg(REGS,registro.telefone, _arg->ip, registro.port)){
        pthread_mutex_unlock(&lock);
        printf("ERRO REGISTRO INFALIDO!!!");
        if (send(_arg->socket, &sucesso, sizeof(bool), 0) < 0){
            perror("Send(ss)");
            return 0;
        }
        close(_arg->socket);  //encerra ns socket de controle
        free(_arg);
        return NULL;
    }
    pthread_mutex_unlock(&lock);
    sucesso = 1;
    if (send(_arg->socket, &sucesso, sizeof(bool), 0) < 0){
        perror("Send(ss)");
        return 0;
    }
    char txt_ip[18];
    _ntoa(_arg->ip,txt_ip);
    printf("NOVA CONEXAO (%s:%d) \t-\tTelefone: %s\n",txt_ip, ntohs(registro.port),
    registro.telefone);
    //NOVA CONEXAO RECEBIDA
    unsigned short ss, n=0;
    while(1){
        ss=0;
        if (recv(_arg->socket, &n, sizeof(unsigned short), 0) == -1){
            perror("Recv()");
            exit(6);
        }
        if(n==0){
            pthread_mutex_lock(&lock);
            remove_reg(REGS,registro.telefone);
            pthread_mutex_unlock(&lock);
            break;
        }
        else{//CONSULTA REGISTROS
            printf("NOVA CONSULTA [%s:%d] (%s) #%d\n",txt_ip, 
            ntohs(registro.port), registro.telefone, n);
            char * recv_buff = (char *) malloc(sizeof(char)*n*13);
            if (recv(_arg->socket, recv_buff, sizeof(char)*n*13, 0) == -1){
                perror("Recv()");
                exit(6);
            }//RECEBO TODOS OS NOMES DESEJADOS
            reg * R_SND = (reg *) malloc(sizeof(reg)*n);
            reg * busca;
            char * tmp_num;
            for(i=0;i<n;i++){//CONSULTO CADA UM
                tmp_num = recv_buff + (i*13);
                //MUTEX!!!!!!!!!!!!!
                pthread_mutex_lock(&lock);
                busca=busca_reg(REGS,tmp_num);
                pthread_mutex_unlock(&lock);
                printf("(%d)\t%s\n",!(busca==NULL),tmp_num);
                if(busca){
                    strcpy(R_SND[ss].telefone,tmp_num);
                    R_SND[ss].ip = busca->ip;
                    R_SND[ss].port = busca->port;
                    R_SND[ss].prox = NULL;
                    ss++;
                }
            }
            printf("\n");
            free(recv_buff);
            if (send(_arg->socket, &ss, sizeof(unsigned short), 0) < 0){
                perror("Send(ss)");
                return 0;
            }
            //ENVIO OS REGISTROS NECESSARIOS (VETOR DE REGISTROS)
            if (ss > send(_arg->socket, R_SND, sizeof(reg)*ss, 0) < 0){
                perror("Send(Buff)");
                return 0;
            }
            free(R_SND);
        }
    }

    printf("ENCERRANDO (%s:%d) \t-\tTelefone: %s\n",txt_ip, ntohs(registro.port),
    registro.telefone);
    close(_arg->socket);  //encerra ns socket de controle
    free(_arg);
}
//--------------------------------------------------------------------------
//op_consulta_registros()


///////////////////////////////////////////////////////////////////
void _ntoa (unsigned long in, char * buffer){
    unsigned char *bytes = (unsigned char *) &in;
    sprintf(buffer, "%d.%d.%d.%d", bytes[0], bytes[1],
                                     bytes[2], bytes[3]);
}