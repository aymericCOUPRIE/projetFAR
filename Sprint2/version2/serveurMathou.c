#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#define TMAX 65000 //taille maximum des paquets (en octets)
#define nbrClientMax 200
int tabdSC[200] ; //tableau de 200 sockets
char pseudos[200][10];
int nbrClient;
pthread_t thread[nbrClientMax];

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

	int nb_octets;
	int rec;
	int nb_recu = 0;

	//Réception de la taille du message à recevoir
	printf("premiere reception \n");
	rec = recv(sockE, &nb_octets, sizeof(int), 0);
	if (rec == -1){
		perror("Erreur 1ere reception\n");
		return -1;
	}
	if (rec == 0){
		perror("Socket fermée\n");
		return 0;
	}

	//Boucle pour recevoir toutes les portions du message
	while(nb_recu < nb_octets){

	    printf("deuxieme reception \n");

		rec = recv(sockE, message, nb_octets*sizeof(char), 0);
		if (rec == -1){
			perror("Erreur 2eme reception\n");
			return -1;
		}
		if (rec == 0) {
			perror("Socket fermée\n");
			return 0;
		}
		nb_recu += rec;
	}

	return 1;
}

//fonction pour récupérer le pseudo
void recuperer_pseudo (char *pseudo, int i){
    if (reception(tabdSC[i], pseudo) != 1) {
        perror("err: recupération pseudo");
        strcpy(pseudos[i], "inconnu");
    }
}

//fonction pour la transmission des messages
void * transmission (void * args){

    printf("je suis dans transmission \n");

    int *recupVoid = args;
    int i = *recupVoid;
    char msg[TMAX] = "";
    int fin = 0;

    printf("la valeur de i = %d \n", i);

    while(fin != 1){
        if (reception(tabdSC[i], msg) != 1){
            perror("err : recep dans trans");
            pthread_exit(NULL);
        }

        if(strcmp(msg,"fin\n") == 0) {
			fin = 1;
		}

		//on met le pseudo au debut du message avant de le transmettre
		char newMsg[TMAX];
		strcpy(newMsg, pseudos[i]);
		strcat(newMsg, ":");
		strcat(newMsg, msg);

        int j = 0;
        while (j < nbrClientMax){
            if (i != j && tabdSC[j] != -1){
            //on transmet son message à l'autre client
                if (envoie(tabdSC[j], newMsg) != 1){
	                perror("err : env dans trans");
	                pthread_exit(NULL);
                }
            }
        }
    }
    printf("je sors de la boucle transmission serveur \n");
    pthread_exit(NULL);
}


//fonction de connexion
void * connexion (void * args){

    int * pointeur = args;
    int dS = *pointeur;

    struct sockaddr_in aC;
    socklen_t lg = sizeof(struct sockaddr_in);

    int i = 0;

    while (nbrClient < nbrClientMax) {

            tabdSC[i] = accept(dS, (struct sockaddr*) &aC,&lg);
            if(tabdSC[i] == -1){
                perror("Erreur accept client \n");
                pthread_exit(NULL);
            }

            printf ("j'ai accepté un nouveau client \n");

            recuperer_pseudo (pseudos[i], i);
            printf("client numéro %d connecté avec le pseudo %s \n", i+1, pseudos[i]);

            printf("i dans transmission = %d \n", i);

            if (pthread_create(&thread[i], NULL, transmission, &i ) != 0){
                perror("creation de thread[i]");
                pthread_exit(NULL);
            } else {
                nbrClient = nbrClient + 1;
                i = i + 1;
                printf("nombre client = %d \n", nbrClient);
            }
    }

    pthread_join(thread[0], NULL);

}

int main(int argc, char* argv[]){

    //initialisation du tabdSC à -1;
    int i = 0;
    while (i < nbrClientMax){
        tabdSC[i] = -1;
        i = i+1;
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

	nbrClient = 0;

	while(1) {
		printf("En attente de connexion des clients\n");

		//Connexion client
		pthread_t threadConnexion;
		if (pthread_create (&threadConnexion, NULL, connexion, &dS) != 0){
		    perror("erreur création thread");
		    return -1;
		}

        //attente de la fin des threads
        pthread_join (threadConnexion, NULL);

        //on ferme les threads et les sockets des clients
        for (long i = 0; i < nbrClient; i++) {
        	pthread_cancel(thread[i]); //ferme le thread du client i
        	close(tabdSC[i]);		    //ferme la socket du client i
        }
        pthread_cancel(threadConnexion); //on ferme la socket de connexion
        printf("Fin de la discution \n");

        //on prévient que l'échange est terminé et on ferme les sockets des clients
		printf("Fin de l'échange\n");

	}
	close(dS);
	return 0;

}