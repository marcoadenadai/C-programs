#ifndef AUX_H_INCLUDED
#define AUX_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include "ascii.h"

void hxdump(char * conteudo, int size){
    int i;
    for(i=0;i<size;i+=2){
        printf("0x%02X%02X, ",conteudo[i],conteudo[i+1]);
    }
}

int send_data_end(int * datasock){
    if (send(*datasock, "00\0", 3, 0) < 0){
            perror("Send()");
            return -1;
    }
    return 1;
}
int send_data(int * datasock, byte * conteudo, unsigned int size){
    unsigned int i,j, blocks = size/255, resto=size%255;
    char s[3], * sendbuff;
    byte * msg = conteudo;
    strcpy(s,"FF");
    for(i=0;i<blocks;i++, msg+=255){
        if (send(*datasock, s, 3, 0) < 0){
            perror("Send()");
            return -1;
        }
        sendbuff = byte2ASCII(msg,255);
       
        if (send(*datasock, sendbuff, 255*2, 0) < 0){
            perror("Send()");
            return -2;
        }
        free(sendbuff);
    }

    if(resto!=0){
        sprintf(s,"%02X",resto);//NUMERO DE BYTES!! (DOBRO ASCII)
        if (send(*datasock, s, 3, 0) < 0){
            perror("Send()");
            return -1;
        }
        sendbuff = byte2ASCII(msg,resto);
       
        if (send(*datasock, sendbuff, resto*2, 0) < 0){
            perror("Send()");
            return -2;
        }
        free(sendbuff);
    }
    return 1;
}

//* size eh alterado na funcao
byte * recv_datablock(int * datasock, unsigned short * size){
    char n[3]="xxx";
    if (recv(*datasock, n, 3, 0) == -1){
        perror("Recv()");
        exit(6);
    }
    if(strcmp(n,"00")==0){
        *size = 0;
        return NULL;
    }
    *size = (16*getHEX(n[0]) + getHEX(n[1]));
    char * rcvbuff = (char *) malloc(sizeof(byte)*2*(*size));
    if (recv(*datasock, rcvbuff, (*size)*2, 0) == -1){
        perror("Recv()");
        exit(6);
    }
    byte * ret = ASCII2byte(rcvbuff,2*(*size));//strlen(rcvbuff);
    free(rcvbuff);
    return ret;
}

#endif