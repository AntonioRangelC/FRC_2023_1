#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

#define MAX_SALAS 100
#define MAX_STR_SIZE 256
#define LISTAR_SALAS 1
#define ENTRAR_SALA 2
#define NOVA_SALA 3
#define DESLIGAR 4
#define TAM_MIN_NOME 5
#define true 1
#define false 0

typedef int bool;

typedef struct {
    int cliente_sd;
    char nome[MAX_STR_SIZE];
    bool ativo;
} cliente;

typedef struct {
    fd_set sala_fd;
    int limite;
    int quantidade_clientes;
    bool ativo;
    cliente *clientes;
} sala;

fd_set master, read_fds;
struct sockaddr_in meu_endereco_socket, endereco_remoto_socket;
int maior_descritor_arquivo, novo_descritor_arquivo, num_bytes, yes=1, tamanho_endereco;
char buffer[MAX_STR_SIZE];
sala salas[MAX_SALAS];

void prepara_servidor();
void sair_da_sala (int socket_descritor_arquivo, int sala_id, int cliente_id, int retirar_master);
void lista_salas();
int cria_sala (int limite);
void envia_msg (int socket_descritor_arquivo, int server_sd, int sala_id, int cliente_id);
void entrar_na_sala(int socket_descritor_arquivo, int sala_id, char nome[], int tam_nome);
void executa_comando (int socket_descritor_arquivo, int sala_id, int cliente_id);
void validar_nome(int socket, char *nome);
void menu(int socket);

