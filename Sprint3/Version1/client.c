#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <limits.h>
#include <dirent.h>
#include <string.h>

#define TMAX 65000 //taille maximum des paquets (en octets)
//création thread
pthread_t threadReceptionFichier;
pthread_t threadEnvoiFichier;
//code fournie
int get_last_tty() {
  FILE *fp;
  char path[1035];
  fp = popen("/bin/ls /dev/pts", "r");
  if (fp == NULL) {
    printf("Impossible d'exécuter la commande\n" );
    exit(1);
  }
  int i = INT_MIN;
  while (fgets(path, sizeof(path)-1, fp) != NULL) {
    if(strcmp(path,"ptmx")!=0){
      int tty = atoi(path);
      if(tty > i) i = tty;
    }
  }

  pclose(fp);
  return i;
}

FILE* new_tty() {
  pthread_mutex_t the_mutex;
  pthread_mutex_init(&the_mutex,0);
  pthread_mutex_lock(&the_mutex);
  system("gnome-terminal"); sleep(1); //ouverture d'un terminal
  char *tty_name = ttyname(STDIN_FILENO);
  int ltty = get_last_tty();
  char str[2];
  sprintf(str,"%d",ltty);
  int i;
  for(i = strlen(tty_name)-1; i >= 0; i--) {
    if(tty_name[i] == '/') break;
  }
  tty_name[i+1] = '\0';
  strcat(tty_name,str);
  FILE *fp = fopen(tty_name,"wb+");
  pthread_mutex_unlock(&the_mutex);
  pthread_mutex_destroy(&the_mutex);
  return fp;
}

//fonction pour envoyer d'un fichier
void *envoieFichier(void *SockEv){
	int *SockE = SockEv;
	FILE* fp1 = new_tty();
  fprintf(fp1,"%s\n","Ce terminal sera utilisé uniquement pour l'affichage");

  // Demander à l'utilisateur quel fichier afficher
  DIR *dp;
  struct dirent *ep;
  dp = opendir ("../Repertoire/");
  if (dp != NULL) {
    fprintf(fp1,"Fichiers contenus dans le répertoire :\n");
    while (ep == readdir (dp)) {
      if(strcmp(ep->d_name,".")!=0 && strcmp(ep->d_name,"..")!=0)
	fprintf(fp1,"%s\n",ep->d_name);
    }
    (void) closedir (dp);
  }
  else {
    perror ("Erreur ouverture du répertoire");
  }
  printf("Indiquer le nom du fichier choisi: ");
  char fileName[1023];
	char filePath[1023];

  fgets(fileName,sizeof(fileName),stdin);
  fileName[strlen(fileName)-1]='\0';


	//envoie  contenu de fichier
  char fileName2[1023];
  strcpy(fileName2, "../Repertoire/");
  strcat(fileName2, fileName);
  strcpy(filePath, fileName2);

  FILE *fps = fopen(filePath, "r");

	//calcul nb de caracteres  fichier
	int nb_char=0;
	char c=fgetc(fps);
	while (c != EOF) {
		nb_char++;
		c=fgetc(fps);
	}

	fclose(fps);
	fps = fopen(filePath, "r");


	//envoie du nbre de caracteres
	int res = send(*SockE, &nb_char, sizeof(int), 0);
	if (res == -1){
		perror("Erreur envoie nb caracteres\n");
		pthread_exit(NULL);
	}
	if (res == 0){
		perror("Socket fermée lors de l'envoie du nb caracteres\n");
		pthread_exit(NULL);
	}

  if (fps == NULL){
    printf("Ne peux pas ouvrir le fichier suivant : %s",fileName);
  }
  else {

		//Envoie du nom de fichier
		int taille = (strlen(fileName)+1)*sizeof(char);
		res = send(*SockE, &taille, sizeof(int), 0); /* envoie du nbre d'octets du paquet contenant le message */
		if (res == -1){
			perror("Erreur envoie taille nom fichier\n");
			pthread_exit(NULL);
		}
		if (res == 0){
			perror("Socket fermée envoie taille nom fichier\n");
			pthread_exit(NULL);
		}
		//on envoit le nom du fichier
		res = send(*SockE, fileName, taille, 0); 
		if (res == -1){
			perror("Erreur envoie nom fichier \n");
			pthread_exit(NULL);
		}
		if (res == 0){
			perror("Socket fermée envoie nom fichier \n");
			pthread_exit(NULL);
		}

    char str[1000];

    // Lire et afficher le contenu du fichier
		char c=fgetc(fps);
    while (c != EOF) {
			taille = 1;
			/* envoie du nbre d'octets du paquet contenant le message */
			res = send(*SockE, &taille, sizeof(int), 0); 
			if (res == -1){
				perror("Erreur envoie taille\n");
				pthread_exit(NULL);
			}
			if (res == 0){
				perror("Socket fermée envoie taille\n");
				pthread_exit(NULL);
			}
			/* envoie du message*/
			res = send(*SockE, &c , taille, 0); 
			if (res == -1){
				perror("Erreur envoie message\n");
				pthread_exit(NULL);
			}
			if (res == 0){
				perror("Socket fermée envoie message\n");
				pthread_exit(NULL);
			}
			c=fgetc(fps);
    }
  }
  fclose(fps);
	pthread_exit(NULL);
}



