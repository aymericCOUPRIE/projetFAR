#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#define TMAX 65000 //taille maximum des paquets (en octets)
#define nbClientsMax 200
int dSC[200]; //tableau de 200 sockets
char pseudos[200][200];
int nbclients;
pthread_t thread[nbClientsMax];

//fonction pour récupérer et stocker les pseudo
void recuperer_pseudo(char *pseudo, int i)
{

    printf("je suis dans récupérer pseudo \n");
	int taillemessage;
	//on récupère le nombre d'octets du paquets où il y a le pseudo
	int res = recv(dSC[i], &taillemessage, sizeof(int), 0);
	if (res == -1)
	{
		perror("Erreur reception taille pseudo");
		pthread_exit(NULL);
	}
	if (res == 0)
	{
		perror("Socket fermée lors de la reception de la taille du pseudo\n");
		pthread_exit(NULL);
	}

    printf("j'ai récupéré la taille du pseudo \n");
    printf("la taille du pseudo vaut %d \n", taillemessage);

	//on récupère le pseudo par morceaux
	int nbrecu = 0;
	while (nbrecu < taillemessage)
	{
		res = recv(dSC[i], pseudo, taillemessage * sizeof(char), 0);
		if (res == -1)
		{
			perror("Erreur reception  pseudo\n");
			pthread_exit(NULL);
		}
		if (res == 0)
		{
			perror("Socket fermée lors de la reception du pseudo\n");
			pthread_exit(NULL);
		}
		nbrecu += res;
	}

	printf("j'ai récupéré le pseudo \n");
	printf("voici le pseudo %s", pseudo);

	//On stocke tous les pseudos dans un tableau
	strcpy(pseudos[i], pseudo);
	printf("j'ai copié le pseudo \n");
}

void *transmission(void *args)
{
    printf("je suis dans transmission");

    int *pointeur = args;
	int i = *pointeur; //numéro de la socket du client qui envoit le message
	char message[TMAX]; //message d'un client
	int fin = 0;		//signal de fin
	int taille_msg;
	int rec; //pour faire test des fonctions

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
		
		int taille_rec = 0; //taille de ce qu'on a reçu du message
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

		//on met le pseudo au début du message avant de le transmettre
		char newmessage[TMAX];
		strcpy(newmessage, pseudos[i]); //copie le pseudo dans newmessage
		strcat(newmessage, " : ");		//concatène ":" après newmessage dans newmessage
		strcat(newmessage, message);	//concatene message à newmessage
		strcpy(message, newmessage);	//copie new message dans message
		//on met à jout la nouvelle taille du message
		taille_msg = (strlen(message) + 1) * sizeof(char);

		//envoie du message aux autres clients sauf à celui qui l'a envoyé (i)
		for (int j = 0; j < nbclients; j++)
		{
			if (j != i)
			{
				/* envoie du nbre d'octets du paquet contenant le message */
				rec = send(dSC[j], &taille_msg, sizeof(int), 0);
				if (rec == -1)
				{
					perror("Erreur envoie taille message\n");
					pthread_exit(NULL);
				}
				else if (rec == 0)
				{
					perror("Socket fermée recption taille \n");
					pthread_exit(NULL);
				}

				rec = send(dSC[j], message, taille_msg, 0); /* envoie du message*/
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
	}
}

//fonction pour que les clients se connectent à n'importe quel moment de la connexion
void *connexion(void *arg)
{
	//nbclients = 0;
	int *dSEv = arg;
	int dSE = *dSEv;
	struct sockaddr_in aC;
	socklen_t lg = sizeof(struct sockaddr_in);
	int i = 0;
	while (1)
	{
		if (i < nbClientsMax)
		{
			dSC[i] = accept(dSE, (struct sockaddr *)&aC, &lg);
			if (dSC[i] == -1)
			{
				perror("Probleme accept d'un client");
				pthread_exit(NULL);
			}
			printf("client connecté \n");
			recuperer_pseudo(pseudos[i], i);
			printf("Client numéro  %d connecté avec le pseudo ' %s' \n", i + 1, pseudos[i]);
			if (pthread_create(&thread[i], NULL, transmission, &i))
			{
				perror("creation threadGet erreur");
				pthread_exit(NULL);
			}
			printf("je suis après le thread");
			nbclients += 1;
			i++;
			printf("nbclients : %d", nbclients);
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
		nbclients = 0;
		printf("En attente de connexion de clients\n");

		//Connexion client
		pthread_t connexionClient; //thread pour connecter un client
		if (pthread_create(&connexionClient, NULL, connexion, &dS))
		{
			perror("Erreur création thread connexionClient");
			return EXIT_FAILURE;
		}

		//On attend qu'il y ait au moins 2 client
		while (nbclients <= 1)
		{
			//....
		}

		printf("Debut de la discussion\n");

		if (pthread_join(thread[0], NULL))
		{ //pthread_join attend que le thread du client se ferme
			perror("Erreur pthread_join ");
			return EXIT_FAILURE;
		}

		//on ferme les threads et les sockets des clients
		for (long i = 0; i < nbclients; i++)
		{
			pthread_cancel(thread[i]); //ferme le thread du client i
			close(dSC[i]);			   //ferme la socket du client i
		}
		pthread_cancel(connexionClient); //on ferme la socket de connexion
		printf("Fin de la discution \n");
	}
	close(dS); //ferme la socket d'écoute

	return 0;
}