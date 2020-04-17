#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#define TMAX 65000 //taille maximum des paquets (en octets)
int tabdSC[10] ;

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
	printf("envoie d'un message de taille : %d \n", taille_msg);

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
	printf("message %s envoyé \n", message);

	return 1;
}

//fonction pour recevoir un message
int reception(int sockE, char* message){
	int nb_octets;
	int rec;
	int nb_recu = 0;

	//Réception de la taille du message à recevoir
	rec = recv(sockE, &nb_octets, sizeof(int), 0);
	if (rec == -1){
		perror("Erreur reception\n");
		return -1;
	}
	if (rec == 0){
		perror("Socket fermée\n");
		return 0;
	}
    printf("reception d'un message de taille %d \n", nb_octets);

	//Boucle pour recevoir toutes les portions du message
	int i = 0;
	while(nb_recu < nb_octets){

	    i = i+1;
	    printf ("je passe dans la boucle pour la %d eme fois \n", i);

		rec = recv(sockE, message, nb_octets*sizeof(char), 0);
		if (rec == -1){
			perror("Erreur reception\n");
			return -1;
		}
		if (rec == 0) {
			perror("Socket fermée\n");
			return 0;
		}
		nb_recu += rec;
	}

	printf("message %s reçu \n", message);

	return 1;
}

void * transmission (void * args){

    int *sens = args;
    int sensTest = *sens;
    int sockReception;
    int sockEnvoi;
    if (sensTest == 0){
        sockReception = tabdSC[0];
        sockEnvoi = tabdSC[1];
    } else {
        sockReception = tabdSC[1];
        sockEnvoi = tabdSC[0];
    }

    char msg[TMAX] = "";
    int fin = 0;
    while(fin != 1){
        if (reception(sockReception, msg) != 1){
            perror("err : recep dans trans");
            pthread_exit(NULL);
        }
        printf("reception de %s \n", msg);
        if(strcmp(msg,"fin\n") == 0) {
			fin = 1;
		} else {
		//on transmet son message à l'autre client
		    printf("envoi du message %s \n", msg);
			if (envoie(sockEnvoi, msg) != 1){
			    perror("err : env dans trans");
			    pthread_exit(NULL);
			}
		}
		printf("valeur de fin : %d \n", fin);
    }
    pthread_exit(NULL);
}

int main(int argc, char* argv[]){

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

	while(1) {
		printf("En attente de connexion des 2 clients\n");

		//Connexion client 1
		tabdSC[0] = accept(dS, (struct sockaddr*) &aC,&lg);
        if(tabdSC[0] == -1){
            perror("Erreur accept client 1\n");
            exit(0);
        }
        printf("Client 1 : Connexion OK\n");

        /*res = envoie(tabdSC[0], "Vous êtes le Client 1 ... en attente de Client 2 \n");
		if (res != 1){
			return -1;
		}*/

		//Connexion client 2
		tabdSC[1] = accept(dS, (struct sockaddr*) &aC,&lg);
        if(tabdSC[1] == -1){
            perror("Erreur accept client 2\n");
            exit(0);
        }
        printf("Client 2 : Connexion OK\n");

        /*res = envoie(tabdSC[1], "Vous êtes le  Client 2.\n");
		if (res != 1){
			return -1;
		}

		//Début échange, on prévient le client 1
		res = envoie(tabdSC[0], "Vous pouvez envoyer un message");
		if(res != 1){
			return -1;
		}*/

		int sens0 = 0;
		int sens1 = 1;

		//création des deux threads
        pthread_t thread0v1;
        pthread_t thread1v0;

        printf("initialisation des threads \n");

        if (pthread_create(&thread0v1, NULL, transmission, &sens0) != 0){
            perror("erreur creation thread\n");
            return -1;
        }

        printf("lancement du premier thread \n");

        if (pthread_create(&thread1v0, NULL, transmission, &sens1) != 0){
            perror("erreur creation thread\n");
            return -1;
        }

        printf("lancement du second thread \n");

        //attente de la fin des threads
        pthread_join (thread0v1, NULL);
        pthread_join (thread1v0, NULL);

		//while(fin != 1) {
			/*//Serveur recoit le message du client 1
			res = reception(dSC1, msg);
			if (res != 1) {
				return -1;
			}

			//Vérification si fin
			if(strcmp(msg,"fin\n") == 0){ //Si le message du client est "fin" alors l'échange est fini
				fin = 1;
			} else {
				//on transmet le message du client 1 au client 2
				res = envoie(dSC2, msg);
				if (res!=1){
					return -1;
				}

				//on reçoit le message du deuxième client
				res = reception(dSC2, msg);
				if (res!=1){
					return -1;
				}

				//on vérifie qu'il ne veut pas mettre fin à la conversation
				if(strcmp(msg,"fin\n") == 0) {
					fin = 1;
				} else {
					//on transmet son message à l'autre client
					res = envoie(dSC1, msg);
					if (res!=1){
						return -1;
					}
				}
			}
		}*/

        //on prévient que l'échange est terminé et on ferme les sockets des clients
		printf("Fin de l'échange\n");
		close(tabdSC[0]);
		close(tabdSC[1]);
	}
	close(dS);
	return 0;

}