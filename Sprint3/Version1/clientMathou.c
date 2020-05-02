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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>


#define TMAX 65000 //taille maximum des paquets (en octets)

char msg[TMAX];
char fileNameReception[TMAX];

//création des deux threads
pthread_t threadEnvoi;
pthread_t threadReception;

//Code fourni et adapté

/* tty est une commande Unix qui affiche sur la sortie standard le nom du fichier connecté sur l'entrée standard. */
int get_last_tty() {
    FILE *fp;
    char path[1035];

    /* engendre un processus en creant un tube (en lecture), exécutant un fork et en invoquant un shell
    le premier argument contient un ligne de commande shell
    fp est donc un flux d'entrée sortie, utilisation de l'entrée et sortie standard */
    fp = popen("/bin/ls /dev/pts", "r");

    if (fp == NULL) {
        printf("Impossible d'exécuter la commande\n" );
        exit(1);
    }
    int i = INT_MIN;

    /* fgets sizeof(path)-1 caractères depuis fp et les stock dans path */
    while (fgets(path, sizeof(path)-1, fp) != NULL) {

        /* Le fichier /dev/ptmx est un fichier spécial caractère. Il sert à créer une  paire de pseudoterminaux maître et esclave.
        Une fois que les deux pseudoterminaux sont ouverts, l'esclave  fournit  une  interface  au processus qui est identique au vrai terminal. */
        if(strcmp(path,"ptmx")!=0){

            /* Cette fonction permet de transformer une chaîne de caractères, représentant une valeur entière, en une valeur numérique de type int.*/
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

    /* commande qui permet d'ouvrir un nouveau terminal */
    system("gnome-terminal"); sleep(1);

    /* La fonction ttyname() renvoie un pointeur sur le chemin d'accèsdu périphérique terminal ouvert */
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

    /* Cette fonction permet d'ouvrir un flux de caractère basé sur fichier */
    FILE *fp = fopen(tty_name,"wb+");
    pthread_mutex_unlock(&the_mutex);
    pthread_mutex_destroy(&the_mutex);
    return fp;
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
	strcpy(msg, "");
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

	strcpy(msg, "");

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

// fonction envoie d'un fichier
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
        while (ep = readdir (dp)) {
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

    int fps = open(filePath, O_RDONLY);

    if (fps == -1){

        printf("Ne peux pas ouvrir le fichier suivant : %s",fileName);

    } else {

        int res = 0;

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

        //calcul nb de caracteres  fichier

        /* possibilité de soucis sur et la structure stat sizeFileByte */
        struct stat file_stat;
        if (fstat(fps, &file_stat) < 0) {
                perror("impossible d'avoir les stats du fichier");
        }
        int sizeFileByte = file_stat.st_size;

        //envoie de la taille du fichier
        res = send(*SockE, &sizeFileByte, sizeof(int), 0);
        if (res == -1){
            perror("Erreur envoie nb caracteres\n");
            pthread_exit(NULL);
        }
        if (res == 0){
            perror("Socket fermée lors de l'envoie du nb caracteres\n");
            pthread_exit(NULL);
        }

        //envoie du contenu du fichier
        long offset = 0;
        int remainData = sizeFileByte;

        // sendfile() copie des données entre deux descripteurs de fichier. Offset est remplie avec la position de l'octet immédiatement après le dernier octet lu.
        while (((res = sendfile(*SockE, fps, &offset, BUFSIZ)) > 0) && remainData > 0){
            remainData = remainData - res;
            printf("J'ai envoyé %d bytes, mon offset vaut %ld et il me reste à envoyer %d bytes \n", res, offset, remainData);
        }
    }
    close(fps);
    pthread_exit(NULL);
}

// fonction reception d'un fichier
void *receptionfichier(void *SockEv){

    int *SockE = SockEv;
	strcpy(fileNameReception, "");
	int taille_fichier;
	int taille_nom;
	char buffer[BUFSIZ];

    // reception de la taille du nom du fichier
	int res = recv(*SockE, &taille_nom, sizeof(int), 0);
	if (res == -1){
		perror("Erreur reception taille \n");
		pthread_exit(NULL);
	}
	if (res == 0){
		perror("Socket fermée reception nombre ligne\n");
		pthread_exit(NULL);
	}

    // reception du nom du fichier
	int taille_rec = 0;
    while (taille_rec < taille_nom)
    {
        res = recv(*SockE, fileNameReception, taille_nom*sizeof(char), 0);
        if (res == -1)
        {
            perror("Erreur reception\n");
            exit(-1);
        }
        if (res == 0)
        {
            perror("Socket fermée\n");
            exit(0);
        }
        taille_rec += res;
    }

	//création du fichier
	char nomFichier[100];
	strcpy(nomFichier,"../Telechargement/");
	strcat(nomFichier,msg);

	FILE* fichier = fopen(nomFichier,"a");

	// reception de la taille du fichier
	res = recv(*SockE, &taille_fichier, sizeof(int), 0);
    if (res == -1){
        perror("Erreur reception taille \n");
        pthread_exit(NULL);
    }
    if (res == 0){
        perror("Socket fermée reception nombre ligne\n");
        pthread_exit(NULL);
    }

	int remainData = taille_fichier;

    // reception du contenu du fichier
    while (remainData > 0 && (res = recv(*SockE, buffer, BUFSIZ, 0)) > 0 ){
        if (res == -1){
            perror("Erreur reception mot fichier\n");
            pthread_exit(NULL);
        }
        if (res == 0){
            perror("Socket fermée reception mot fichier\n");
            pthread_exit(NULL);
        }
        fprintf(fichier, "%s", buffer);
        remainData = remainData - res;
        printf("J'ai reçu %d bytes, il me reste à recevoir %d bytes \n", res, remainData);
	}
	printf("\n le Fichier reçu se trouve maintenant dans le dossier Telechargement :\n");
	printf("%s\n", nomFichier);
	fclose(fichier);
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