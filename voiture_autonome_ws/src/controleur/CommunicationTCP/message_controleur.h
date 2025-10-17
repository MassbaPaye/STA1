#include <stdint.h>
#include <pthread.h>  
#include "config.h"
#include "messages.h"

/* --- Mutex global pour protéger la table des voitures --- */
extern pthread_mutex_t mutex_voitures;

/* --- Structure pour gérer une connexion voiture --- */
typedef struct {
    int sockfd;         // socket associée à la voiture
    int id_voiture;     // identifiant de la voiture
    pthread_t tid;      // thread qui gère cette connexion
} VoitureConnection;

/* --- Tableau global des connexions et compteur --- */
extern VoitureConnection voitures_list[MAX_VOITURES];
extern int nb_voitures;

/* === Fonctions génériques === */
int sendMessage(int sockfd, MessageType type, void* message);
int recvMessage(int sockfd, MessageType* type, void* message);

/* === Fonctions spécialisées === */
int sendPositionVoiture(int sockfd, const PositionVoiture* pos);
int sendDemande(int sockfd, const Demande* dem);
int sendConsigne(int sockfd, const Consigne* cons);
int sendItineraire(int sockfd, const Itineraire* iti);

int recvPositionVoiture(int sockfd, PositionVoiture* pos);
int recvDemande(int sockfd, Demande* dem);
int recvConsigne(int sockfd, Consigne* cons);
int recvItineraire(int sockfd, Itineraire* iti);

int trouver_socket(int id_voiture);
