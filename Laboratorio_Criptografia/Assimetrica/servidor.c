//Antonio Igor Carvalho - 180030264
// Antonio Rangel Chaves - 180098021
// Douglas da Silva Monteles - 190012200
// Enzo Gabriel Guedes Queiroz Saraiva - 160119006

// servidor.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>

void decrypt_file(int client_socket, RSA* priv_key) {
    FILE* input_fp = fdopen(client_socket, "rb");
    if (!input_fp) {
        perror("Erro ao abrir arquivo de entrada");
        return;
    }

    FILE* output_fp = fopen("fractal_decrypted.bmp", "wb");
    if (!output_fp) {
        perror("Erro ao abrir arquivo de saída");
        fclose(input_fp);
        return;
    }

    int rsa_len = RSA_size(priv_key);
    unsigned char input_block[rsa_len];
    unsigned char output_block[rsa_len];

    int bytes_read;
    while ((bytes_read = fread(input_block, 1, rsa_len, input_fp)) > 0) {
        int decrypted_len = RSA_private_decrypt(bytes_read, input_block, output_block, priv_key, RSA_PKCS1_PADDING);
        fwrite(output_block, 1, decrypted_len, output_fp);
    }

    fclose(input_fp);
    fclose(output_fp);
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    RSA* priv_key = NULL;

    // Carregar chave privada
    FILE* priv_file = fopen("chave.priv", "r");
    if (!priv_file) {
        perror("Erro ao abrir chave.priv");
        return 1;
    }

    priv_key = PEM_read_RSAPrivateKey(priv_file, NULL, NULL, NULL);
    if (!priv_key) {
        perror("Erro ao carregar chave privada");
        fclose(priv_file);
        return 1;
    }

    fclose(priv_file);

    // Cria o socket do servidor
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Erro ao criar o socket");
        RSA_free(priv_key);
        return 1;
    }

    // Define as propriedades do socket do servidor
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(8080);

    // Vincula o socket do servidor a uma porta e endereço
    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("Erro ao vincular o socket ao endereço");
        RSA_free(priv_key);
        close(server_socket);
        return 1;
    }

    // Inicia a escuta do socket do servidor
    if (listen(server_socket, 1) < 0) {
        perror("Erro ao escutar o socket");
        RSA_free(priv_key);
        close(server_socket);
        return 1;
    }

    // Aguarda conexões
    socklen_t client_address_len = sizeof(client_address);
    client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_address_len);

    if (client_socket < 0) {
        perror("Erro ao aceitar conexão");
        RSA_free(priv_key);
        close(server_socket);
        return 1;
    }

    // Recebe a mensagem encriptada de Alice e desencripta
    decrypt_file(client_socket, priv_key);

    // Fecha o socket do servidor
    RSA_free(priv_key);
    close(client_socket);
    close(server_socket);

    return 0;
}
