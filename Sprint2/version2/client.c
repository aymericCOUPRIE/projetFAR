#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define TMAX 65000 //taille maximum des paquets (en octets)

struct param_thread {
    int socket;
    char buffer [BUFSIZ];
};

//fonction pour envoyer un message
void *envoie(void *paramVoid) {

    struct param_thread *param = (struct param_thread *)paramVoid;

	int taille_msg;
	int res; //pour les tests de retour de fonctions


    taille_msg = (strlen(param -> buffer) + 1) * sizeof(char);

    //Envoi de la taille du message
    res = send(param -> socket, &taille_msg, sizeof(int), 0);
    if (res == -1)
    {
        perror("Erreur envoie\n");
        exit(-1);
    }
    if (res == 0)
    {
        perror("Socket fermée\n");
        exit(0);
    }

    //Envoi du message
    res = send(param -> socket, param -> buffer, strlen(param -> buffer)+1, 0);
    if (res == -1)
    {
        perror("Erreur envoie\n");
        exit(-1);
    }
    if (res == 0)
    {
        perror("Aucun envoie\n");
        exit(0);
    }

}

void *affichage_message(void *paramVoid){

    struct param_thread *param = (struct param_thread *)paramVoid;

    printf(" message recu de %s\n", param -> buffer);
}

//fonction pour recevoir un message
void *reception(void *paramVoid)
{

    struct param_thread *param = (struct param_thread *)paramVoid;
	int taille_msg;
	int rec;
	int taille_rec = 0;


	while (1)
	{
		//Réception de la taille du message à recevoir
		rec = recv(param -> socket, &taille_msg, sizeof(int), 0);
		if (rec == -1)
		{
			perror("Erreur reception\n");
			exit(-1);
		}
		else if (rec == 0)
		{
			perror("Aucun message recu\n");
			exit(0);
		}

		//Boucle pour recevoir toutes les portions du message
		taille_rec = 0;
		while (taille_rec < taille_msg)
		{
			rec = recv(param -> socket, param -> buffer, taille_msg*sizeof(char), 0);
			if (rec == -1)
			{
				perror("Erreur reception\n");
				exit(-1);
			}
			if (rec == 0)
			{
				perror("Socket fermée\n");
				exit(0);
			}
			taille_rec += rec;
		}
		affichage_message(param -> buffer);
	}
	pthread_exit(NULL);
}

void *fctReceptionThread (void* paramVoid){

     struct param_thread *param = (struct param_thread *)paramVoid;

     while (1){
        reception((void *)param);
        affichage_message((void *)param);
     }
}

void *fctEnvoieThread (void* paramVoid){

    struct param_thread *param = (struct param_thread *)paramVoid;

    while (1){
        fgets(param -> buffer, BUFSIZ, stdin); //saisie clavier du message
        envoie((void *)param);
    }
    pthread_exit(NULL);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{

	//Vérification des arguments
	if (argc != 3)
	{
		printf("paramètres: ./client Adresse_IP  Numéro_de_port\n");
		exit(0);
	}

	//definition de la socket
	int dS;			   //Socket client
	dS = socket(PF_INET, SOCK_STREAM, 0);
	if (dS == -1)
	{
		perror("Erreur socket\n");
		return -1;
	}

	//structure de la socket
	struct sockaddr_in aS;
	aS.sin_family = AF_INET;
	aS.sin_port = htons(atoi(argv[2])); //on récupère le port dans les arguments du terminal

	int res;
	res = inet_pton(AF_INET, argv[1], &(aS.sin_addr)); //on converti l'adresse IP décimale en binaire et on la stock dans aS.sin_addr pour ensuite désigner le destinataire
	if (res == -1)
	{
		perror("Erreur inet_pton\n");
		return -1;
	}

	//connexion au serveur
	socklen_t lgA = sizeof(aS);
	res = connect(dS, (struct sockaddr *)&aS, lgA);
	if (res == -1)
	{
		perror("Erreur connect\n");
		return -1;
	}

	//demande pseudo
	char pseudo[200];
	printf("Saisir votre pseudo: \n");
	fgets(pseudo, 200, stdin);
	//remplace \n par \0
	pseudo[strlen(pseudo) - 1] = '\0';

	//initialisation envoiePseudo
	struct param_thread param_pseudo;
    param_pseudo.socket = dS;
    strcpy(param_pseudo.buffer, pseudo);

	envoie((void*)&param_pseudo);

	char msg[TMAX] = "";

	//création des deux threads
	pthread_t threadEnvoi;
	pthread_t threadReception;

	// initialisation du threadEnvoie
    struct param_thread param_envoie;
    param_envoie.socket = dS;

	if (pthread_create(&threadEnvoi, NULL, fctEnvoieThread, (void *)&param_envoie) != 0)
	{
		perror("erreur creation thread\n");
		return -1;
	}

	// initialisation du threadReception
	struct param_thread param_reception;
	param_reception.socket = dS;

	if (pthread_create(&threadReception, NULL, fctReceptionThread, (void *)&param_reception) != 0)
	{
		perror("erreur creation thread\n");
		return -1;
	}

	//attente de la fin des threads
	pthread_join(threadEnvoi, NULL);
	pthread_join(threadReception, NULL);

	//ferme la socket
	close(dS);
	return 0;
}