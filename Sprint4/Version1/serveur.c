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

/*int tabSalon[200]; //tableau de distribution de salon
int tabdSC[200] ; //tableau de 200 sockets
int tabdSCFichier[200]; //200 sockets pour les fichiers
char pseudos[200][10];
int nbrClient;
int fin = 0;
int nbrSalonMax = 5;*/

pthread_t thread[nbrClientMax]; //pour les messages
pthread_t thread2[nbrClientMax]; //pour les fichiers

int fin = 0;
int nbrClient;
int nbrSalon;

struct client {
    int socketMes;
    int socketFil;
    char pseudo[10];
    int salon;
};

struct salon {
    char name[100];
    char desc[200];
    int nbrClientPres;
    int capacityMax;
};

struct client tabClient[200];
struct salon tabSalon[10];

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
    if (reception(tabClient[i].socketMes, pseudo) != 1) {
        perror("err: recupération pseudo");
        strcpy(tabClient[i].pseudo, "inconnu");
    }
}

int salon_valable (int salon){
    return (salon <= nbrSalon) && (tabSalon[salon].capacityMax != tabSalon[salon].nbrClientPres);
}

void recuperer_salon (int *salon, int i){
    printf("voici les salons disponibles\n");
    int j = 0;
    while (j<nbrSalon){
        printf("voici le salon n°%d %s : %s contenant %d personne et ayant pour capacité max %d \n",
        j,
        tabSalon[j].name,
        tabSalon[j].desc,
        tabSalon[j].nbrClientPres,
        tabSalon[j].capacityMax);
        j = j + 1;
    }
    do {
        int rec = recv(tabClient[i].socketMes, &tabClient[i].salon, sizeof(int), 0);
        if (rec == -1){
            perror("Erreur 1ere reception\n");
            exit(0);
        }
        if (rec == 0){
            perror("Socket fermée\n");
            exit(0);
        }
    } while (salon_valable(tabClient[i].salon) == 0);
    tabSalon[tabClient[i].salon].nbrClientPres = tabSalon[tabClient[i].salon].nbrClientPres + 1;

}

void * ajout_salon(int i){

    struct salon newSalon;
    reception(tabClient[i].socketMes, newSalon.name);
    reception(tabClient[i].socketMes, newSalon.desc);
    newSalon.nbrClientPres = 0;
    int capa = 0;
    int rec = recv(tabClient[i].socketMes, &capa, sizeof(int), 0);
    if (rec == -1){
        perror("Erreur 1ere reception\n");
        exit(0);
    }
    if (rec == 0){
        perror("Socket fermée\n");
        exit(0);
    }
    if (capa < 0 || capa > 200){
        capa = 10; //je choisi une capacité maximal par défault
    }
    newSalon.capacityMax = capa;

    tabSalon[nbrSalon] = newSalon;
    nbrSalon = nbrSalon + 1;
}

//fonction pour la transmission des messages
void * transmission (void * args){

    int *recupVoid = args;
    int i = *recupVoid; //Expéditeur message
    char msg[TMAX] = "";

    while(fin != 1){
        if (reception(tabClient[i].socketMes, msg) != 1){
            perror("err : recep dans trans");
            pthread_exit(NULL);
        }

        if(strcmp(msg,"fin\n") == 0) {
			fin = 1;
		}

		if(strcmp(msg,"salon\n") == 0) {
		    int old = tabClient[i].salon;
		    recuperer_salon(&tabClient[i].salon, i);
		    tabSalon[old].nbrClientPres = tabSalon[old].nbrClientPres - 1;
		    printf("%s est maintenant dans le salon %d \n", tabClient[i].pseudo, tabClient[i].salon);
		} else if(strcmp(msg, "ajout salon\n") == 0 && nbrSalon < 10) {
		    ajout_salon(i);
		} else {
		    int salon;
            salon = tabClient[i].salon;

            //on met le pseudo au debut du message avant de le transmettre
            char newMsg[TMAX];
            strcpy(newMsg, tabClient[i].pseudo);
            strcat(newMsg, ":");
            strcat(newMsg, msg);

            int j = 0;
            while (j < nbrClient){
                if (i != j && salon == tabClient[j].salon){
                //on transmet son message à l'autre client
                    if (envoie(tabClient[j].socketMes, newMsg) != 1){
                        perror("err : env dans trans");
                        pthread_exit(NULL);

                    }
                }
                j = j+1;
            }
            printf("le serveur a envoyé %s", newMsg);
		}
    }
    printf("je sors de la boucle transmission serveur \n");
    pthread_exit(NULL);
}

