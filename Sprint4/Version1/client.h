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

int dSFile;
int dSMessage;

pthread_t threadEnvoi;
pthread_t threadReception;
pthread_t threadReceptionFichier;
pthread_t threadEnvoiFichier;

//tty est une commande Unix qui affiche sur la sortie standard le nom du fichier connecté sur l'entrée standard.
int get_last_tty();

//
FILE* new_tty();

void *affichage_message(void *paramVoid);

//fonction pour recevoir un message
void *reception(void *paramVoid);

//fonction reception d'un fichier
void *receptionfichier(void *null);

//fonction qui vérifie si le message recu = file et active un nouveau thread
void *recu_Msg_File (void * paramVoid);

// fonction appelé par le threadReception
void *fctReceptionThread (void* paramVoid);

void *envoi_nv_salon (void* paramVoid);

//fonction pour envoyer un message
void *envoie(void *paramVoid);

void *ajout_salon (void* paramVoid);

void *affichage_repertoire ();

void *envoiFichier(void *paramVoid);

void *envoi_Msg_file (void* paramVoid);

void *fctEnvoiThread (void* paramVoid);