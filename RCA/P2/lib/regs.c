#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "regs.h"

Regs * inicializa_reg(){
    Regs * ret = (Regs *) malloc(sizeof(Regs));
    ret->R = NULL;
    ret->total = 0;
    return ret;
}

reg * busca_reg (Regs * REGS, char tel[13]){
    reg * aux;
    for(aux=REGS->R;aux;aux=aux->prox){
        if(strcmp(aux->telefone,tel)==0)
            return aux;
    }
    return NULL;
}

bool insere_reg (Regs * REGS, char tel[13], unsigned long ip, unsigned short port){
    int i;
    for(i=0;i<13;i++){
        if(tel[i] < 0x30 && tel[i] != 0x00 || tel[i] > 0x39)
            return 0;//NON NUMERIC TELEFONE
    }
    if(busca_reg(REGS,tel))
        return 0;
    //insere ordenado
    reg * aux, * ant = NULL, * novo = (reg *)malloc(sizeof(reg));
    strcpy(novo->telefone,tel);
    novo->ip = ip;
    novo->port = port;
    for(aux=REGS->R;aux && strcmp(aux->telefone,tel)<0;ant=aux ,aux=aux->prox);
    if(!ant){ //insere no inicio
        novo->prox = REGS->R;
        REGS->R = novo;
    }
    else{
        novo->prox = ant->prox;
        ant->prox = novo;
    }
    (REGS->total)++;
    return 1;
}

bool remove_reg(Regs * REGS, char tel[13]){
    reg * aux, * ant = NULL, * rmv;
    for(aux=REGS->R ;aux; ant=aux ,aux=aux->prox){
        if(strcmp(aux->telefone, tel)==0){
            if(!ant)//primeiro da fila
                REGS->R = NULL;
            else{
                if(aux->prox)//meio da fila
                    ant->prox = aux->prox;
                else//fim da fila
                    ant->prox = NULL;
            }
            free(aux);
            (REGS->total)--;
            return 1;
        }
    }
    return 0;
}

void libera_reg(Regs * REGS){
    reg * aux, * rmv;
    for(aux=REGS->R;aux;){
        rmv=aux;
        aux=aux->prox;
        free(rmv);
    }
    free(REGS);
}

/*
void imprime_reg (Regs * REGS){
    reg * aux;
    for(aux=REGS->R;aux;aux=aux->prox)
        printf("Telefone: %s\tIp: %ld\tPort: %d\n",aux->telefone,aux->ip,aux->port);
}*/

int total_reg(Regs * REGS){
    int ret = 0;
    reg * aux;
    for(aux=REGS->R;aux;aux=aux->prox)
        ret++;
    return ret;
}