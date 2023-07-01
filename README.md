# FRC_2023_1

## Projeto de pesquisa - Criando ambientes virtuais de conversação com uso system call select()
### Integrantes do grupo

Antonio Rangel Chaves - 180098021
Antonio Ferreira de Castro Neto - 190044799
Enzo Gabriel Guedes Queiroz Saraiva - 160119006

### Como rodar?

#### Abra o seu terminal na pasta do projeto e entre na pasta Projeto de Pesquisa
```
cd Projeto de Pesquisa
```

#### Compile o arquivo principal.c
```
gcc -o principal principal.c
```

#### Rode o servidos
```
./principal [ip do servidor] [porta do servidor]
# Exemplo
./principal 127.0.0.1 4000
```

#### Abra outro terminal e digite o seu nome

```
telnet 127.0.0.1 4000

# Seja bem-vindo(a) as salas de bate-papo, digite seu nome por favor
Enzo

Pressione enter para continuar!
```

#### Interaja com o menu, inicialmente criando uma sala
```
[1] Listar salas disponiveis
[2] Entrar em sala de bate-papo
[3] Criar sala de bate-papo
[4] Listar participantes de uma sala
[5] Desconectar

3
Digite o tamanho da sala:
10
Sala 0 criada
Pressione enter para continuar!
```

#### Depois interaja para entrar na sala criada
```
[1] Listar salas disponiveis
[2] Entrar em sala de bate-papo
[3] Criar sala de bate-papo
[4] Listar participantes de uma sala
[5] Desconectar

2
Digite o numero da sala:
0
Voce esta na sala 0  - Digite 'exit' para sair da sala
```

#### Para a comunicação com o cliente na sala, abra outro terminal, conecte naquele servidor e liste as salas disponíveis
```
telnet 127.0.0.1 4000

# Seja bem-vindo(a) as salas de bate-papo, digite seu nome por favor
Antonio
Pressione enter para continuar!

[1] Listar salas disponiveis
[2] Entrar em sala de bate-papo
[3] Criar sala de bate-papo
[4] Listar participantes de uma sala
[5] Desconectar

1
======= Salas Criadas =======

 Lista de salas ativas: 

[Sala 0]: ativa (1/10)
Pressione enter para continuar!

```

#### Depois interaja para verificar os integrantes daquela sala
```
[1] Listar salas disponiveis
[2] Entrar em sala de bate-papo
[3] Criar sala de bate-papo
[4] Listar participantes de uma sala
[5] Desconectar

4
Digite o numero da sala:
0
Nome: Enzo
```

#### Depois interaja para entrar na sala
```
[1] Listar salas disponiveis
[2] Entrar em sala de bate-papo
[3] Criar sala de bate-papo
[4] Listar participantes de uma sala
[5] Desconectar

2
Digite o numero da sala:
0
Voce esta na sala 0  - Digite 'exit' para sair da sala
```
