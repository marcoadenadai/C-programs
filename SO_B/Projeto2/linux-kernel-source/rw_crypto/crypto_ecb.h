//INTEGRANTES E RA
//Breno Baldovinotti 		| 14315311
//Caroline Gerbaudo Nakazato 	| 17164260
//Marco Antônio de Nadai Filho 	| 16245961
//Nícolas Leonardo Külzer Kupka | 16104325
//Paulo Mangabeira Birocchi 	| 16148363

#include <crypto/internal/skcipher.h>
#include <linux/crypto.h>

#define KEY_SIZE 16
#define ENCRYPT 0
#define DECRYPT 1

struct tcrypt_result {
    struct completion completion;
    int err;
};

struct skcipher_def {
    struct scatterlist in;
    struct scatterlist out;
    struct crypto_skcipher * tfm;
    struct skcipher_request * req;
    struct tcrypt_result result;
    char * scratchpad;
    char * ciphertext;
    unsigned char * key;
};

typedef struct message {
    char * data;
    int blocks;
} message;

void skcipher_finish(struct skcipher_def * sk);

message * alloc_hex_padding(unsigned char * input, int size);

int aes_enc_dec(int mode, message * input, struct skcipher_def * sk);
