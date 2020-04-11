#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#define TMAX 65000

int envoie(int SockE, char* mot) {
	int taille = (strlen(mot)+1)*sizeof(char);
	int mes;

	//Envoi de la taille du message
	mes = send(SockE, &taille, sizeof(int), 0);
	if (mes<0){
		perror("Erreur envoie\n");
		return -1;
	}
	if (mes==0){
		//perror("Socket fermée\n");
		return 0;
	}

	//Envoi du message
	mes = send(SockE, mot, strlen(mot)+1, 0);
	if (mes < 0){
		perror("Erreur envoie\n");
		return -1;
	}
	if (mes == 0){
		//perror("Socket fermée\n");
		return 0;
	}
	return 1;
}


int reception(int sockE, char* mot){
	int nb_octets;
	int rec;
	int nb_recu = 0;

	//Réception de la taille du message à recevoir
	rec = recv(sockE, &nb_octets, sizeof(int), 0);
	if (rec<0){
		perror("Erreur reception\n");
		return -1;
	}
	if (rec==0){
		//perror("Socket fermée\n");
		return 0;
	}

	//Boucle pour recevoir toutes les portions du message
	while(nb_recu<nb_octets){
		rec = recv(sockE, mot, nb_octets*sizeof(char), 0);
		if (rec<0){
			perror("Erreur reception\n");
			return -1;
		}
		if (rec==0){
			//perror("Socket fermée\n");
			return 0;
		}
		nb_recu += rec;
	}
	printf("Message reçu : %s\n", mot);
	return 1;
}


int main(int argc, char* argv[]){ 

	//Vérification des arguments
	if(argc =! 3) {
		printf("Il faut 2 paramètres :\n\t1 : Adresse IP\n\t2: Numéro de port\n");
		exit(0);
	}

	//definition de la socket
	int dS;
	dS = socket(PF_INET, SOCK_STREAM, 0);
	if (dS < 0) {
		perror("Erreur socket\n");
		return -1;
	}

	struct sockaddr_in aS;
	aS.sin_family = AF_INET;
	aS.sin_port = htons(atoi(argv[2]));
	
	int res;
	res = inet_pton(AF_INET, argv[1], &(aS.sin_addr));
	if (res<=0) {
		perror("Erreur inet_pton\n");
		return -1;
	}


	socklen_t lgA = sizeof(aS);
	res = connect(dS, (struct sockaddr *) &aS, lgA);
	if (res < 0) {
		perror("Erreur connect\n");
		return -1;
	}

	char mot[TMAX] = "";

	res = reception(dS, mot);
	if(res != 1){
		return -1;
	}

	while(1) {
		res = reception(dS, mot);
		if(res == -1) {
			perror("Erreur reception");
			exit(0);
		} else if (res == 0){
			printf("Fin échange\n");
			return -1;
		}

		if(strcmp(mot,"Vous êtes : Client 2\n") !=0 ) {
			printf("Votre message : ");
			fgets(mot, TMAX, stdin);
			res = envoie(dS, mot);
			if(res != 1) {
				return -1;
			}
		}
	}
	return 0;
}