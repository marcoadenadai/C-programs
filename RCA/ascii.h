#ifndef ASCII_H_INCLUDED
#define ASCII_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char byte;

char h2c(byte h){
    if(h>=0 && h<=9)
        return (0x30 + h);
    else if(h>=10 && h<=15)
        return (0x61 + h - 10);
}

//lembrar do free
char * byte2ASCII (byte * B, int size){
    char c[3], * ret = (char *) malloc(sizeof(char)*(1+(size*2)));
    ret[0]='\0';
    int i;
    for(i=0;i<size;i++){
        c[2]='\0';
        c[1]=h2c(B[i]%16);
        c[0]=h2c((B[i]/16)%16);
        strcat(ret,c);
    }
    return ret;
}//tem NULL no fim


byte getHEX(char c){
    if(c >= 0x30 && c<= 0x39)
        return (byte) c - 0x30;
    else if(c >= 0x41 && c<= 0x46)
        return (byte) 10 + c - 0x41;
    else if(c >= 0x61 && c<= 0x66)
        return (byte) 10 + c - 0x61;
}
//lembrar do free
byte * ASCII2byte (char * str, int size){
    byte * ret = (byte *) malloc(sizeof(byte)*(1+(size/2)));
    char c;
    int i;
    for(i=0;i<size/2;i++)
        ret[i] = 16*getHEX(str[i*2]) + getHEX(str[(i*2)+1]);
    return ret;
}//nao tem NULL no FIM
//lembrar do free
char * ushort2ASCII (unsigned short n){
    unsigned char * c = (unsigned char *) malloc(sizeof(unsigned char)*5);
    sprintf(c,"%04X",n);
    return c;
}//tem NULL no FIM

#endif