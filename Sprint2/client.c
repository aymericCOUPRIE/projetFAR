#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#define TMAX 65000 //taille maximum des paquets (en octets)

//fonction pour envoyer un message
void *envoie(void *SockEv/*int SockE, char* message*/) {

    char msg[TMAX] = "";

	int taille_msg;
	int mes;

	int *SockE = SockEv;


	while (1){
	    printf("Votre message : ");
        fgets(msg, TMAX, stdin); //saisie clavier du message
		taille_msg = (strlen(msg)+1)*sizeof(char);
    	//Envoi de la taille du message
    	mes = send(*SockE, &taille_msg, sizeof(int), 0);
    	if (mes == -1){
    		perror("Erreur envoie\n");
    		exit(-1);
    	}
    	if (mes==0){
    		perror("Socket fermée\n");
    		exit (0);
    	}

    	//Envoi du message
    	mes = send(*SockE, msg, strlen(msg)+1, 0);
    	if (mes == -1){
    		perror("Erreur envoie\n");
    		exit(-1);
    	}
    	if (mes == 0){
    		perror("Aucun envoie\n");
    		exit(0);
    	}
	}

}

//fonction pour recevoir un message
void *reception(void* sockEv /*int sockE, char* message*/){

	int taille_msg;
	int rec;
	int taille_rec = 0;
	char msg[TMAX] = "";

	int *sockE = sockEv;

	while (1) {
	    //Réception de la taille du message à recevoir
    	rec = recv(*sockE, &taille_msg, sizeof(int), 0);
    	if (rec == -1){
    		perror("Erreur reception\n");
    		exit (-1);
    	} else if (rec == 0) {
    		perror("Aucun message recu\n");
    		exit (0);
    	}

    	//Boucle pour recevoir toutes les portions du message
    	while(taille_rec < taille_msg){
    		rec = recv(*sockE, msg, taille_msg*sizeof(char), 0);
    		if (rec == -1){
    			perror("Erreur reception\n");
    			exit (-1);
    		}
    		if (rec==0){
    			perror("Socket fermée\n");
    			exit (0);
    		}
    		taille_rec += rec;
    	}
    	printf("Message reçu : %s\n", msg);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

//connexion au serveur
	socklen_t lgA = sizeof(aS);
	res = connect(dS, (struct sockaddr *) &aS, lgA);
	if (res == -1) {
		perror("Erreur connect\n");
		return -1;
	}

	char msg[TMAX] = "";

//création des deux threads
    pthread_t threadEnvoi;
    pthread_t threadReception;

    if (pthread_create(&threadEnvoi, NULL, envoie, &dS) != 0){
        perror("erreur creation thread\n");
        return -1;
    }

    if (pthread_create(&threadReception, NULL, reception, &dS) != 0){
        perror("erreur creation thread\n");
        return -1;
    }

//attente de la fin des threads

    pthread_join (threadEnvoi, NULL);
    pthread_join (threadReception, NULL);

    return 0;
}