all: client serveur clean
CC=gcc

client: client.o
	$(CC) -o client client.o -pthread
client.o: client.c
	gcc -o client.o -c client.c
 
serveur: serveur.o
	$(CC) -o serveur serveur.o -pthread
serveur.o: serveur.c
	gcc -o serveur.o -c serveur.c

clean:
	rm -rf *.o
