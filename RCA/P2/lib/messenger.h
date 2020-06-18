#ifndef _MESSENGER_H
#define _MESSENGER_H
#include <stdio.h> 
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "contatos.h"
#include "regs.h"
#ifndef _BYTE_DEF
#define _BYTE_DEF
typedef unsigned char byte;
#endif
//biblioteca de comunicacao

//funcoes comunicacao com a central -----------------------------------------
int consulta_central(int * central, Regs * REGS, char * numeros[13], unsigned short n); //fazer free ctts dps
bool exit_central(int * central);
//funcoes comunicacao P2P ---------------------------------------------------
bool envia_msg(char user_telefone[13], reg * R, void * msg, int size, bool msg_type, char * filename);
byte * recebe_msg(int * ns, int * size, char sender_tel[13]);
//RECEBE DOIS TIPOS D MSG    E IMPRIMER!!!
//void recebe_txt(int * ns, char * msg);
//void recebe_img(int * ns, FILE * f);

#endif