//fonction pour recevoir un fichier
void *recoieFichier(void *SockEv){
	int *SockE = SockEv;
	char msg[TMAX];//texte contenu dans le fichier que le client va envoyer
	int nb_octets;
	int nb_char;

	//reception du nb de lignes
	int res = recv(*SockE, &nb_char, sizeof(int), 0); 
	if (res == -1){
		perror("Erreur reception nombre ligne \n");
		pthread_exit(NULL);
	}
	if (res == 0){
		perror("Socket fermée reception nombre ligne\n");
		pthread_exit(NULL);
	}

	//reception du nom de fichier
	res = recv(*SockE, &nb_octets, sizeof(int), 0); /*nbre d'octets du paquet contenant le mot recu*/
	if (res == -1){
		perror("Erreur reception taille\n");
		pthread_exit(NULL);
	}
	if (res == 0){
		//perror("Socket fermée reception taille\n");
		pthread_exit(NULL);
	}
	int nb_recu = 0;//nbre d'octets recus

	while(nb_recu<nb_octets){
		res = recv(*SockE, msg, nb_octets*sizeof(char), 0); 
		if (res == -1){
			perror("Erreur reception message fichier\n");
			pthread_exit(NULL);
		}
		if (res == 0){
			perror("Socket fermée reception message\n");
			pthread_exit(NULL);
		}
		nb_recu+=res;
	}

	//création du fichier
	char nomFichier[100];
	strcpy(nomFichier,"../Telechargement/");
	strcat(nomFichier,msg);

	FILE* fichier = fopen(nomFichier,"a");

	int nb_char_recue=0;
	char c;


	while(nb_char_recue<nb_char){
		//reception du contenu du fichier
		res = recv(*SockE, &nb_octets, sizeof(int), 0); 
		if (res == -1){
			perror("Erreur reception taille\n");
			pthread_exit(NULL);
		}
		if (res == 0){
			//perror("Socket fermée reception taille\n");
			pthread_exit(NULL);
		}
		nb_recu = 0;//nbre d'octets recus

		while(nb_recu<nb_octets){
			res = recv(*SockE, &c, sizeof(char), 0); 
			if (res == -1){
				perror("Erreur reception mot fichier\n");
				pthread_exit(NULL);
			}
			if (res == 0){
				perror("Socket fermée reception mot fichier\n");
				pthread_exit(NULL);
			}
			nb_recu+= res;
		}

		nb_char_recue++;
		fprintf(fichier, "%c", c);
	}
	printf("\n le Fichier reçu se trouve maintenant dans le dossier Telechargement :\n");
	printf("%s\n", nomFichier);
	fclose(fichier);
	pthread_exit(NULL);
}

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

		//on regarde si il veut envoyer un fichier
		if(strcmp(msg,"file\n")==0){
			if( pthread_create(&threadEnvoiFichier, NULL, envoieFichier, NULL ) ){
				perror("creation threadFileSnd erreur");
				pthread_exit(NULL);
			}

			if(pthread_join(threadEnvoiFichier, NULL)){ //pthread_join attend la fermeture
				perror("Erreur fermeture threadFileSnd");
				pthread_exit(NULL);
			}
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
	

		//on regarde si le contenu du message est "file"
		if(strcmp(msg,"file\n") !=0){
		printf(" %s\n", msg);
		}
		else{
			if( pthread_create(&threadReceptionFichier, NULL, recoieFichier, NULL ) ){
				perror("creation threadFileSnd erreur");
				pthread_exit(NULL);
			}

			if(pthread_join(threadReceptionFichier, NULL)){ //pthread_join attend la fermeture
				perror("Erreur fermeture threadFileSnd");
				pthread_exit(NULL);
			}

		}
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

	//création des threads
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