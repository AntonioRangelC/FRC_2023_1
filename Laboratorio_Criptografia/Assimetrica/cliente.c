//Antonio Igor Carvalho - 180030264
// Antonio Rangel Chaves - 180098021
// Douglas da Silva Monteles - 190012200
// Enzo Gabriel Guedes Queiroz Saraiva - 160119006


// cliente.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>

int encrypt_file(const char* input_file, const char* output_file, RSA* pub_key) {
    FILE* input_fp = fopen(input_file, "rb");
    if (!input_fp) {
        perror("Erro ao abrir arquivo de entrada");
        return 1;
    }

    FILE* output_fp = fopen(output_file, "wb");
    if (!output_fp) {
        perror("Erro ao abrir arquivo de saída");
        fclose(input_fp);
        return 1;
    }

    int rsa_len = RSA_size(pub_key);
    unsigned char input_block[rsa_len - 11];
    unsigned char output_block[rsa_len];

    int bytes_read;
    while ((bytes_read = fread(input_block, 1, rsa_len - 11, input_fp)) > 0) {
        int encrypted_len = RSA_public_encrypt(bytes_read, input_block, output_block, pub_key, RSA_PKCS1_PADDING);
        fwrite(output_block, 1, encrypted_len, output_fp);
    }

    fclose(input_fp);
    fclose(output_fp);
    return 0;
}

int main() {
    int client_socket;
    struct sockaddr_in server_address;
    RSA* pub_key = NULL;

    // Carregar chave pública
    FILE* pub_file = fopen("chave.pub", "r");
    if (!pub_file) {
        perror("Erro ao abrir chave.pub");
        return 1;
    }

    pub_key = PEM_read_RSAPublicKey(pub_file, NULL, NULL, NULL);
    if (!pub_key) {
        perror("Erro ao carregar chave pública");
        fclose(pub_file);
        return 1;
    }

    fclose(pub_file);

    // Cria o socket do cliente
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Erro ao criar o socket");
        RSA_free(pub_key);
        return 1;
    }

    // Define as propriedades do socket do servidor
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(8080);

    if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0) {
        perror("Erro ao converter o endereço IP");
        RSA_free(pub_key);
        close(client_socket);
        return 1;
    }

    // Tenta conectar ao servidor
    if (connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("Erro ao conectar ao servidor");
        RSA_free(pub_key);
        close(client_socket);
        return 1;
    }

    // Encripta o arquivo fractal.c com a chave pública
    if (encrypt_file("fractal.bmp", "fractal_encrypted.bmp", pub_key) != 0) {
        perror("Erro ao encriptar o arquivo fractal.c");
        RSA_free(pub_key);
        close(client_socket);
        return 1;
    }

    // Envia o arquivo encriptado para o servidor (Bob)
    FILE* encrypted_fp = fopen("fractal_encrypted.bmp", "rb");
    if (!encrypted_fp) {
        perror("Erro ao abrir arquivo encriptado");
        RSA_free(pub_key);
        close(client_socket);
        return 1;
    }

    char buffer[1024];
    int bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), encrypted_fp)) > 0) {
        send(client_socket, buffer, bytes_read, 0);
    }

    fclose(encrypted_fp);

    // Fecha o socket do cliente
    RSA_free(pub_key);
    close(client_socket);

    return 0;
}
