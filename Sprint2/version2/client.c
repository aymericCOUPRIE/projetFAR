#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define TMAX 65000 //taille maximum des paquets (en octets)


//Fonction qui envoie le pseudo au serveur pour qu'il le stock
void *envoiepseudo(char *pseudo, void *SockEv)
{
	int *SockE = SockEv;

	int taille = (strlen(pseudo) + 1) * sizeof(char);
	//envoie taille du spseudo en octets
	int res = send(*SockE, &taille, sizeof(int), 0);
	if (res == -1)
	{
		perror("Erreur envoie taille pseudo\n");
		pthread_exit(NULL);
	}
	if (res == 0)
	{
		perror("Socket fermée envoie taille pseudo\n");
		pthread_exit(NULL);
	}
	//envoie pseudo
	res = send(*SockE, pseudo, strlen(pseudo) + 1, 0);
	if (res == -1)
	{
		perror("Erreur envoie pseudo\n");
		pthread_exit(NULL);
	}
	if (res == 0)
	{
		perror("Socket fermée envoie pseudo\n");
		pthread_exit(NULL);
	}
}

//fonction pour envoyer un message
void *envoie(void *SockEv)
{
	char msg[TMAX] = "";
	int taille_msg;
	int res; //pour les tests de retour de fonctions

	int *SockE = SockEv;

	while (1)
	{
		// printf("Votre message : ");
		fgets(msg, TMAX, stdin); //saisie clavier du message
		taille_msg = (strlen(msg) + 1) * sizeof(char);
		//Envoi de la taille du message
		res = send(*SockE, &taille_msg, sizeof(int), 0);
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
		res = send(*SockE, msg, strlen(msg)+1, 0);
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

	pthread_exit(NULL);
}

//fonction pour recevoir un message
void *reception(void *SockEv)
{

	int taille_msg;
	int rec;
	int taille_rec = 0;

	char msg[TMAX] = "";

	int *SockE = SockEv;

	while (1)
	{
		//Réception de la taille du message à recevoir
		rec = recv(*SockE, &taille_msg, sizeof(int), 0);
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
			rec = recv(*SockE, msg, taille_msg*sizeof(char), 0);
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
		printf("Message reçu : %s\n", msg);
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
	envoiepseudo(pseudo, &dS);

	char msg[TMAX] = "";

	//création des deux threads
	pthread_t threadEnvoi;
	pthread_t threadReception;

	if (pthread_create(&threadEnvoi, NULL, envoie, &dS) != 0)
	{
		perror("erreur creation thread\n");
		return -1;
	}

	if (pthread_create(&threadReception, NULL, reception, &dS) != 0)
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