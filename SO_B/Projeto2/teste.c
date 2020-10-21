//INTEGRANTES E RA
//Breno Baldovinotti 		| 14315311
//Caroline Gerbaudo Nakazato 	| 17164260
//Marco Antônio de Nadai Filho 	| 16245961
//Nícolas Leonardo Külzer Kupka | 16104325
//Paulo Mangabeira Birocchi 	| 16148363
#include <stdio.h>
#include <stdlib.h>
#include <linux/kernel.h>
#include <sys/syscall.h>
#include <sys/types.h>  // open()
#include <sys/stat.h>   // open()
#include <fcntl.h>      // open()
#include <unistd.h> //syscall() e close()
#include <string.h>

#define LEN 1024

int main(int argc, char ** argv)
{
    printf("Entre com uma string a ser criptografada:\n");
    char txt[LEN];
    scanf("%[^\n]%*c", txt);
    int tam = strlen(txt);

    int fd = open("out_teste.txt",O_WRONLY|O_CREAT,0640);//O_RDWR  = read and write
    if(fd != -1){
        syscall(546, fd, txt, tam); //faz o write_crypt
        close(fd);
    }

    //agora eh o read
    fd = open("out_teste.txt", O_RDONLY);
    char * r = (char *)malloc(tam);
    if(fd != -1){
        syscall(547, fd, r, tam); //faz o read_crypt
        close(fd);
    }
    printf("String READ=%s\n",r);
    int i;
    for(i=0;i<tam;i++){
        printf("string_read[%d]=0X%x\n",i,(unsigned char)r[i]);
    }
    free(r);

   return 0;
}
