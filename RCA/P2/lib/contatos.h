#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>

#ifndef _CONTATOS_H
#define _CONTATOS_H
#include <stdbool.h>

typedef struct contato {
    char nome[32];
    char telefone[13];
    struct contato * prox;
} contato;

typedef struct Contatos {
    unsigned long total;
    contato * Lista;
} Contatos;

Contatos * InicializaContatos();
bool InsereContato(Contatos * C, char nome[32], char tel[13]);
bool RemoveContato(Contatos * C, char tel[13]);
void LiberaContatos(Contatos * C);
char * BuscaNumero(Contatos * C, char nome[32]);
char * BuscaNome(Contatos * C, char tel[13]);

//FileNew_Contato   -   Cria Arquivo de Contato.
bool FileNew_Contato(char nome[32], char tel[13]);
//FileRmv_Contato   -   Deleta Arquivo de Contato.
bool FileRmv_Contato(char nome[32]);
//FileLoadContatos  -   Carrega Arquivos para Lista (MEMORIA).
void FileLoadContatos(Contatos * C);

void ImprimeContatos(Contatos * C);

int TotalContatos(Contatos * C);

contato * getContatoPos(Contatos * C, int pos);

char ** getContatosOptions(Contatos * C);

#endif