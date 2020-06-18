#ifndef _REGS_H
#define _REGS_H
#include <stdbool.h>

typedef struct reg{
    char telefone[13];
    unsigned long  ip;
    unsigned short port;
    struct reg * prox;
} reg; //lista ligada

typedef struct Regs{
    unsigned long total;
    reg * R;
}Regs;


Regs * inicializa_reg();

void libera_reg(Regs * REGS);

reg * busca_reg(Regs * REGS, char tel[13]);

bool insere_reg(Regs * REGS, char tel[13], unsigned long ip, unsigned short port);

bool remove_reg(Regs * REGS, char tel[13]);

#endif