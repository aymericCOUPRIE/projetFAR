#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

pthread_t thread[nbrClientMax]; //pour les messages
pthread_t thread2[nbrClientMax]; //pour les fichiers


//fonction pour envoyer un message
int envoie(int SockE, char* message);

//fonction pour recevoir un message
int reception(int sockE, char* message);

//fonction pour récupérer le pseudo
void recuperer_pseudo (char *pseudo, int i);

void recuperer_salon (int *salon, int i);

void * ajout_salon(int i);

//fonction pour la transmission des messages
void * transmission (void * args);

void* transmissionFichier(void* arg);

//fonction de connexion
void * connexion (void * args);
