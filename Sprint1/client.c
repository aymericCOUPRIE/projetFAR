#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#define TMAX 65000 //taille maximum des paquets (en octets)

//fonction pour envoyer un message
int envoie(int SockE, char* message) {
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
int reception(int sockE, char* message){
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


int main(int argc, char* argv[]){ 

	//Vérification des arguments
	if(argc != 3) {
		printf("paramètres: ./client Adresse_IP  Numéro_de_port\n");
		exit(0);
	}

	//definition de la socket
	int dS;
	dS = socket(PF_INET, SOCK_STREAM, 0);
	if (dS == -1) {
		perror("Erreur socket\n");
		return -1;
	}

//structure de la socket
	struct sockaddr_in aS;
	aS.sin_family = AF_INET;
	aS.sin_port = htons(atoi(argv[2])); //on récupère le port dans les arguments du terminal
	
	int res;
	res = inet_pton(AF_INET, argv[1], &(aS.sin_addr)); //on converti l'adresse IP décimale en binaire et on la stock dans aS.sin_addr pour ensuite désigner le destinataire
	if (res<=0) {
		perror("Erreur inet_pton\n");
		return -1;
	}


	socklen_t lgA = sizeof(aS);
	res = connect(dS, (struct sockaddr *) &aS, lgA);
	if (res == -1) {
		perror("Erreur connect\n");
		return -1;
	}

	char msg[TMAX] = "";

	res = reception(dS, msg);
	if(res != 1){
		return -1;
	}

	while(1) {
//on reçoit un message
		res = reception(dS, msg);
		if(res == -1) {
			perror("Erreur reception");
			exit(0);
		} else if (res == 0){
			printf("Fin échange\n");
			return -1;
		}

		if(strcmp(msg,"Vous êtes le  Client 2.\n") !=0 ) { //pour que le client 1 envoie un message en premier 
			printf("Votre message : ");
			fgets(msg, TMAX, stdin); //saisie clavier du message
			res = envoie(dS, msg); //envoie du message
			if(res != 1) {
				return -1;
			}
		}
	}
	return 0;
}