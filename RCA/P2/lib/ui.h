#ifndef _INTERFACE_H
#define _INTERFACE_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include "contatos.h"

#ifndef _BYTE_DEF
#define _BYTE_DEF
typedef unsigned char byte;
#endif 

//INTERFACE DE USUARIO USANDO NCURSES - para compilar, use: gcc (...) -lncurses

#define KEY_F1  265 //contatos
#define KEY_F2  266 //mensagem
#define KEY_F3  267 //exit
#define KEY_TAB 9
//KEY_BACKSPACE existe
#define KEY_SPACE 32
#define KEY_ENTERR 10

typedef struct menu_options{
    int total;
    unsigned short * selected_id;
}menu_options;

//FUNCTIONS
void draw_box(WINDOW * w);
void draw_inputbox(WINDOW * w, char * title, char * input, int size);
bool draw_msgbox(WINDOW * w, char * title, char * txt);
int draw_menu(WINDOW * w, char * title, char ** content, int n, int EXIT_KEY);
menu_options * draw_menu_multi(WINDOW * w, char * title, char ** content, int n, int EXIT_KEY);
//-----FUNCOES EXCLUSIVAS WHATSAP2P------------------------------------------------------------
contato * draw_novocontato(WINDOW * w);
void draw_send_txt(WINDOW * w, char * dest, char * input);

FILE * draw_send_img(WINDOW * w, char * dest, int EXIT_KEY, char ** fname);

void draw_messenger(WINDOW * w, char lines[19][77]);
void print_recebida(char tel[13], Contatos * C, char lines[19][77], char * msg);
void print_enviada(char tel[13], Contatos * C, char lines[19][77], char * msg);
//--------------------------------------------------------------------------------------------

#endif