void* transmissionFichier(void* arg) {
    int res;
 
    int *recupVoid = arg;
    int i = *recupVoid; //Expéditeur fichier
    char msg[TMAX] = "";
    int tailleFichier = 0;

    int salon;
    salon = tabClient[i].salon;

    while (1) {

        //recevoir le titre du fichier
        if (reception(tabClient[i].socketFil, msg) != 1){
            perror("err : recep dans trans");
            pthread_exit(NULL);
        }

        printf("le titre du fichier est : %s \n", msg);
        
        int j = 0; 
        while (j < nbrClient){
            if (j != i  && salon == tabClient[j].salon) {
                //on envoie à tout le nom du fichier
                if (envoie(tabClient[j].socketFil, msg) != 1){
	                perror("erreur nom du fichier");
	                pthread_exit(NULL);
                }
            }
            j++; 
        }
        printf("envoi nom fichier en cours\n");


        //recevoir la taille fichier
        int rec = recv(tabClient[i].socketFil, &tailleFichier, sizeof(int), 0);
        if (rec == -1){
            perror("Erreur 1ere reception\n");
            exit(0);
        }
        if (rec == 0){
            perror("Socket fermée\n");
            exit(0);
        }

        j = 0; 
        while (j < nbrClient){
            if (j != i && salon == tabClient[j].salon) {
                //on envoie à tout le monde la taille du fichier
                int mes = send(tabClient[j].socketFil, &tailleFichier, sizeof(int), 0);
                if (mes == -1){
                    perror("Erreur envoie\n");
                    exit(0);
                }
                if (mes==0){
                    perror("Socket fermée\n");
                    exit(0);
                }
            }
            j++; 
        }
        printf("envoi taille fichier fini\n");
      
        j=0;
        int remainData = tailleFichier;
        char buffer[BUFSIZ];

        while (remainData > 0 && (res = recv(tabClient[i].socketFil, buffer, BUFSIZ, 0)) > 0 ){
            j = 0;
            if (res == -1){
                perror("Erreur reception mot fichier\n");
                pthread_exit(NULL);
            }
            if (res == 0){
                perror("Socket fermée reception mot fichier\n");
                pthread_exit(NULL);
            }
            while(j < nbrClient){
                if (i != j && salon == tabClient[j].salon){
                    int mes = send(tabClient[j].socketFil, buffer, BUFSIZ, 0);
                    if (mes == -1){
                        perror("Erreur envoie\n");
                        exit(0);
                    }
                    if (mes == 0){
                        perror("Aucun envoie\n");
                        exit(0);
                    }
                }
                j = j+1;
            }
            remainData = remainData - res;
            printf("J'ai reçu %d bytes, il me reste à recevoir %d bytes \n", res, remainData);
        }
        printf("fin de l'envoi du fichier \n");
    }
    pthread_exit(0);

    
}

//fonction de connexion
void * connexion (void * args){

    int * pointeur = args;
    int dS = *pointeur;

    struct sockaddr_in aC;
    socklen_t lg = sizeof(struct sockaddr_in);

    int i = 0;
    int j = 0;

    while (nbrClient < nbrClientMax && fin == 0) {

            tabClient[i].socketMes = accept(dS, (struct sockaddr*) &aC,&lg);
            if(tabClient[i].socketMes == -1){
                perror("Erreur accept client \n");
                pthread_exit(NULL);
            }

            printf ("j'ai accepté un nouveau client \n");

            recuperer_pseudo (tabClient[i].pseudo, i);

            recuperer_salon (&tabClient[i].salon, i);

            tabClient[i].socketFil = accept(dS, (struct sockaddr*) &aC,&lg);
            if(tabClient[i].socketFil == -1){
                perror("Erreur accept client \n");
                pthread_exit(NULL);
            }

            printf("client numéro %d connecté avec le pseudo %s dans le salon %d \n", i+1, tabClient[i].pseudo, tabClient[i].salon);

            j = i;
            i = i + 1;

            if (pthread_create(&thread[j], NULL, transmission, &j ) != 0){
                perror("erreur création thread pour message");
                pthread_exit(NULL);
            }
            if (pthread_create(&thread2[j],NULL, transmissionFichier, &j) != 0){
                perror("erreur création thread pour fichier");
                pthread_exit(NULL);
            }else {
                nbrClient = nbrClient + 1;
                printf("nombre client = %d \n", nbrClient);
            }
    }
    pthread_join(thread[0], NULL);

    pthread_exit (NULL);
}

int main(int argc, char* argv[]){

    // je créer un salon par default

    struct salon defaultSalon;
    strcpy(defaultSalon.name,"salon par défaut");
    strcpy(defaultSalon.desc,"si aucun salon n'est choisit le client se retrouve dans celui ci");
    defaultSalon.nbrClientPres = 0;
    defaultSalon.capacityMax = 200;

    tabSalon[0] = defaultSalon;

    struct salon premSalon;
    strcpy(premSalon.name,"premier salon");
    strcpy(premSalon.desc,"premier salon privé créé par le serveur");
    premSalon.nbrClientPres = 0;
    premSalon.capacityMax = 5;

    tabSalon[1] = premSalon;

    nbrSalon = 2;

    //initialisation du tabdSC à -1;
    /*int i = 0;
    while (i < nbrClientMax){
        tabdSC[i] = -1;
        i = i+1;
    }*/

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

    printf("En attente de connexion des clients\n");

    //Connexion client
    pthread_t threadConnexion;
    if (pthread_create (&threadConnexion, NULL, connexion, &dS) != 0){
        perror("erreur création thread");
        return -1;
    }

    while (nbrClient == 0){
        //..
    }

    //attente de la fin des threads
    pthread_join (thread[0], NULL);

    //on ferme les threads et les sockets des clients
    for (long i = 0; i < nbrClient; i++) {
        pthread_cancel(thread[i]); //ferme le thread du client i
        pthread_cancel(thread2[i]);
        close(tabClient[i].socketMes);//ferme la socket du client i
        close(tabClient[i].socketFil);
    }

    pthread_cancel(threadConnexion); //on ferme la socket de connexion
    printf("Fin de la discution \n");

    //on prévient que l'échange est terminé et on ferme les sockets des clients
	printf("Fin de l'échange\n");

	close(dS);
	return 0;

}