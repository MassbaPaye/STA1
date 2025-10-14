/********************************************************/
/* Application : client TCP dans le domaine AF_INET     */
/* Date : 23/09/2024                                   */
/* version : 1.0                                        */
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
#include "config.h"

#define CHECK_ERROR(val1,val2, msg)         if (val1==val2) { perror(msg); exit(EXIT_FAILURE);}

#define MAXOCTETS 80


int main_communication_tcp_controleur()
{
    int sd1 ; //socket de dialogue
    
    struct sockaddr_in adrlect; //adresse du lecteur
    
    int erreur; 
    int nbcars; 
    
    char buff[MAXOCTETS];
    
    //Etape 1 : creation de la socket
    
    sd1=socket(AF_INET, SOCK_STREAM,0); // La socket va utilise TCP
    
    printf("Descripteur de socket = %d \n", sd1); // Inutile
    
    CHECK_ERROR(sd1,-1, "erreur de creation de la socket !!! \n");
    
    // Etape 2  : On complete l'adresse de la socket
    
    adrlect.sin_family=AF_INET;
    adrlect.sin_port=htons(TCP_PORT);
    adrlect.sin_addr.s_addr=inet_addr(CONTROLEUR_IP);
    
    // Etape 3 : connexion vers le serveur
    
    erreur=connect(sd1, (const struct sockaddr *) &adrlect, sizeof(adrlect));
    
    CHECK_ERROR(erreur,-1, "La connexion n'a pas ete ouverte !!! \n");

    // Etape 4 : dialogue unidirectionnel vers le serveur
    do
    {
    
    printf("ECRIVAIN>");
    
    //Etape 4 : Envoi de data
    
    fgets(buff, MAXOCTETS, stdin);
    buff[strlen(buff)-1]='\0';
    
    nbcars=send(sd1, buff, strlen(buff)+1, 0); 
    }
    while (strcmp(buff, "fin")!=0);
    
    printf("Toucher une touche pour quitter l'application !!! \n");
    
    getchar();
   
    close(sd1);
    
      
return 1;
    
}
