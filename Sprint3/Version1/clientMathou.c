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

struct param_thread {
    int socket;
    char buffer [BUFSIZ];
};

//création des deux threads
pthread_t threadEnvoi;
pthread_t threadReception;
pthread_t threadReceptionFichier;
pthread_t threadEnvoiFichier;

//Code fourni et compris

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

void *affichage_message(void *paramVoid){

    struct param_thread *param = (struct param_thread *)paramVoid;

    printf(" message recu de %s\n", param -> buffer);
}

//fonction pour recevoir un message
void *reception(void *paramVoid) {

    struct param_thread *param = (struct param_thread *)paramVoid;
	int taille_msg;
	int rec;
	int taille_rec = 0;

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

}

//fonction reception d'un fichier
void *receptionfichier(void *paramVoid){

    struct param_thread *param = (struct param_thread *)paramVoid;

	int taille_fichier;
	int taille_nom;
	char buffer[BUFSIZ];

    reception((void *)param);

	//création du fichier
	char nomFichier[100];
	strcpy(nomFichier,"../Telechargement/");
	strcat(nomFichier,param -> buffer);

	FILE* fichier = fopen(nomFichier,"a");

    if (fichier == NULL) {
        perror("failed to open file");
    }

	// reception de la taille du fichier
	int res = recv(param -> socket, &taille_fichier, sizeof(int), 0);
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
    while (remainData > 0 && (res = recv(param -> socket, buffer, BUFSIZ, 0)) > 0 ){
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

//fonction qui vérifie si le message recu = file et active un nouveau thread
void *recu_Msg_File (void * paramVoid){

    struct param_thread *param = (struct param_thread *)paramVoid;

    char *strToken = strtok(param -> buffer, ": ");
    // on enleve le pseudo de la chaine à comparer
    strToken = strtok(NULL, ": ");

    if(strcmp(strToken,"file\n") ==0){

        if( pthread_create(&threadReceptionFichier, NULL, receptionfichier, NULL ) ){
            perror("creation threadFileSnd erreur");
            pthread_exit(NULL);
        }

        if(pthread_join(threadReceptionFichier, NULL)){ //pthread_join attend la fermeture
            perror("Erreur attente threadFileSnd");
            pthread_exit(NULL);
        }

    }
}

// fonction appelé par le threadReception
void *fctReceptionThread (void* paramVoid){

     struct param_thread *param = (struct param_thread *)paramVoid;

     while (1){
        reception((void *)param);
        affichage_message((void *)param);
        recu_Msg_File((void *)param);
     }

     pthread_exit(NULL);
}

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

//fonction qui affiche la liste des répertoire dans un nouveau terminal
void *affichage_repertoire (){
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

}

// fonction envoie d'un fichier
void *envoiFichier(void *paramVoid){

    struct param_thread *param = (struct param_thread *)paramVoid;

    //TODO : si erreur remettre l'affichage ici
	affichage_repertoire();

    //choix du fichier à envoyer
    printf("Indiquer le nom du fichier choisi: \n");

    char fileName[1024] = "";
    char filePath[1024] = "";

    fgets(fileName,sizeof(fileName),stdin);
    //system("exit"); sleep(1);
    fileName[strlen(fileName)-1]='\0';

    //ouverture du fichier
    strcpy(filePath, "../Repertoire/");
    strcat(filePath, fileName);

    printf("le path vaut : %s \n", filePath);
    printf("filename vaut : %s \n", fileName);

    printf("j'ouvre le fichier \n");

    //j'utilise open car cela me permet d'utiliser des fonctions comme sendFile qui sont plus efficaces.
    int fps = open(filePath, O_RDONLY);

    printf("la valeur de fps vaut %d", fps);
    printf("point 2");

    sleep(3);

    if (fps == -1){

        printf("Ne peux pas ouvrir le fichier suivant : %s",fileName);

    } else {

        int res = 0;

        printf("le nom du fichier vaut %s", fileName);

        //initialisation et envoie du nom du fichier
        strcpy(param -> buffer, fileName);
        envoie((void *)param);

        printf("point 3");

        //calcul nb de caracteres  fichier
        /* possibilité de soucis sur et la structure stat sizeFileByte */
        struct stat file_stat;
        if (fstat(fps, &file_stat) < 0) {
                perror("impossible d'avoir les stats du fichier");
        }
        int sizeFileByte = file_stat.st_size;

        //envoie de la taille du fichier
        res = send(param -> socket, &sizeFileByte, sizeof(int), 0);
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
        while (((res = sendfile(param -> socket, fps, &offset, BUFSIZ)) > 0) && remainData > 0){
            if (res == -1){
                perror("Erreur envoie du contenu du fichier\n");
                pthread_exit(NULL);
            }
            if (res == 0){
                perror("Socket fermée lors de l'envoie du contenu du fichier\n");
                pthread_exit(NULL);
            }
            remainData = remainData - res;
            printf("J'ai envoyé %d bytes, mon offset vaut %ld et il me reste à envoyer %d bytes \n", res, offset, remainData);
        }
    }
    close(fps);
    pthread_exit(NULL);
}

//fonction qui vérifie si le message envoyé = file et active un nouveau thread
void *envoi_Msg_file (void* paramVoid){

    struct param_thread *param = (struct param_thread *)paramVoid;

    if(strcmp(param -> buffer,"file\n") ==0){

        if( pthread_create(&threadEnvoiFichier, NULL, envoiFichier, NULL ) ){
            perror("creation threadFileSnd erreur");
            pthread_exit(NULL);
        }

        if(pthread_join(threadEnvoiFichier, NULL)){ //pthread_join attend la fermeture
            perror("Erreur attente threadFileSnd");
            pthread_exit(NULL);
        }

    }
}

// fonction appelé par le threadEnvoi
void *fctEnvoiThread (void* paramVoid){

    struct param_thread *param = (struct param_thread *)paramVoid;

    while (1){
        fgets(param -> buffer, BUFSIZ, stdin); //saisie clavier du message
        envoie((void *)param);
        envoi_Msg_file((void *)param);
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

	if (pthread_create(&threadEnvoi, NULL, fctEnvoiThread, (void *)&param_envoie) != 0)
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