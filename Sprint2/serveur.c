#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#define TMAX 65000 //taille maximum des paquets (en octets)

//fonction pour envoyer un message
int *envoie(int SockE, char* message) {
	int taille_msg = (strlen(message)+1)*sizeof(char);
	int mes;

	//Envoi de la taille du message
	mes = send(SockE, &taille_msg, sizeof(int), 0);
	if (mes == -1){
		perror("Erreur envoie\n");
		return -1;
	}
	if (mes==0){
		perror("Socket fermée\n");
		return 0;
	}

	//Envoi du message
	mes = send(SockE, message, strlen(message)+1, 0);
	if (mes == -1){
		perror("Erreur envoie\n");
		return -1;
	}
	if (mes == 0){
		perror("Aucun envoie\n");
		return 0;
	}
	return 1;
}

//fonction pour recevoir un message
int *reception(int sockE, char* message){
	int taille_msg;
	int rec;
	int taille_rec = 0;

	//Réception de la taille du message à recevoir
	rec = recv(sockE, &taille_msg, sizeof(int), 0);
	if (rec == -1){
		perror("Erreur reception\n");
		return -1;
	} else if (rec == 0) {
		perror("Aucun message recu\n");
		return 0;
	}

	//Boucle pour recevoir toutes les portions du message
	while(taille_rec < taille_msg){
		rec = recv(sockE, message, taille_msg*sizeof(char), 0);
		if (rec == -1){
			perror("Erreur reception\n");
			return -1;
		}
		if (rec==0){
			perror("Socket fermée\n");
			return 0;
		}
		taille_rec += rec;
	}
	printf("Message reçu : %s\n", message);
	return 1;
}