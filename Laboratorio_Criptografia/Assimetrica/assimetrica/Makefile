#  Grupo:
#  Antonio Igor Carvalho - 180030264
#  Antonio Rangel Chaves - 180098021
#  Douglas da Silva Monteles - 190012200
#  Enzo Gabriel Guedes Queiroz Saraiva - 160119006

# Execute: make f-gerarsa primeiro para gerar as chaves
f-gerarsa:
	rm -f ./gerarsa
	gcc -o gerarsa gerarsa.c -lssl -lcrypto
	clear
	./gerarsa -p

# Execute: make f-server para executar o servidor
f-server:
	rm -f ./servidor
	gcc servidor.c -o servidor -lssl -lcrypto -Wall
	clear
	./servidor

# Execute: make f-client para executar o cliente
f-client:
	rm -f ./cliente
	gcc cliente.c -o cliente -lssl -lcrypto -Wall
	clear
	./cliente