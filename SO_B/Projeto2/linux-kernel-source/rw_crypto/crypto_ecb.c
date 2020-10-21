//INTEGRANTES E RA
//Breno Baldovinotti 		| 14315311
//Caroline Gerbaudo Nakazato 	| 17164260
//Marco Antônio de Nadai Filho 	| 16245961
//Nícolas Leonardo Külzer Kupka | 16104325
//Paulo Mangabeira Birocchi 	| 16148363

#include "crypto_ecb.h"

static char *key = "0102030405060708A1A2A3A4A5A6A7A8";

//******************************************************************************************//
//                      FUNCOES                                                             //
static void getRAWString(unsigned char * RAWout, char * str, int key_size){
    int i,j;
    char c[2];
    for(i=0;i<key_size;i++){
        for(j=0;j<2;j++){
            c[j] = str[i*2+j];
            if(c[j] >= 'A' && c[j] <= 'F')
                c[j] = c[j] - 'A' + 10;
            else if(c[j] >= '0' && c[j] <= '9')
                c[j] = c[j] - '0';
            else{
                printk(KERN_WARNING "ERRO: O texto \'%c\' (0x%x) nao corresponde a um valor HEXA valido..\n",c[0],c[0]);
            }
        }
        RAWout[i] = c[0] * 16 + c[1];
    }
}
static unsigned char * alloc_RAWString(char * str, int key_size){
    char * tmp = kmalloc(1+(key_size*2), GFP_KERNEL);
    unsigned char * ret = kmalloc(key_size+1, GFP_KERNEL);
    
    int i;
    if(strlen(str) >= key_size*2){
        for(i=0;i<key_size*2;i++){
            tmp[i]=str[i];
        }
    }
    else{//faco o padding se necessario na key
        for(i=0;i<strlen(str);i++){
            tmp[i]=str[i];
        }
        for(i=strlen(str);i<key_size*2;i++){
            tmp[i]='0';
        }
    }
    getRAWString(ret,tmp,key_size);
    kfree(tmp);
    return ret;
}
message * alloc_hex_padding(unsigned char * input, int size){
    message * M = kmalloc(sizeof(message), GFP_KERNEL);
    int blocks = size/KEY_SIZE;
    int  i=0;
    if(size < KEY_SIZE || size%KEY_SIZE != 0){
        M->data = kmalloc(1+(KEY_SIZE*(blocks+1)), GFP_KERNEL); //alloca nova memoria relativo ao bloco atual
        if (!M->data) {
            pr_info("Nao foi possivel alocar o texto a ser encriptado\n");
            return NULL;
        }
        for(i=0;i<size;i++){
            M->data[i]=input[i];
        }
        for(i=size%KEY_SIZE; i<KEY_SIZE ;i++){//FAZ O PADDIN A PARTIR DA END_POS
            M->data[i+(blocks * KEY_SIZE)]= '\0';//''#';
        }
        M->data[i+(blocks * KEY_SIZE)]='\0';
        pr_info("PADDING EXECUTADO!\n");
        M->blocks=blocks+1;
    }
    else{ //se estiver com o padding correto só aloca
        M->data = kmalloc(1+size, GFP_KERNEL);
        if (!M->data) {
            pr_info("Nao foi possivel alocar o texto a ser encriptado\n");
            return NULL;
        }
        for(i=0;i<size;i++){
            M->data[i]=input[i];
            M->blocks=size/KEY_SIZE;
        }
    }
    return M;
}
//******************************************************************************************//
static int skcipher_result(struct skcipher_def * sk, int rc);
static void skcipher_callback(struct crypto_async_request *req, int error);
//static void skcipher_finish(struct skcipher_def * sk);
//******************************************************************************************//
//******************************************************************************************//
int aes_enc_dec(int mode, message * input, struct skcipher_def * sk){
    int ret = -EFAULT;
    //Alocar handle de cifra com chave simétrica (tfm)
    if (!sk->tfm) {
        sk->tfm = crypto_alloc_skcipher("ecb-aes-aesni", CRYPTO_ALG_TYPE_ABLKCIPHER/*PODESER0*/, 0);
        if (IS_ERR(sk->tfm)) {
            pr_info("Nao foi possivel alocar o handle de cifra (TFM)\n");
            return PTR_ERR(sk->tfm);
        }
    }
    //Alocar handle de request na memoria do kernel
    if (!sk->req) {
        sk->req = skcipher_request_alloc(sk->tfm, GFP_KERNEL);
        if (!sk->req) {
            pr_info("Nao foi possivel alocar o request na memoria do kernel\n");
            ret = -ENOMEM;
            return ret;
        }
    }
    skcipher_request_set_callback(sk->req, CRYPTO_TFM_REQ_MAY_BACKLOG,
                                  skcipher_callback,
                                  &sk->result);

    //CHAVE HARDCODED!!!!
    sk->key = alloc_RAWString(key, KEY_SIZE);

    /* AES 128 with given symmetric key */
    if (crypto_skcipher_setkey(sk->tfm, sk->key, KEY_SIZE)) {
        pr_info("A Chave Criptografica nao pode ser configurada\n");
        ret = -EAGAIN;
        return ret;
    }

    //STRING PARA A SAIDA !!!!!!!!!!!!!!!! 
    if (!sk->ciphertext) {
        sk->ciphertext = kmalloc(KEY_SIZE*input->blocks, GFP_KERNEL);
        if (!sk->ciphertext) {
            pr_info("Nao foi possivel alocar o vetor de saida (ciphertext)\n");
            return ret;
        }
    }
    memset(sk->ciphertext,'\0',KEY_SIZE*input->blocks);

    sg_init_one(&sk->in, input->data, KEY_SIZE*input->blocks);
    sg_init_one(&sk->out, sk->ciphertext, KEY_SIZE*input->blocks);
    //FUNCAO PRINCIPAL DE CRITOGRAFIA!!!
    skcipher_request_set_crypt(sk->req, &sk->in, &sk->out,
                               KEY_SIZE*input->blocks, NULL/*sk->ivdata*/);
    init_completion(&sk->result.completion);

    if(mode <= 0){
        pr_info("ENCRYPT AES128 OPERATION:\n");
        ret = crypto_skcipher_encrypt(sk->req);
    }
    else
    {
        pr_info("DECRYPT AES128 OPERATION:\n");
        ret = crypto_skcipher_decrypt(sk->req);
    }
        
    ret = skcipher_result(sk, ret);
    if (ret)
        return ret;

    pr_info("Encryption request successful\n");
    return ret;

}
//******************************************************************************************//
//******************************************************************************************//
static void skcipher_callback(struct crypto_async_request *req, int error)
{
    struct tcrypt_result *result = req->data;

    if (error == -EINPROGRESS)
        return;

    result->err = error;
    complete(&result->completion);
    pr_info("Encryption finished successfully\n");
}

static int skcipher_result(struct skcipher_def * sk, int rc)
{
    switch (rc) {
    case 0:
        break;
    case -EINPROGRESS:
    case -EBUSY:
        rc = wait_for_completion_interruptible(
            &sk->result.completion);
        if (!rc && !sk->result.err) {
            reinit_completion(&sk->result.completion);
            break;
        }
    default:
        pr_info("skcipher encrypt returned with %d result %d\n",
            rc, sk->result.err);
        break;
    }

    init_completion(&sk->result.completion);

    return rc;
}

void skcipher_finish(struct skcipher_def * sk)
{
    if (sk->tfm)
        crypto_free_skcipher(sk->tfm);
    if (sk->req)
        skcipher_request_free(sk->req);
    if (sk->key)
        kfree(sk->key);
    if (sk->ciphertext)
        kfree(sk->ciphertext);
}
