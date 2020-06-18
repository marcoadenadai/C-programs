#include <unistd.h>
#include "contatos.h"

Contatos * InicializaContatos(){
    Contatos * ret = (Contatos *) malloc(sizeof(Contatos));
    ret->total = 0;
    ret->Lista = NULL;
    return ret;
}

char * BuscaNumero(Contatos * C, char nome[32]){
    contato * aux;
    for(aux=C->Lista;aux;aux=aux->prox){
        if(strcmp(aux->nome,nome)==0)
            return aux->telefone;
    }
    return NULL;
}

char * BuscaNome(Contatos * C, char tel[13]){
    contato * aux;
    for(aux=C->Lista;aux;aux=aux->prox){
        if(strcmp(aux->telefone,tel)==0)
            return aux->nome;
    }
    return NULL;
}

bool InsereContato (Contatos * C, char nome[32], char tel[13]){
    int i;
    for(i=0;i<13;i++){
        if(tel[i] < 0x30 && tel[i] != 0x00 || tel[i] > 0x39)
            return 0;//NON NUMERIC TELEFONE
    }
    //insere ordenado (!!!! VER OQ ACONTECE SE TENTAR CONTATO COM MSM NOME!)
    contato * aux, * ant = NULL, * novo = (contato *)malloc(sizeof(contato));
    strcpy(novo->telefone,tel);
    strcpy(novo->nome,nome);
    for(aux=C->Lista;aux && strcmp(aux->nome,nome)<0;ant=aux ,aux=aux->prox);
    if(!ant){ //insere no inicio
        novo->prox = C->Lista;
        C->Lista = novo;
    }
    else{
        novo->prox = ant->prox;
        ant->prox = novo;
    }
    (C->total)++;
    return 1;
}

bool RemoveContato(Contatos * C, char tel[12]){
    contato * aux, * ant = NULL, * rmv;
    for(aux=C->Lista ;aux; ant=aux ,aux=aux->prox){
        if(strcmp(aux->telefone, tel)==0){
            if(!ant)//primeiro da fila
                C->Lista = NULL;
            else{
                if(aux->prox)//meio da fila
                    ant->prox = aux->prox;
                else//fim da fila
                    ant->prox = NULL;
            }
            free(aux);
            (C->total)--;
            return 1;
        }
    }
    return 0;
}

void LiberaContatos(Contatos * C){
    contato * aux, * rmv;
    for(aux=C->Lista;aux;){
        rmv=aux;
        aux=aux->prox;
        free(rmv);
    }
    free(C);
}

//--------------------------------------------------

typedef struct fileCtt {
    char nome[32];
    char telefone[13];
} fileCtt;


bool FileNew_Contato(char nome[32], char tel[13]){
    char txt[32+4];
    strcpy(txt,nome);
    strcat(txt,".ctt");
    FILE * fp = fopen(txt,"wb");
    if(!fp){    
        perror("File opening failed");
        return 0;
    }
    fileCtt novo;
    memset(novo.nome,'\0',32);
    memset(novo.telefone,'\0',13);
    strcpy(novo.nome,nome);
    strcpy(novo.telefone,tel);
    if(fwrite(&novo,sizeof(fileCtt),1,fp) != 1 ){
        fclose(fp);
        perror("File write failed");
        return 0;
    }
    fclose(fp);
    return 1;
}

bool FileRmv_Contato(char nome[32]){
    char txt[32+4];
    memset(txt,'\0',36);
    strcpy(txt,nome);
    strcat(txt,".ctt");
    if(remove(txt)==0)
        return 1;
    perror("Cant Remove File");
    return 0;
}


static char *last_strstr(const char *haystack, const char *needle){
    if (*needle == '\0')
        return (char *) haystack;
    char *result = NULL;
    for (;;) {
        char *p = strstr(haystack, needle);
        if (p == NULL)
            break;
        result = p;
        haystack = p + 1;
    }
    return result;
}

void FileLoadContatos(Contatos * C){
    if(!C)
        return;
    fileCtt novo;
    DIR * d;
    FILE * fp;
    char * ptr = NULL;
    struct dirent * dir;
    d = opendir(".");
    if(d){
        while((dir = readdir(d))){
            if(strlen(dir->d_name) > 4){
                ptr = dir->d_name;
                ptr += strlen(dir->d_name) - 4;
                if(strcmp(ptr,".ctt")==0){
                    fp = fopen(dir->d_name, "rb");
                    if(!fp){    
                        perror("File opening failed");
                        return;;
                    }
                    memset(novo.nome,'\0',32);
                    memset(novo.telefone,'\0',13);
                    if(fread(&novo,sizeof(fileCtt),1,fp) != 1 ){
                        fclose(fp);
                        perror("File read failed");
                        return;
                    }
                    if(!InsereContato(C,novo.nome, novo.telefone))
                        printf("Erro InsereContato - Returned 0.\n");
                    fclose(fp);
                }
            }
        }
        closedir(d);
    }
}

void ImprimeContatos(Contatos * C){
    if(!C)
        return;
    contato * aux;
    printf("\n");
    for(aux=C->Lista;aux;aux=aux->prox)
        printf("Nome: %s\t\t--\tTelefone: %s\n",aux->nome, aux->telefone);
    printf("\n");
}

contato * getContatoPos(Contatos * C, int pos){
    if(pos > C->total)
        return NULL;
    contato * aux;
    int i=0;
    for(aux=C->Lista;aux && i<pos;i++, aux=aux->prox);
    return aux;
}

char ** getContatosOptions(Contatos * C){
    char ** ret = (char **)malloc(sizeof(char*)*C->total);
    contato * aux;
    int tam, i;
    for(i=0, aux=C->Lista;aux;i++, aux=aux->prox){
        tam=(strlen(aux->nome) + strlen(aux->telefone) + 3);
        ret[i] = (char *) malloc(sizeof(char) * tam+1);
        memset(ret[i],'\0',tam+1);
        strcpy(ret[i],aux->nome);
        strcat(ret[i]," - ");
        strcat(ret[i],aux->telefone);
    }
    return ret;
}
/*
int main (void){
    Contatos * C = InicializaContatos();
    char nombre[32];
    char telefono[13];
    strcpy(nombre,"Markin Sangue Bao");
    strcpy(telefono,"019983561800");
    FileNew_Contato(nombre,telefono);

    printf("Contato: %s, Numero: %s\n", nombre, BuscaNumero(C,nombre));

    FileLoadContatos(C);

    printf("Contato: %s, Numero: %s\n", nombre, BuscaNumero(C,nombre));

    ImprimeContatos(C);

    //FileRmv_Contato(nombre);
    FileLoadContatos(NULL);
    // CRIAR FUNCOES FALTANTES DO CONTATOS.H
    LiberaContatos(C);
    ImprimeContatos(C);
    return 0;
}*/