#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#define TMAX 65000 //taille maximum des paquets (en octets)
int dSC[2];		   //tableau de deux sockets

void *transmission(void *args)
{
	int i = (long)args;
	char message[TMAX]; //message d'un client
	int fin = 0;		//signal de fin
	int taille_msg;
	int rec;			//pour faire test des fonctions
	int taille_rec = 0; //taille de ce qu'on a reçu du message

	while (fin != 1)
	{
		//on reçoit la taille d'un message d'un client

		rec = recv(dSC[i], &taille_msg, sizeof(int), 0);
		if (rec == -1)
		{
			perror("Erreur reception taille message\n");
			pthread_exit(NULL);
		}
		else if (rec == 0)
		{
			perror("Socket fermée reception taille message\n");
			pthread_exit(NULL);
		}

		//Boucle pour recevoir toutes les portions du message
		while (taille_rec < taille_msg)
		{
			rec = recv(dSC[i], message, taille_msg * sizeof(char), 0);
			if (rec == -1)
			{
				perror("Erreur reception message\n");
				pthread_exit(NULL);
			}
			if (rec == 0)
			{
				perror("Socket fermée reception message\n");
				pthread_exit(NULL);
			}
			taille_rec += rec;
		}

		//on vérifie si le client veut mettre fin à la conversation
		if (strcmp(message, "fin\n") == 0)
		{ //si il veut mettre fin à la conversation
			//on envoit la taille du mot "fin"
			rec = send(dSC[i], &taille_msg, sizeof(int), 0);
			if (rec == -1)
			{
				perror("Erreur envoie taille fin \n");
				pthread_exit(NULL);
			}
			if (rec == 0)
			{
				perror("Socket fermée envoie taille fin\n");
				pthread_exit(NULL);
			}
			//on envoit le mot "fin"
			rec = send(dSC[i], message, taille_msg, 0);
			if (rec == -1)
			{
				perror("Erreur envoie message fin\n");
				pthread_exit(NULL);
			}
			if (rec == 0)
			{
				perror("Socket fermée envoie message fin\n");
				pthread_exit(NULL);
			}
		}

		//envoie du message à l'autre client
		int dSC2; //l'autre client
		if (i == 0)
		{
			dSC2 = dSC[1];
		}
		else
		{
			dSC2 = dSC[0];
		}

		/* envoie du nbre d'octets du paquet contenant le message */
		rec = send(dSC2, &taille_msg, sizeof(int), 0);
		if( rec == -1){
perror("Erreur envoie taille message\n");
		pthread_exit(NULL);
		}else if (rec == 0)
		{
			perror("Socket fermée recption taille \n");
			pthread_exit(NULL);
		}

		rec = send(dSC2, message, taille_msg, 0); /* envoie du message*/
		if (rec == -1)
		{
			perror("Erreur envoie message\n");
			pthread_exit(NULL);
		}
		if (rec == 0)
		{
			perror("Socket fermée envoie message\n");
			pthread_exit(NULL);
		}
	}
	pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		perror("paramètres : ./serveur  Numero_de_port");
		exit(0);
	}

	int dS = socket(PF_INET, SOCK_STREAM, 0); //création de la socket
	if (dS == -1)
	{
		perror("Erreur création socket\n");
		exit(0);
	}

	//structure de la socket
	struct sockaddr_in ad;
	ad.sin_family = AF_INET;
	ad.sin_addr.s_addr = INADDR_ANY;
	ad.sin_port = htons(atoi(argv[1])); //on récupère le port passé en paramètres, atoi permet de convertir une chaine de caractère en int

	//nomage de la socket
	int res = bind(dS, (struct sockaddr *)&ad, sizeof(struct sockaddr_in));
	if (res == -1)
	{
		perror("Erreur bind\n");
		exit(0);
	}

	//on passe la socket en écoute
	res = listen(dS, 7);
	if (res == -1)
	{
		perror("Erreur listen\n");
		exit(0);
	}

	struct sockaddr_in aC;
	socklen_t lg = sizeof(struct sockaddr_in);

	while (1)
	{
		printf("En attente de connexion des 2 clients\n");

		//Connexion client 1
		dSC[0] = accept(dS, (struct sockaddr *)&aC, &lg);
		if (dSC[0] == -1)
		{
			perror("Probleme accept client 1  \n");
			exit(0);
		}
		printf("Client 1 : Connexion OK\n");
		

		//Connexion client 2
		dSC[1] = accept(dS, (struct sockaddr *)&aC, &lg);
		if (dSC[1] == -1)
		{
			perror("Erreur accept client 2\n");
			exit(0);
		}
		printf("Client 2 : Connexion OK\n");

		

		//création des 2 threads
		pthread_t threadC1vC2; //thread client 1 vers client 2
		pthread_t threadC2vC1; //thread client 2 vers client 1

		printf("Debut de la discussion\n");

		if (pthread_create(&threadC1vC2, NULL, transmission, (void *)0))
		{
			perror("creation threadGet erreur");
			return EXIT_FAILURE;
		}

		if (pthread_create(&threadC2vC1, NULL, transmission, (void *)1))
		{
			perror("creation threadSnd erreur");
			return EXIT_FAILURE;
		}

		if (pthread_join(threadC1vC2, NULL) || pthread_join(threadC2vC1, NULL))
		{ //pthread_join attend la fermeture de threadSnd
			perror("Erreur fermeture threadSnd");
			return EXIT_FAILURE;
		}

		//on ferme les thread
		pthread_cancel(threadC2vC1);
		pthread_cancel(threadC1vC2);

		//on prévient que l'échange est terminé et on ferme les sockets des clients
		printf("Fin de l'échange\n");
		printf("Fin de l'échange\n");
		close(dSC[0]); //ferme la socket du client 1
		close(dSC[1]); //ferme la socket du client 2
	}
	close(dS); //ferme la socket d'écoute

	return 0;
}