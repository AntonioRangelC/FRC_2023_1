//Antonio Igor Carvalho - 180030264
// Antonio Rangel Chaves - 180098021
// Douglas da Silva Monteles - 190012200
// Enzo Gabriel Guedes Queiroz Saraiva - 160119006

// gerarsa.c

#include <stdio.h>
#include <stdlib.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Uso: %s -p\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "-p") == 0) {
        // Gerar números primos p e q
        BIGNUM *p = BN_new();
        BIGNUM *q = BN_new();
        RSA *rsa = RSA_new();

        if (BN_generate_prime_ex(p, 32, 1, NULL, NULL, NULL) &&
            BN_generate_prime_ex(q, 32, 1, NULL, NULL, NULL)) {
            FILE *f = fopen("primos.txt", "w");
            if (f) {
                BN_print_fp(f, p);
                fprintf(f, "#");
                BN_print_fp(f, q);
                fclose(f);
                printf("Primos gerados e salvos no arquivo primos.txt\n");
            } else {
                printf("Erro ao criar o arquivo primos.txt\n");
            }
        } else {
            printf("Erro ao gerar os números primos p e q\n");
        }

        printf("Numero p: %d\n", &p);
        printf("Numero q: %d\n", &q);

        // Gerar chaves pública e privada
        rsa = RSA_generate_key(1024, 65537, NULL, NULL);
        FILE *pub = fopen("chave.pub", "w");
        FILE *priv = fopen("chave.priv", "w");

        PEM_write_RSAPublicKey(pub, rsa);
        PEM_write_RSAPrivateKey(priv, rsa, NULL, NULL, 0, NULL, NULL);

        fclose(pub);
        fclose(priv);

        RSA_free(rsa);

        return 0;
    } else {
        printf("Opção inválida!\n");
        return 1;
    }
}
