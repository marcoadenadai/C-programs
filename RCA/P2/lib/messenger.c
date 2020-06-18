#include "messenger.h"

//funcoes comunicacao com a central -----------------------------------------
int consulta_central(int * central, Regs * REGS, char * numeros[13], unsigned short n){
    unsigned short ex = n;
    if (send(*central, &ex, sizeof(unsigned short), 0) < 0){
        perror("Send(Exit)");
        return 0;
    }
    int i,j,k=0;
    char * send_buff = (char *) malloc(sizeof(char)*n*13);
    for(i=0;i<n;i++){
        for(j=0;j<13;j++){
            send_buff[k]=numeros[i][j];
            k++;
        }
    }
    //envio todos os numeros do meu interesse
    if (send(*central, send_buff, sizeof(char)*n*13, 0) < 0){
        perror("Send(Buff)");
        return 0;
    }
    free(send_buff);
    reg * recv_regs = (reg *) malloc(sizeof(reg)*n);
    //espero resultado de quantas buscas deram sucesso!
    unsigned short sucesso = 0;
    if(recv(*central, &sucesso, sizeof(unsigned short), 0) == -1){
        perror("Recv()");
        return 0;
    }
    //recebe vetor de registros com prox = NULL
    if (sucesso > 0){
        if((recv(*central, recv_regs, sizeof(reg)*sucesso, 0)) == -1)
            return 0;//FAIOOOW
        for(i=0;i<sucesso;i++)
            insere_reg(REGS,recv_regs[i].telefone, recv_regs[i].ip, recv_regs[i].port);
    }
    
    free(recv_regs);
    return sucesso;
}

bool exit_central(int * central){
    unsigned short ex = 0;
    if (send(*central, &ex, sizeof(unsigned short), 0) < 0){
        perror("Send(Exit)");
        return 0;
    }
    return 1;
}

//funcoes comunicacao P2P ---------------------------------------------------
bool envia_msg(char user_telefone[13], reg * R, void * msg, int size, bool msg_type, char * filename){//0=TXT, 1=IMG
    struct sockaddr_in externo_addr;
    int externo;
    if ((externo = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        perror("Socket()");
        exit(3);
    }
    externo_addr.sin_family = AF_INET;
    externo_addr.sin_port = R->port;
    externo_addr.sin_addr.s_addr = R->ip;
    //Conecta ao servidor
    if (connect(externo, (struct sockaddr *)&externo_addr, sizeof(externo_addr)) < 0){
        perror("Connect()");
        return 0;
    }
    //FALO MEU NUMERO
    if (send(externo, user_telefone, sizeof(char)*13, 0) < 0){
        perror("Send()");
        return 0;
    }
    //FALO O TIPO DA MSG
    if (send(externo, &msg_type, sizeof(bool), 0) < 0){
        perror("Send()");
        return 0;
    }
    //FALO O TAMANHO DA MSG
    if (send(externo, &size, sizeof(int), 0) < 0){
        perror("Send()");
        return 0;
    }
    //ENVIO A MENSAGEM EM SI
    if(msg_type == 0){//TEXTO
        if (send(externo, (byte *)msg, sizeof(unsigned char)*size, 0) < 0){
            perror("Send()");
            return 0;
        }
    }
    else{//IMAGEM //------//
        byte tam = strlen(filename);
        //ENVIO FILENAME STRLEN
        if (send(externo, &tam, sizeof(byte), 0) < 0){
            perror("Send()");
            return 0;
        }
        //ENVIO FILENAME
        if (send(externo, filename, sizeof(char)*tam, 0) < 0){
            perror("Send()");
            return 0;
        }
        //msg = FILE *
        byte tmp_buff[256];
        
        do{
            memset(tmp_buff,'\0',256);
            tam = fread(tmp_buff,sizeof(byte),255,msg);
            if (send(externo, &tam, sizeof(byte), 0) < 0){
                perror("Send()");
                return 0;
            }
            if (send(externo, tmp_buff, tam*sizeof(byte), 0) < 0){
                perror("Send()");
                return 0;
            }
        }while(!feof(msg));
        tam=0;
        if (send(externo, &tam, sizeof(byte), 0) < 0){
            perror("Send()");
            return 0;
        }//FIM DE MENSAGEM
        fclose(msg);
        

    }
    close(externo);
    return 1;
}

byte * recebe_msg(int * ns, int * size, char sender_tel[13]){
    byte * ret = NULL;
    if (recv(*ns, sender_tel, sizeof(char)*13, 0) == -1){
        perror("Recv()");
        return NULL;
    }
    bool msg_type;
    if (recv(*ns, &msg_type, sizeof(bool), 0) == -1){
        perror("Recv()");
        return NULL;
    }
    if (recv(*ns, size, sizeof(int), 0) == -1){
        perror("Recv()");
        return NULL;
    }
    if(msg_type==0){//TXT
        ret = (byte *) malloc(sizeof(byte)*(*size +1));
        if (recv(*ns, ret, sizeof(unsigned char)*(*size), 0) == -1){
            perror("Recv()");
            return NULL;
        }
    }
    else{//IMG//msg = FILE *
        byte tam=0;
        //recebo filename strlen
        if (recv(*ns, &tam, sizeof(byte), 0) == -1){
                perror("Recv()");
                return NULL;
        }
        ret = malloc(sizeof(char)*tam);//recebo filename
        if (recv(*ns, ret, sizeof(char)*tam, 0) == -1){
                perror("Recv()");
                return NULL;
        }
        FILE * fp = fopen(ret, "wb");
        if(!fp){
            free(ret);
            return NULL;
        }
            
        byte tmp_buff[256];
        
        
        while(1){
            if (recv(*ns, &tam, sizeof(byte), 0) == -1){
                perror("Recv()");
                return NULL;
            }
            if(tam==0)
                break;
            memset(tmp_buff,'\0',256);
            if (recv(*ns, tmp_buff, sizeof(byte)*tam, 0) == -1){
                perror("Recv()");
                return NULL;
            }
            fwrite(tmp_buff,sizeof(byte),tam,fp);
        }
        fclose(fp);
    }
    return ret;
    //MUTEX
    //chama print_recebida(char tel[13], Contatos * C, char ** lines, char * msg)
}