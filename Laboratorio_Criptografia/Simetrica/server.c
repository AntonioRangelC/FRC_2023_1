#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <openssl/des.h>

#define PORT 12345
#define BUFFER_SIZE 1024
#define FILE_RECEIVED "received_file.bmp"

// Função para descriptografar o arquivo BMP usando DES no modo ECB
void decrypt_file(const char *input_file, const char *output_file, const unsigned char *key)
{
  FILE *input_fp, *output_fp;
  DES_key_schedule ks;
  DES_cblock des_key;

  // Abre o arquivo de entrada em modo binário
  input_fp = fopen(input_file, "rb");
  if (!input_fp)
  {
    perror("Erro ao abrir arquivo de entrada");
    return;
  }

  // Abre o arquivo de saída em modo binário
  output_fp = fopen(output_file, "wb");
  if (!output_fp)
  {
    perror("Erro ao abrir arquivo de saída");
    fclose(input_fp);
    return;
  }

  // Define a chave DES
  memcpy(des_key, key, strlen(key));
  DES_set_key(&des_key, &ks);

  // Lê e descriptografa os dados
  unsigned char input_block[8];
  unsigned char output_block[8];
  size_t bytes_read;

  while ((bytes_read = fread(input_block, 1, sizeof(input_block), input_fp)) > 0)
  {
    DES_ecb_encrypt(input_block, output_block, &ks, DES_DECRYPT);
    fwrite(output_block, 1, bytes_read, output_fp);
  }

  fclose(input_fp);
  fclose(output_fp);
}

void error(const char *message)
{
  perror(message);
  exit(EXIT_FAILURE);
}

int main()
{
  int server_socket, client_socket, file_size;
  struct sockaddr_in server_addr, client_addr;
  socklen_t client_addr_len = sizeof(client_addr);
  char buffer[BUFFER_SIZE];
  char received_key[9];
  FILE *file;

  // Criar socket
  if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    error("Erro ao criar socket.");

  // Configurar a estrutura de endereço do servidor
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);
  server_addr.sin_addr.s_addr = INADDR_ANY;

  // Vincular o socket à porta
  if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    error("Erro ao vincular.");

  // Escutar por conexões
  if (listen(server_socket, 1) == -1)
    error("Erro ao escutar por conexões.");

  printf("Aguardando conexão...\n");

  // Aceitar conexão de um cliente
  if ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len)) == -1)
    error("Erro ao aceitar conexão.");

  printf("Conexão estabelecida com o cliente.\n");

  // Receber o tamanho do arquivo BMP
  recv(client_socket, &file_size, sizeof(file_size), 0);
  file_size = ntohl(file_size);

  // Receber a chave do cliente
  int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
  
  if (bytes_received == -1)
    error("Erro ao receber dados.");

  buffer[bytes_received] = '\0'; // Adicionar o terminador nulo para formar a string
  strcpy(received_key, buffer);
  printf("Chave recebida do cliente: %s\n", received_key);

  buffer[0] = '\0';

  // Receber o arquivo BMP e salvar no disco
  file = fopen(FILE_RECEIVED, "wb");
  if (file == NULL)
    error("Erro ao criar o arquivo.");

  int total_received = 0;

  while (total_received < file_size)
  {
    bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
    if (bytes_received <= 0)
      break;

    fwrite(buffer, 1, bytes_received, file);
    total_received += bytes_received;
  }

  printf("Recebido arquivo BMP com sucesso. Tamanho total: %d bytes\n", total_received);

  // Fechar os sockets e o arquivo
  fclose(file);
  close(client_socket);
  close(server_socket);

  const char *decrypted_bmp_file = "fractal_decrypted.bmp";
  decrypt_file(FILE_RECEIVED, decrypted_bmp_file, received_key);

  return 0;
}
