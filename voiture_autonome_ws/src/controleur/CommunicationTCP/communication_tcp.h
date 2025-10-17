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
extern VoitureConnection* voitures_list[MAX_VOITURES];
extern int nb_voitures;

VoitureConnection* get_voiture_by_id(int id_voiture);
VoitureConnection* get_voiture_by_sockfd(int sockfd);

/* === Fonctions génériques === */
int sendMessage(int sockfd, MessageType type, void* message);
int recvMessage(int sockfd, MessageType* type, void* message);

/* === Fonctions spécialisées === */
int sendConsigne(int sockfd, const Consigne* cons);
int sendItineraire(int sockfd, const Itineraire* iti);
int sendFin(int sockfd);

int sendMessage(int Id, MessageType type, void* message);

int recvPositionVoiture(int sockfd, PositionVoiture* pos);
int recvDemande(int sockfd, Demande* dem);
int recvFin(int sockfd, char* buffer, size_t max_size);

void* receive_thread(void* arg);

void afficher_voitures_connectees();

void deconnecter_voiture(VoitureConnection* v);