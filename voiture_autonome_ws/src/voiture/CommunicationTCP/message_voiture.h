#ifndef MESSAGES_H
#define MESSAGES_H

#include <stdint.h>
#include <pthread.h>   // pour pthread_mutex_t et pthread_t
#include "config.h"

/* --- Mutex global pour protéger la table des voitures --- */
extern pthread_mutex_t mutex_voitures;

/* --- Structure pour gérer une connexion voiture --- */
typedef struct {
    int sockfd;         // socket associée à la voiture
    int id_voiture;     // identifiant de la voiture
    pthread_t tid;      // thread qui gère cette connexion
} VoitureConnection;

/* --- Tableau global des connexions et compteur --- */
extern VoitureConnection voitures[MAX_VOITURES];
extern int nb_voitures;

/* Enumération pour le type de message échangé par TCP */
typedef enum {
    MESSAGE_POSITION   = 1,
    MESSAGE_DEMANDE    = 2,
    MESSAGE_CONSIGNE   = 3,
    MESSAGE_ITINERAIRE = 4
} MessageType;

/* Enumération pour le type de demande Voiture -> Contrôleur routier */
typedef enum {
    LIBERATION_STRUCTURE  = 0,
    RESERVATION_STRUCTURE = 1
} DemandeType;

/* Enumération pour le type de consigne Contrôleur -> Voiture */
typedef enum {
    AUTORISATION = 0,
    ATTENTE      = 1
} ConsigneType;

/* Type utilitaire : Point */
typedef struct {
    int x;       // mm
    int y;       // mm
    int z;       // mm
    float theta; // orientation en degrés
} Point;

/* Structures échangées par TCP */
typedef struct {
    int id_voiture;
    int x;
    int y;
    int z;
    float theta;
    float vx;
    float vy;
    float vz;
} PositionVoiture;

typedef struct {
    int id_voiture;
    int structure_id;
    DemandeType type_operation;
    char direction;
} Demande;

typedef struct {
    int id_voiture;
    int id_structure;
    ConsigneType autorisation;
} Consigne;

typedef struct {
    int id_voiture;
    int nb_points;
    Point* points; // tableau dynamique
} Itineraire;

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

#endif // MESSAGES_H
