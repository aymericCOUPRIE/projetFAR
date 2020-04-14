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

if (argc != 2) {
        perror("paramètres : ./serveur  Numero_de_port");
        exit(0);
    }

    int dS;
	dS = socket(PF_INET, SOCK_STREAM, 0); //création de la socket
	if (dS == -1) {
		perror("Erreur création socket");
		exit(0);
	}

//structure de la socket
	struct sockaddr_in ad;
    ad.sin_family = AF_INET;
    ad.sin_addr.s_addr = INADDR_ANY;
    ad.sin_port = htons(atoi(argv[1])); //on récupère le port passé en paramètres, atoi permet de convertir une chaine de caractère en int


//nomage de la socket
    int res;
    res = bind(dS, (struct sockaddr*)&ad,sizeof(struct sockaddr_in));
    if(res == -1){
        perror("Erreur bind");
        exit(0);
	}

//on passe la socket en écoute
	res = listen(dS, 7);
	if(res == -1) {
		perror("Erreur listen");
		exit(0);
	}

	struct sockaddr_in aC;
	socklen_t lg = sizeof(struct sockaddr_in);

	return (0);

}