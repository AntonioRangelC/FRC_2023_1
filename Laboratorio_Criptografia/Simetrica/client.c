/*
  Grupo:

  Antonio Igor Carvalho - 180030264
  Antonio Rangel Chaves - 180098021
  Douglas da Silva Monteles - 190012200
  Enzo Gabriel Guedes Queiroz Saraiva - 160119006
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define PORT 12345
#define BUFFER_SIZE 1024
#define FILE_TO_SEND "fractaljulia.bmp"
#define KEY "abcdefgh"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/des.h>

#define KEY_SIZE 8 // Tamanho da chave DES em bytes

// Função para criptografar o arquivo BMP usando DES no modo ECB
void encrypt_file(const char* input_file, const char* output_file, const unsigned char* key) {
    FILE *input_fp, *output_fp;
    DES_key_schedule ks;
    DES_cblock des_key;

    // Abre o arquivo de entrada em modo binário
    input_fp = fopen(input_file, "rb");
    if (!input_fp) {
        perror("Erro ao abrir arquivo de entrada");
        return;
    }

    // Abre o arquivo de saída em modo binário
    output_fp = fopen(output_file, "wb");
    if (!output_fp) {
        perror("Erro ao abrir arquivo de saída");
        fclose(input_fp);
        return;
    }

    // Define a chave DES
    memcpy(des_key, key, KEY_SIZE);
    DES_set_key(&des_key, &ks);

    // Lê e criptografa os dados
    unsigned char input_block[8];
    unsigned char output_block[8];
    size_t bytes_read;

    while ((bytes_read = fread(input_block, 1, sizeof(input_block), input_fp)) > 0) {
        DES_ecb_encrypt(input_block, output_block, &ks, DES_ENCRYPT);
        fwrite(output_block, 1, bytes_read, output_fp);
    }

    fclose(input_fp);
    fclose(output_fp);
}

void error(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

int main() {
    int client_socket, file_size;
    struct sockaddr_in server_addr;
    FILE *file;
    char buffer[BUFFER_SIZE];

    // Criar socket
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        error("Erro ao criar socket.");

    // Configurar a estrutura de endereço do servidor
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0)
        error("Erro ao converter IP.");

    // Conectar ao servidor
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
        error("Erro ao conectar ao servidor.");

    printf("Conectado ao servidor.\n");

    // Caminhos dos arquivos de entrada e saída
    const char* input_bmp_file = "fractaljulia.bmp";
    const char* encrypted_bmp_file = "fractal_encrypted.bmp";

    encrypt_file(input_bmp_file, encrypted_bmp_file, KEY);

    // Abrir o arquivo BMP a ser enviado
    file = fopen(encrypted_bmp_file, "rb");
    if (file == NULL)
        error("Erro ao abrir o arquivo.");

    // Obter o tamanho do arquivo BMP
    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Enviar o tamanho do arquivo BMP para o servidor
    int file_size_nw = htonl(file_size);
    send(client_socket, &file_size_nw, sizeof(file_size_nw), 0);

    // Enviar a chave para o servidor
    char message[] = KEY;
    int message_len = strlen(message);

    if (send(client_socket, message, message_len, 0) != message_len)
      error("Erro ao enviar a chave.");

    // Enviar o arquivo BMP para o servidor
    int total_sent = 0, bytes_sent;
    while ((bytes_sent = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        send(client_socket, buffer, bytes_sent, 0);
        total_sent += bytes_sent;
    }

    printf("Enviado arquivo BMP com sucesso. Tamanho total: %d bytes\n", total_sent);

    // Fechar o socket e o arquivo
    fclose(file);
    close(client_socket);

    return 0;
}
