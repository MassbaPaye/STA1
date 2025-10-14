/********************************************************/
/* Application : serveur TCP dans le domaine AF_INET    */
/* Date : 23/09/2024                                   */
/* version : 2.0                                        */
/* Auteur : A. TOGUYENI                                 */
/********************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/un.h>
#include <sys/types.h>          
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "serveur_TCP.h"

#include "config.h"


int main_communication_tcp_controleur()
{
   int se; // socket d'écoute
int sd1; // socket de dialogue
int pid;
struct sockaddr_in adrlect;
int erreur; 
int nbcars; 
char buff[MAXOCTETS];
int nbfils=1;

// Etape 1 : creation de la socket
se = socket(AF_INET, SOCK_STREAM, 0);
CHECK_ERROR(se, -1, "erreur de creation de la socket !!!\n");

// Etape 2 : on complete l'adresse de la socket
adrlect.sin_family = AF_INET;
adrlect.sin_port = htons(TCP_PORT);
adrlect.sin_addr.s_addr = INADDR_ANY;

// Etape 3 : on affecte une adresse à la socket d'écoute
erreur = bind(se, (const struct sockaddr *) &adrlect, sizeof(struct sockaddr_in));
CHECK_ERROR(erreur, -1, "Echec de bind !!!\n");

// Etape 4 : écoute
listen(se, 8);
printf("Lecteur en attente de connexion !!!\n");

// Etape 5 : boucle d'accept
while (1) {
    sd1 = accept(se, NULL, NULL); // accepter la connexion
    CHECK_ERROR(sd1, -1, "Erreur accept !!!\n");
    printf("Nouvelle connexion accepte par le serveur !!!\n");

    pid = fork();
    if (pid) {
        // père
        close(sd1);
        nbfils++;
    } else {
        // fils
        close(se);
        do {
            nbcars = recv(sd1, buff, MAXOCTETS, 0);
            if (nbcars > 0)
                printf("LECTEUR %d - msg recu : %s\n", nbfils, buff);
            else
                printf("KO !!!\n");
        } while (strcmp(buff, "fin") != 0);

        close(sd1);
        return 0;
    }
}
 
}
