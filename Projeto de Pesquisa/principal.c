/*
Autores: 
    - Antonio Rangel Chaves - 180098021
    - Antonio Ferreira de Castro Neto - 190044799
    - Enzo Gabriel Guedes Queiroz Saraiva - 160119006

*/




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
#define LISTAR_PARTICIPANTES_SALA 4
#define DESCONECTAR 5
#define TAM_MIN_NOME 2
#define QTD_MAX_CLIENTES 100
#define true 1
#define false 0

typedef int bool;

typedef struct
{
    int cliente_sd;
    char nome[MAX_STR_SIZE];
    bool ativo;
    int sala;
    int id_cliente;
} cliente;

typedef struct
{
    fd_set sala_fd;
    int limite;
    int quantidade_clientes;
    bool ativo;
    cliente *clientes;
} sala;

fd_set master, read_fds;
struct sockaddr_in meu_endereco_socket, endereco_remoto_socket;
int maior_descritor_arquivo, novo_descritor_arquivo, num_bytes, yes = 1, tamanho_endereco;
char buffer[MAX_STR_SIZE];
sala salas[MAX_SALAS];
cliente clientes_aplicacao[QTD_MAX_CLIENTES];

void prepara_servidor();
void sair_da_sala(int socket, sala salas[], cliente clientes[], int total_clientes);
void lista_salas();
int cria_sala(int limite, int socket);
void envia_msg(int socket_descritor_arquivo, int server_sd, int sala_id, int cliente_id);
void entrar_na_sala(int socket_descritor_arquivo, int sala_id, cliente cliente);
void menu(int socket, cliente cliente);
int validar_entrada(int sala, int socket);
void validar_nome(int socket, char *nome, int *tam_nome);
void desconectar_cliente(int socket, sala salas[], cliente clientes[], int total_clientes);
void lista_participantes_sala(int socket, int sala);

int qtd_clientes = 0;
char enter_continuar[50] = "Pressione enter para continuar!\n";

