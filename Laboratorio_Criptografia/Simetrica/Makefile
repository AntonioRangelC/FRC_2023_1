#  Grupo:
#  Antonio Igor Carvalho - 180030264
#  Antonio Rangel Chaves - 180098021
#  Douglas da Silva Monteles - 190012200
#  Enzo Gabriel Guedes Queiroz Saraiva - 160119006

# Execute: make f-server para executar o servidor

f-server:
	echo ""
	rm -f ./server
	gcc server.c -o server -lssl -lcrypto -Wall
	clear
	./server

# Execute: make f-client para executar o cliente

f-client:
	rm -f ./client
	gcc client.c -o client -lssl -lcrypto -Wall
	clear
	./client