int main (int argc, char *argv[]) {
    if (argc < 3) {
        printf("Digite IP e porta para o servidor\nExemplo: 127.0.0.1 4000\n");
        exit(1);
    }

    // Faz a Limpeza dos sets master e das salas e inicializa o servidor
    prepara_servidor();

    // Configuracao de socket
    int socket_descritor_arquivo = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(socket_descritor_arquivo, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    meu_endereco_socket.sin_family = AF_INET;
    meu_endereco_socket.sin_addr.s_addr = inet_addr(argv[1]);
    meu_endereco_socket.sin_port = htons(atoi(argv[2]));
    memset(&(meu_endereco_socket.sin_zero), 0, 8);

    

    // Bind
    if (bind(socket_descritor_arquivo, (struct sockaddr *)&meu_endereco_socket, sizeof(meu_endereco_socket)) < 0)
    {
        perror("Erro ao fazer bind");
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(socket_descritor_arquivo, 10) < 0)
    {
        perror("Erro ao escutar por conexoes");
        exit(EXIT_FAILURE);
    }

    printf("Servidor sendo executado\n");

    // Adiciona os file descriptors no set master
    FD_SET(socket_descritor_arquivo, &master);
    FD_SET(0, &master);

    // maior_descritor_arquivo -> maior file descriptor (socket descriptor)
    maior_descritor_arquivo = socket_descritor_arquivo;
    // tamanho_endereco -> tamnaho da struct tamanho_endereco
    tamanho_endereco = sizeof(endereco_remoto_socket);

    int sala;
    int escolha;
    char opcao_invalida[] = "Escolha invalida, digite novamente\n";
    while(1) {
        // Informa que o master receberá descritores de leitura e realiza o select
        read_fds = master;
        select(maior_descritor_arquivo+1, &read_fds, NULL, NULL, NULL);

        for (int i = 0; i <= maior_descritor_arquivo; i++) {
            // Testa se o file descriptor esta no cesto
            if (FD_ISSET(i, &read_fds)) {
                // Checa o file descriptor e o socket
                if (i == socket_descritor_arquivo) {
                    // Conexao, adiciona o socket descriptor no cesto
                    novo_descritor_arquivo = accept(socket_descritor_arquivo, (struct sockaddr *)&endereco_remoto_socket, &tamanho_endereco);
                    FD_SET(novo_descritor_arquivo, &master);

                    int limite, tam_nome;
                    char nome[MAX_STR_SIZE];

                    char boas_vindas[] = "Seja bem-vindo(a) as salas de bate-papo, digite seu nome por favor\n";
                    send(novo_descritor_arquivo, boas_vindas, strlen(boas_vindas), 0);
                    
                    // Recebe nome do usuario
                    recv(novo_descritor_arquivo, nome, MAX_STR_SIZE, 0);

                    validar_nome(novo_descritor_arquivo, nome);
                    
                    tam_nome = strlen(nome);
                    
                    printf("%s acabou de entrar na sala de espera\n", nome);
                    

                    menu(novo_descritor_arquivo);
                    recv(novo_descritor_arquivo, buffer, MAX_STR_SIZE, 0); //Lendo opcao do menu

                    escolha = atoi(buffer);
                    
                    switch (escolha)
                    {
                    case LISTAR_SALAS:
                        lista_salas();
                        break;

                    case ENTRAR_SALA:
                        send(novo_descritor_arquivo, "Digite o numero da sala:\n", strlen("Digite o numero da sala:\n"), 0);
                        recv(novo_descritor_arquivo, buffer, MAX_STR_SIZE, 0);
                        sala = atoi(buffer);
                        entrar_na_sala(novo_descritor_arquivo, sala, nome, tam_nome);
                        break;

                    case NOVA_SALA:
                        // informa limite da sala
                        send(novo_descritor_arquivo, "Digite o tamanho da sala:\n", strlen( "Digite o tamanho da sala:\n"), 0);
                        recv(novo_descritor_arquivo, buffer, MAX_STR_SIZE, 0);
                        limite = atoi(buffer);
                        sala = cria_sala(limite);
                        break;
                    case DESLIGAR:

                        break;
                    default:

                        send(novo_descritor_arquivo, opcao_invalida, strlen(opcao_invalida), 0);
                        menu(novo_descritor_arquivo);
                        break;
                    }
                    

                    // De qualquer forma insere ele na sala nova ou existente
                    //entrar_na_sala(novo_descritor_arquivo, sala, nome, tam_nome);

                    // Se o valor do socket_descritor_arquivo for maior que o atual (mais coisas no cesto)
                    // atualiza esse valor para as proximas iteracoes do loop
                    if (novo_descritor_arquivo > maior_descritor_arquivo)
                        maior_descritor_arquivo = novo_descritor_arquivo;
                }
                else {
                    // Se nao for o descritor do socket, cria um buffer, recebe a mensagem
                    // e a retransmite por todos os sockets conectados
                    memset(&buffer, 0, sizeof(buffer));
                    num_bytes = recv(i, buffer, sizeof(buffer), 0);

                    // Encontra a sala que o descritor do socker se encontra
                    int sala_id;
                    for (sala_id = 0; sala_id < MAX_SALAS; sala_id++)
                        if (FD_ISSET(i, &salas[sala_id].sala_fd))
                            break;

                    // Encontra o id do cliente na sala atual do mesmo
                    int cliente_id;
                    for (cliente_id = 0; cliente_id < salas[sala_id].limite; cliente_id++)
                        if (salas[sala_id].clientes[cliente_id].cliente_sd == i)
                            break;

                    // Desconexao forcada
                    if (num_bytes == 0) {
                        printf("Desconectando forcadamente o descritor %d\n", i);
                        sair_da_sala(i, sala_id, cliente_id, 1);
                    }

                    // // Caso o primeiro caracter da mensagem seja uma / executa comando
                    // if (buffer[0] == '/')
                    //     executa_comando(i, sala_id, cliente_id);
                    // // Caso não, encaminha a mensagem na sala
                    // else{
                    //     envia_msg(i, socket_descritor_arquivo, sala_id, cliente_id);
                    // }
                    envia_msg(i, socket_descritor_arquivo, sala_id, cliente_id);
                }
            }
        }
    }

    return 0;
}


void menu(int socket){
    char opcoes[]  = "[1] Listar salas disponiveis\n[2]Entrar em sala de bate-papo\n[3] Criar sala de bate-papo\n[4] Desligar\n";
    send(socket, opcoes, strlen(opcoes), 0);

}

void validar_nome(int socket, char *nome){

    char mensagem_erro[] = "O nome deve ter pelo menos tres letras, digite novamente.\n";
    int tamanho_nome = strlen(nome);
    while( tamanho_nome <= TAM_MIN_NOME){

        send(socket, mensagem_erro , strlen(mensagem_erro), 0);        
        recv(socket, nome, MAX_STR_SIZE, 0);
    }

}


void prepara_servidor() {
    // Faz a Limpeza dos sets master 
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    // Inicializacao do servidor, zerando valores de todas as salas
    for (int i = 0; i < MAX_SALAS; i++) {
        FD_ZERO(&salas[i].sala_fd);
        salas[i].limite = 0;
        salas[i].quantidade_clientes = 0;
        salas[i].ativo = false;
    }
}

void sair_da_sala (int socket_descritor_arquivo, int sala_id, int cliente_id, int retirar_master) {
    printf("Sala %d: arquivo descriptor %d, user %s saiu.\n", sala_id, socket_descritor_arquivo, salas[sala_id].clientes[cliente_id].nome);
    // retirar o descritor da master e da sala.
    salas[sala_id].clientes[cliente_id].ativo = 0;
    // Ao sair da sala, deve-se diminuir a quantidade_clientes de clientes
    salas[sala_id].quantidade_clientes--;
    if (retirar_master == 1)
        FD_CLR(socket_descritor_arquivo, &master);
    FD_CLR(socket_descritor_arquivo, &salas[sala_id].sala_fd);

    // E caso a quantidade_clientes fique igual 0, deve-se fechar a mesma
    // deixando-a inativa e desalocando o vetor de clientes
    if (salas[sala_id].quantidade_clientes == 0) {
        printf("Sala %d: sairam todos os users, sala sera desativada!\n", sala_id);
        free(salas[sala_id].clientes);
        salas[sala_id].ativo = false;
    }
}

void lista_salas () {
    send(novo_descritor_arquivo, "\n======= Salas Criadas =======\n", strlen( "\n======= Salas Criadas =======\n"), 0);
    char sala_char[3];
    char limite_char[3];
    char qtd_clientes_char[3];

    send(novo_descritor_arquivo, "\n Lista de salas ativas: \n", strlen( "\nLista de salas ativas: \n"), 0);
    
    for (int sala = 0; sala < MAX_SALAS; sala++) {
        if (salas[sala].ativo == true){
            // for(int i = 0; i < 250; i++){
            char msg[150];
            //     msg[i] = ;
            // }
            int limite = salas[sala].limite;
            int qtd_clientes = salas[sala].quantidade_clientes;
            strcat(msg, "Sala ");
            sprintf(sala_char, "%d", sala);
            sprintf(limite_char, "%d", limite);
            sprintf(qtd_clientes_char, "%d", qtd_clientes);
            printf("\nLista de salas ativas: \n");
            printf("Sala %d: ativada, capacidade maxima de usuarios: %d\n", sala, limite);
            strcat(msg, sala_char);
            strcat(msg, ": ativa (");
            strcat(msg, qtd_clientes_char);
            strcat(msg, "/");
            strcat(msg, limite_char);
            strcat(msg, ")\n");
            send(novo_descritor_arquivo, msg, strlen(msg), 0);
        }
    }
}

int cria_sala (int limite) {
    // Para criar uma sala, deve-se encontrar a primeira sala
    // vazia (ativo = 0) setar como ativa e atualizar seu limite
    int sala;
    for (sala = 0; sala < MAX_SALAS; sala++)
        if (salas[sala].ativo == false)
            break;

    salas[sala].ativo = 1;
    salas[sala].limite = limite;
    salas[sala].clientes = malloc(limite * sizeof(cliente));

    // Apos isso, deve-se instanciar o seu vetor de clientes
    // e desativar todos os presentes. Tambem e necessario 
    // retornar o valor da sala
    for (int i = 0; i < limite; i++)
        salas[sala].clientes[i].ativo = false;

    printf("Sala %d: ativada, capacidade maxima de %d usuarios.\n", sala, limite);
    return sala;
}

void envia_msg (int socket_descritor_arquivo, int server_sd, int sala_id, int cliente_id) {
    printf("Sala %d: mensagem do file descriptor %d enviada.\n", sala_id, socket_descritor_arquivo);
    // Descriptor para cada file
    for (int j = 0; j <= maior_descritor_arquivo; j++){
        // checa se ele esta no cesto do master
        if (FD_ISSET(j, &salas[sala_id].sala_fd)){
            // e checa se o valor nao e o descritor de si mesmo
            if (j != socket_descritor_arquivo && j != server_sd) {
                // por fim envia a mensagem para aquele socket descritor
                char msg[500] = "[";
                strcat(msg, salas[sala_id].clientes[cliente_id].nome);
                strcat(msg, "] => ");
                strcat(msg, buffer);
                send(j, msg, 500, 0);
            }
        }

    }
        

}

void entrar_na_sala(int socket_descritor_arquivo, int sala_id, char nome[], int tam_nome) {
    char msg[250];
    char sala_char[3];
    sprintf(sala_char, "%d", sala_id);
    strcat(msg, "Voce esta na sala ");
    strcat(msg, sala_char);
    send(socket_descritor_arquivo, msg, strlen( msg), 0);
    printf("Sala %d: file descriptor %d entrando.\n", sala_id, socket_descritor_arquivo);
    // Para inserir na sala, deve-se aumentar a quantidade_clientes, adicionar
    // o descritor no cesto da sala, encontra uma posição na sala
    // que esteja vazia (cliente.ativo = 0) e insere seus atributos
    // socket descriptor, ativo e nome
    FD_SET(socket_descritor_arquivo, &salas[sala_id].sala_fd);
    salas[sala_id].quantidade_clientes++;
    if (salas[sala_id].limite < salas[sala_id].quantidade_clientes){
        printf("sala na capacidade maxima!\n");
    }
    else {
        for (int i = 0; i < salas[sala_id].limite; i++) {
            if (salas[sala_id].clientes[i].ativo == false) {
                salas[sala_id].clientes[i].cliente_sd = socket_descritor_arquivo;
                salas[sala_id].clientes[i].ativo = 1;
                strncpy(salas[sala_id].clientes[i].nome, nome, tam_nome);
                break;
            }
        }
    }
}

void executa_comando (int socket_descritor_arquivo, int sala_id, int cliente_id) {
    buffer[strlen(buffer) - 2] = '\0';
    printf("Comando \"%s\" acionado na sala %d pelo file descriptor %d\n", buffer, socket_descritor_arquivo, sala_id);
    char resp_buf[MAX_STR_SIZE];

    // Se o recv retornar 0 ou a mensagem foi de sair
    // retira o socket descriptor do cesto
    if (strncmp(buffer+1, "sair", 4) == 0 || strncmp(buffer+1, "s", 1) == 0) {
        printf("Desconectando descritor %d\n", socket_descritor_arquivo);
        strcpy(resp_buf, "Cliente Desconectado\n");
        send(socket_descritor_arquivo, resp_buf, strlen(resp_buf), 0);
        close(socket_descritor_arquivo);
        sair_da_sala(socket_descritor_arquivo, sala_id, cliente_id, 1);
    }

    // Caso o comando seja listar, deve-se passar por todos os
    // clientes ativos da sala e lista-los enviando-os com sends
    if (strncmp(buffer+1, "listar", 6) == 0 || strncmp(buffer+1, "l", 1) == 0) {
        send(socket_descritor_arquivo, "\n======= Clientes Conectados Na Sala =======", 40, 0);
        for (int i = 0; i < salas[sala_id].limite; i++) {
            cliente c = salas[sala_id].clientes[i];
            if (c.ativo == 1 && c.cliente_sd != socket_descritor_arquivo) {
                char nome[] = "\n";
                strcat(nome, c.nome);
                send(socket_descritor_arquivo, nome, strlen(nome), 0);
            }
            else if (c.ativo == 1 && c.cliente_sd == socket_descritor_arquivo) { 
                char nome[] = "\n[";
                strcat(nome, c.nome);
                strcat(nome, "]");
                send(socket_descritor_arquivo, nome, strlen(nome), 0);
            }
        }
        send(socket_descritor_arquivo, "\n\n", 2, 0);
    }

    // Caso o cliente queira trocar de sala, deve-se executar duas
    // rotinas, a de sair de uma sala e a de inserir em um sala
    if (strncmp(buffer+1, "trocar_sala", 11) == 0 || strncmp(buffer+1, "t", 1) == 0) {
        recv(socket_descritor_arquivo, buffer, MAX_STR_SIZE, 0);
        int nova_sala = atoi(buffer);
        char nome[MAX_STR_SIZE];
        strcpy(nome, salas[sala_id].clientes[cliente_id].nome);
        sair_da_sala(socket_descritor_arquivo, sala_id, cliente_id, 0);
        entrar_na_sala(socket_descritor_arquivo, nova_sala, nome, strlen(nome));
    }
}