int main(int argc, char *argv[])
{
    int sala, id_socket;
    int escolha;

    if (argc < 3)
    {
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

    while (1)
    {
        FD_ZERO(&read_fds);
        FD_SET(socket_descritor_arquivo, &read_fds);
        maior_descritor_arquivo = socket_descritor_arquivo;

        // Adicionar sockets de clientes à lista de descritores de arquivo
        for (int j = 0; j < QTD_MAX_CLIENTES; j++)
        {
            id_socket = clientes_aplicacao[j].cliente_sd;

            if (id_socket > 0)
                FD_SET(id_socket, &read_fds);

            if (id_socket > maior_descritor_arquivo)
                maior_descritor_arquivo = id_socket;
        }

        // Informa que o master receberá descritores de leitura e realiza o select
        // read_fds = master;
        select(maior_descritor_arquivo + 1, &read_fds, NULL, NULL, NULL);

        for (int i = 0; i <= maior_descritor_arquivo; i++)
        {
            // Testa se o file descriptor esta no cesto

            if (FD_ISSET(i, &read_fds))
            {
                // Checa o file descriptor e o socket

                if (i == socket_descritor_arquivo)
                {
                    // Conexao, adiciona o socket descriptor no cesto
                    novo_descritor_arquivo = accept(socket_descritor_arquivo, (struct sockaddr *)&endereco_remoto_socket, &tamanho_endereco);
                    FD_SET(novo_descritor_arquivo, &read_fds);

                    char nome[MAX_STR_SIZE];

                    char boas_vindas[] = "Seja bem-vindo(a) as salas de bate-papo, digite seu nome por favor\n";
                    send(novo_descritor_arquivo, boas_vindas, strlen(boas_vindas), 0);

                    // Recebe nome do usuario
                    int tam_nome = recv(novo_descritor_arquivo, nome, MAX_STR_SIZE, 0);
                    tam_nome -= 2;

                    validar_nome(novo_descritor_arquivo, nome, &tam_nome);
                    nome[tam_nome] = '\0';
                    // cria nova instancia do novo cliente
                    cliente novo_cliente;
                    novo_cliente.cliente_sd = novo_descritor_arquivo;
                    novo_cliente.ativo = false;
                    novo_cliente.sala = -1;
                    novo_cliente.id_cliente = qtd_clientes;
                    strncpy(novo_cliente.nome, nome, tam_nome);

                    novo_cliente.nome[tam_nome] = '\0';
                    clientes_aplicacao[qtd_clientes] = novo_cliente; // adiciona o novo cliente ao vetor de clientes conectados a aplicacao
                    qtd_clientes++;

                    for (int aux = 0; aux < qtd_clientes; aux++)
                    {
                        printf("Nome: %s\n", clientes_aplicacao[aux].nome);
                    }

                    send(novo_descritor_arquivo, enter_continuar, strlen(enter_continuar), 0);
                    printf("%s acabou de entrar na sala de espera\n", nome);

                    

                    menu(novo_descritor_arquivo, novo_cliente);

                    // De qualquer forma insere ele na sala nova ou existente
                    // entrar_na_sala(novo_descritor_arquivo, sala, nome, tam_nome);

                    // Se o valor do socket_descritor_arquivo for maior que o atual (mais coisas no cesto)
                    // atualiza esse valor para as proximas iteracoes do loop
                    if (novo_descritor_arquivo > maior_descritor_arquivo)
                        maior_descritor_arquivo = novo_descritor_arquivo;
                }
                else
                {
                }
            }
        }

        for (int aux = 0; aux < qtd_clientes; aux++)
        {
            printf("%d %s\n", clientes_aplicacao[aux].cliente_sd, clientes_aplicacao[aux].nome);
            id_socket = clientes_aplicacao[aux].cliente_sd;

            if (FD_ISSET(id_socket, &read_fds))
            {

                if (clientes_aplicacao[aux].sala == -1)
                {
                    // cliente nao esta em nenhuma sala
                    printf("nao esta em nenhuma sala\n");

                    menu(clientes_aplicacao[aux].cliente_sd, clientes_aplicacao[aux]);
                }
                else
                {
                    // Se nao for o descritor do id_socket, cria um buffer, recebe a mensagem
                    // e a retransmite por todos os id_sockets conectados
                    printf("else\n");
                    memset(&buffer, 0, sizeof(buffer));
                    num_bytes = recv(id_socket, buffer, sizeof(buffer), 0);

                    // Encontra a sala que o descritor do socker se encontra
                    int sala_id;
                    for (sala_id = 0; sala_id < MAX_SALAS; sala_id++)
                        if (FD_ISSET(id_socket, &salas[sala_id].sala_fd))
                            break;

                    // Encontra o id do cliente na sala atual do mesmo
                    int cliente_id;
                    for (cliente_id = 0; cliente_id < salas[sala_id].limite; cliente_id++)
                        if (salas[sala_id].clientes[cliente_id].cliente_sd == id_socket)
                            break;

                    // Desconexao forcada
                    if (num_bytes == 0)
                    {
                        printf("Desconectando forcadamente o descritor %d\n", id_socket);
                        sair_da_sala(id_socket, salas, clientes_aplicacao, qtd_clientes);
                    }

                    // // Caso o primeiro caracter da mensagem seja uma / executa comando
                    // if (buffer[0] == '/')
                    //     executa_comando(i, sala_id, cliente_id);
                    // // Caso não, encaminha a mensagem na sala
                    // else{
                    //     envia_msg(i, id_socket_descritor_arquivo, sala_id, cliente_id);
                    // }
                    envia_msg(id_socket, socket_descritor_arquivo, sala_id, cliente_id);
                }
            }
        }
    }

    return 0;
}

// A funcao retorna 1 se for possivel entrar na sala e 0 caso contrario
int validar_entrada(int sala, int socket)
{
    char excedeu_tamanho[] = "Os numeros das salas so vao de 1 a 100, digite novamente\n";
    char sala_inativa[] = "Esta sala esta inativa, digite o numero de uma sala ativa\n";
    char sala_lotada[] = "Esta sala esta lotada\n";
    int invalido = 1;
    char buffer[MAX_STR_SIZE];

    // validar se a sala existe
    if (sala > MAX_SALAS)
    {
        send(socket, excedeu_tamanho, strlen(excedeu_tamanho), 0);
        return 0;
        // recv(novo_descritor_arquivo, buffer, MAX_STR_SIZE, 0);
        // *sala = atoi(buffer);
    }
    else if (salas[sala].ativo == false)
    {
        send(socket, sala_inativa, strlen(sala_inativa), 0);
        return 0;
        // recv(novo_descritor_arquivo, buffer, MAX_STR_SIZE, 0);
        // *sala = atoi(buffer);
    }
    else if (salas[sala].limite == salas[sala].quantidade_clientes)
    {
        send(socket, sala_lotada, strlen(sala_lotada), 0);
        return 0;
        // recv(novo_descritor_arquivo, buffer, MAX_STR_SIZE, 0);
        // *sala = atoi(buffer);
    }
    else
    {
        return 1;
    }
}

void menu(int socket, cliente cliente)
{    
    int invalido = 1, sala, limite, escolha;
    bool tem_sala_ativa = false;
    char opcao_invalida[] = "Escolha invalida, digite novamente\n";
    char opcoes[] = "\n[1] Listar salas disponiveis\n[2] Entrar em sala de bate-papo\n[3] Criar sala de bate-papo\n[4] Listar participantes de uma sala\n[5] Desconectar\n";
    char entrada[2];
    // for(int aux = 0; aux < MAX_STR_SIZE; aux++){
    //     buffer[aux] = '';
    // }
    while (invalido)
    {

        char enter[3];
        recv(socket, enter, sizeof(enter), 0);
        send(socket, opcoes, strlen(opcoes), 0);

        recv(socket, entrada, MAX_STR_SIZE, 0); // Lendo opcao do menu

        escolha = atoi(entrada);

        switch (escolha)
        {
        case LISTAR_SALAS:
            invalido = 0;
            lista_salas();
            send(novo_descritor_arquivo, enter_continuar, strlen(enter_continuar), 0);
            break;

        case ENTRAR_SALA:
            invalido = 0;
            for (int sala = 0; sala < MAX_SALAS; sala++)
            {
                if (salas[sala].ativo == true)
                {
                    tem_sala_ativa = true;
                    break;
                }
            }
            if (tem_sala_ativa == false)
            {
                send(socket, "Nao ha salas ativas, crie uma sala\n", strlen("Nao ha salas ativas, crie uma sala\n"), 0);
                break;
            }
            send(socket, "Digite o numero da sala:\n", strlen("Digite o numero da sala:\n"), 0);
            recv(socket, buffer, MAX_STR_SIZE, 0);
            sala = atoi(buffer);
            if (validar_entrada(sala, socket))
            {
                entrar_na_sala(socket, sala, cliente);
            }

            break;

        case NOVA_SALA:
            invalido = 0;
            // informa limite da sala
            send(socket, "Digite o tamanho da sala:\n", strlen("Digite o tamanho da sala:\n"), 0);
            recv(socket, buffer, MAX_STR_SIZE, 0);
            limite = atoi(buffer);
            sala = cria_sala(limite, socket);
            send(novo_descritor_arquivo, enter_continuar, strlen(enter_continuar), 0);
            break;
        case LISTAR_PARTICIPANTES_SALA:
            invalido = 0;
            for (int sala = 0; sala < MAX_SALAS; sala++)
            {
                if (salas[sala].ativo == true)
                {
                    tem_sala_ativa = true;
                    break;
                }
            }
            if(tem_sala_ativa == false){
                send(socket, "Nao ha salas ativas para serem listadas\n", strlen("Nao ha salas ativas para serem listadas\n"), 0);
                break;
            }

            send(socket, "Digite o numero da sala:\n", strlen("Digite o numero da sala:\n"), 0);
            recv(socket, buffer, MAX_STR_SIZE, 0);
            sala = atoi(buffer);
            lista_participantes_sala(socket, sala);
            break;

        case DESCONECTAR:
            invalido = 0;
            desconectar_cliente(socket, salas, clientes_aplicacao, qtd_clientes);
            break;
        default:

            send(socket, opcao_invalida, strlen(opcao_invalida), 0);
            break;
        }
    }

    send(socket, "\n", strlen("\n"), 0);
}

void validar_nome(int socket, char *nome, int *tam_nome)
{

    char mensagem_erro[] = "O nome deve ter pelo menos tres letras, digite novamente.\n";

    while (*tam_nome <= TAM_MIN_NOME)
    {

        send(socket, mensagem_erro, strlen(mensagem_erro), 0);

        *tam_nome = recv(socket, nome, MAX_STR_SIZE, 0);
        *tam_nome -= 2;
    }
}

void prepara_servidor()
{
    // Faz a Limpeza dos sets master
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    // Inicializacao do servidor, zerando valores de todas as salas
    for (int i = 0; i < MAX_SALAS; i++)
    {
        FD_ZERO(&salas[i].sala_fd);
        salas[i].limite = 0;
        salas[i].quantidade_clientes = 0;
        salas[i].ativo = false;
    }
}

void sair_da_sala(int socket, sala salas[], cliente clientes[], int total_clientes)
{
    char response[250];
    int client_index = -1;
    for (int i = 0; i < total_clientes; i++)
    {
        if (clientes[i].cliente_sd == socket)
        {
            client_index = i;
            break;
        }
    }

    if (client_index != -1)
    {
        int sala_id = clientes[client_index].sala;
        for(int i = 0; i < salas[sala_id].quantidade_clientes; i++){
            if(salas[sala_id].clientes[i].cliente_sd == socket){
                salas[sala_id].clientes[i].ativo = false;
                break;
            }
        }

        clientes[client_index].sala = -1;

        salas[sala_id].quantidade_clientes--;
        sprintf(response, "Voce saiu da sala %d.\n", sala_id);
        send(socket, response, strlen(response), 0);
        send(novo_descritor_arquivo, "Pressione enter para continuar!\n", strlen("Pressione enter para continuar!\n"), 0);
        menu(socket, clientes[client_index]);
    }
    else
    {
        sprintf(response, "Voce não está em uma sala.\n");
        send(socket, response, strlen(response), 0);
    }
}

void lista_salas()
{
    send(novo_descritor_arquivo, "\n======= Salas Criadas =======\n", strlen("\n======= Salas Criadas =======\n"), 0);
    char sala_char[3];
    char limite_char[3];
    char qtd_clientes_char[3];

    send(novo_descritor_arquivo, "\n Lista de salas ativas: \n", strlen("\nLista de salas ativas: \n"), 0);
    send(novo_descritor_arquivo, "\n\n", strlen("\n\n"), 0);
    for (int sala = 0; sala < MAX_SALAS; sala++)
    {
        if (salas[sala].ativo == true)
        {
            char *msg = (char *)malloc(sizeof(char) * 53);
            int limite = salas[sala].limite;
            int qtd_clientes = salas[sala].quantidade_clientes;
            strcat(msg, "[Sala ");
            sprintf(sala_char, "%d", sala);
            sprintf(limite_char, "%d", limite);
            sprintf(qtd_clientes_char, "%d", qtd_clientes);
            printf("\nLista de salas ativas: \n");
            printf("Sala %d: ativada, capacidade maxima de usuarios: %d\n", sala, limite);
            strcat(msg, sala_char);
            strcat(msg, "]: ativa (");
            strcat(msg, qtd_clientes_char);
            strcat(msg, "/");
            strcat(msg, limite_char);
            strcat(msg, ")\n");
            send(novo_descritor_arquivo, msg, strlen(msg), 0);
        }
    }
}

int cria_sala(int limite, int socket)
{
    // Para criar uma sala, deve-se encontrar a primeira sala
    // vazia (ativo = 0) setar como ativa e atualizar seu limite
    int sala;
    char mensagem[15] = "Sala ";
    char sala_char[3];
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

    sprintf(sala_char, "%d", sala);
    strcat(mensagem, sala_char);
    strcat(mensagem, " criada\n");

    printf("Sala %d: ativada, capacidade maxima de %d usuarios.\n", sala, limite);
    send(socket, mensagem, strlen(mensagem), 0);
    return sala;
}

void envia_msg(int socket_descritor_arquivo, int server_sd, int sala_id, int cliente_id)
{
    if(strncmp(buffer, "exit", 4) == 0) {
        sair_da_sala(socket_descritor_arquivo, salas, clientes_aplicacao, qtd_clientes);
    }

    printf("Sala %d: mensagem '%s' enviada por %s.\n", sala_id, buffer, salas[sala_id].clientes[cliente_id].nome);
    // Descriptor para cada file
    for (int j = 0; j <= maior_descritor_arquivo; j++)
    {
        // checa se ele esta no cesto do master
        if (FD_ISSET(j, &salas[sala_id].sala_fd))
        {
            // e checa se o valor nao e o descritor de si mesmo
            if (j != socket_descritor_arquivo && j != server_sd)
            {
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

void entrar_na_sala(int socket_descritor_arquivo, int sala_id, cliente cliente)
{
    char *msg = (char *)malloc(25 * sizeof(char));
    char sala_char[3];
    sprintf(sala_char, "%d", sala_id);
    strcat(msg, "Voce esta na sala ");
    strcat(msg, sala_char);
    send(socket_descritor_arquivo, msg, strlen(msg), 0);
    send(socket_descritor_arquivo, "  - Digite 'exit' para sair da sala\n", strlen("  - Digite 'exit' para sair da sala\n"), 0);
    printf("Sala %d: file descriptor %d entrando.\n", sala_id, socket_descritor_arquivo);
    // Para inserir na sala, deve-se aumentar a quantidade_clientes, adicionar
    // o descritor no cesto da sala, encontra uma posição na sala
    // que esteja vazia (cliente.ativo = 0) e insere seus atributos
    // socket descriptor, ativo e nome
    FD_SET(socket_descritor_arquivo, &salas[sala_id].sala_fd);
    salas[sala_id].quantidade_clientes++;


    for (int i = 0; i < salas[sala_id].limite; i++)
    {
        if (salas[sala_id].clientes[i].ativo == false)
        {
            salas[sala_id].clientes[i].cliente_sd = socket_descritor_arquivo;
            salas[sala_id].clientes[i].ativo = 1;
            salas[sala_id].clientes[i].id_cliente = cliente.id_cliente;
            strncpy(salas[sala_id].clientes[i].nome, cliente.nome, strlen(cliente.nome));

            // procura o cliente que acabou de entrar na sala e atualiza o numero da sala que ele esta
            for (int aux = 0; aux < qtd_clientes; aux++)
            {
                if (clientes_aplicacao[aux].id_cliente == cliente.id_cliente)
                {
                    salas[sala_id].clientes[i].sala = sala_id;
                    clientes_aplicacao[aux].sala = sala_id;
                }
            }
            break;
        }
    }
}

void desconectar_cliente(int socket, sala salas[], cliente clientes[], int total_clientes)
{
    char msg1[21];
    char msg2[32];
    int clientIndex = -1;
    for (int i = 0; i < total_clientes; i++)
    {
        if (clientes[i].cliente_sd == socket)
        {
            clientIndex = i;
            break;
        }
    }

    if (clientIndex != -1)
    {
        // Remover cliente da lista de clientes
        for (int j = clientIndex; j < total_clientes - 1; j++)
        {
            clientes[j] = clientes[j + 1];
        }
        total_clientes--;

        // Se o cliente estiver em uma sala, removê-lo da sala
        // if (clientes[clientIndex].sala != -1)
        // {
        //     leave_room(socket, rooms, totalRooms, clients, totalClients);
        // }

        // Fechar o socket e marcar como 0 na lista de sockets de clientes
        sprintf(msg1, "Voce saiu do chat.\n");
        send(socket, msg1, strlen(msg1), 0);

        for (int j = 0; j < QTD_MAX_CLIENTES; j++)
        {
            if (socket == clientes[j].cliente_sd)
            {
                clientes[j].cliente_sd = 0;
                break;
            }
        }
    }
    else
    {
        sprintf(msg2, "Erro ao desconectar o cliente.\n");
        send(socket, msg2, strlen(msg2), 0);
    }
    close(socket);
}

void lista_participantes_sala(int socket, int sala_id){
    if(salas[sala_id].ativo == false){
        send(socket, "Sala nao existe.\n", strlen("Sala nao existe.\n"), 0);
    } else{
        for (int i = 0; i < salas[sala_id].limite; i++)
        {
            if (salas[sala_id].clientes[i].ativo == true)
            {
                send(socket, "Nome: ", strlen("Nome: "), 0);
                send(socket, salas[sala_id].clientes[i].nome, strlen(salas[sala_id].clientes[i].nome), 0);
                send(socket, "\n", strlen("\n"), 0);
            }
        }
    }
    send(socket, enter_continuar, strlen(enter_continuar), 0);
}