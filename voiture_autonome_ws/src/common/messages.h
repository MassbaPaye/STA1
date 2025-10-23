#ifndef MESSAGES_H
#define MESSAGES_H

//#include <stdbool.h>
#include "config.h"

/* Enumération pour le type de message échangé par TCP */
typedef enum {
    MESSAGE_POSITION = 1,
    MESSAGE_DEMANDE  = 2,
    MESSAGE_CONSIGNE = 3,
    MESSAGE_ITINERAIRE = 4,
    MESSAGE_FIN = 5,
    MESSAGE_ETAT = 6
} MessageType;

/* Enumération pour le type de demande Voiture->Controleur routier */
typedef enum {
    LIBERATION_STRUCTURE = 0,
    RESERVATION_STRUCTURE = 1
} DemandeType;

/* Enumération pour le type de consigne Controleur->Voiture (suite à une demande) */
typedef enum {
    CONSIGNE_AUTORISATION = 0,
    CONSIGNE_ATTENTE = 1
} ConsigneType;

typedef enum {
    OBSTACLE_VOITURE = 0,
    PANNEAU_LIMITATION_30 = 1,
    PANNEAU_BARRIERE = 2,
    PANNEAU_CEDER_PASSAGE = 3,
    PANNEAU_FIN_30 = 4,
    PANNEAU_INTERSECTION = 5,
    PANNEAU_PARKING = 6,
    PANNEAU_SENS_UNIQUE = 7,
    PONT = 8
} ObstacleType;

// === TYPES UTILITAIRES ===

/*  Type Utilitaire : Point
    Description : Le type Point représente un point dans l'espace + un angle.
    Utilisation : Il peut etre utilisé comme
     - simple coordonnée (x, y) ou (x, y, z)
     - Position d'une voiture (x, y, theta) ou (x, y, z, theta)
*/
typedef struct {
    float x; // mm
    float y; // mm
    float z; // mm
    float theta; // degrés
} Point;


// === MESSAGES COMMUNIQUES PAR TCP ===

/*  Message TCP C/V : Demande 
    Tache émetrice : Gestion de comportement
    Tache réceptrice : Controleur routier
    Description : Permet à une voiture de formuler une demande de réservation
        ou le libération d'une structure auprès du controleur routier
*/
typedef struct {
    int structure_id;
    DemandeType type; // LIBERATION_STRUCTURE ou RESERVATION_STRUCTURE
    char direction; // ex: "n" ou "s"
} Demande;

/*  Message TCP C/V : Itinéraire 
    Tache émetrice : Planificateur d'itinéraire
    Tache réceptrice : Gestion de comportement
    Description : Sert à communiquer à une voiture l'itinéraire qu'elle doit suivre.
    /!\ Mémoire : Planificateur d'itinéraire alloue le tableau avec malloc 
        et Gestion de comportement le libère
*/

typedef struct {
    float x; // mm
    float y; // mm
    float z; // mm
    float theta; // degrés
    int pont;
    int depacement;
} Point_iti;

typedef struct {
    int nb_points;
    Point_iti* points;
} Itineraire;

// Le type Point sera surement redéfini avec les metadonnées de la carte
// Certains champs seront peut-etre ajoutés (ex: position de parking, nécessité de se garer)


/*  Message TCP C/V : PositionVoiture 
    Tache émetrice : Localisation
    Tache réceptrice : Planificateur d'itinéraire + IHM
    Description : Permet aux voiture de partager leur position + vitesse au
        planificateur d'itinéraire et l'utilisateur (via l'IHM) 
*/
typedef struct {
    float x; // mm
    float y; // mm
    float z; // mm
    float theta; // degrés
    float vx; // mm/s
    float vy; // mm/s
    float vz; // mm/s
} PositionVoiture;


/*  Message TCP C/V : Consigne 
    Tache émetrice : Controleur routier
    Tache réceptrice : Gestion de comportement
    Description : Permet au controleur routier de répondre à la demande d'une voiture
*/
typedef struct {
    int structure_id;
    ConsigneType autorisation; // CONSIGNE_AUTORISATION ou CONSIGNE_ATTENTE
} Consigne;


// === TYPES COMMUNICATION INTERNES ===
// Permettent d'échanger entre les taches au sein du meme processus

/*  Type Communication Interne : Trajectoire 
    Tache écrivaine : Gestion de comportement
    Tache lectrice : Suiveur de trajectoire
    Description : Donne les points que la voiture doit suivre, la vitesse, et si la voiture 
        doit se garer.
*/


typedef struct {
    int nb_points;
    Point_iti points[MAX_POINTS_TRAJECTOIRE]; // Tableau des points absolus représentant la trajectoire que la voiture doit suivre
    float vitesse; // [mm/s] vitesse vers laquelle la voiture doit converger
    float vitesse_max; // [mm/s] vitesse maximale autorisée que la voiture ne doit jamais dépasser
    int arreter_fin; // Indique si la voiture doit conserver sa vitesse ou si elle doit prévoir de l'arreter  
} Trajectoire;


/*  Type Utilitaire : Panneau 
    Description : Indique la présence d'un panneau sur la voie
*/


/*  Type Utilitaire : Obstacle 
    Description : Indique la présence d'un obstcale sur la voie
*/
typedef struct {
    ObstacleType type;
    Point pointd;
    Point pointg;
} Obstacle;

/*  Type Utilitaire : MarquageSol 
    Description : Position des lignes de marquage au sol sur la route
    /!\ Mémoire : Il faut libérer les lignes de point
*/
typedef struct {
    int nb_points_gauche;
    int nb_points_droite;
    Point ligne_gauche[MAX_POINTS_MARQUAGE];
    Point ligne_droite[MAX_POINTS_MARQUAGE];
} MarquageSol;

/*  Message TCP IA/V : DonneesDetection 
    Tache émetrice/ecrivaine : Detection Environnement
    Tache réceptrice/lectrice : Gestion de comportement
    Description : Message envoyé par l'IA de la voiture au processus voiture informant 
        d'un eventuel obstacle, de la position du marquage au sol et des panneaux présents. 
    /!\ Mémoire : Il faut libérer les lignes de point dans marquage_sol
*/
typedef struct {
    int count;
    Obstacle obstacle[MAX_OBSTACLES_SIMULTANES];
    MarquageSol marquage_sol;
} DonneesDetection;


typedef struct {
    int overrun;
    float ref1, ref2;
    float speed1, speed2;
    float angle;
    float vfiltre1, vfiltre2;
} SensorData;


// Annexe : Messages pas utiles pour le moment

/*  Message TCP IA/V : EtatVoiture 
    Tache émetrice : Gestion de comportement
    Tache réceptrice : IHM
    Description : Permet d'afficher des erreurs sur l'interface utilisateur
*/
typedef struct {
    int batterie;
    int status_code; // indique la présence d'une erreur
    char message[256]; // Message d'erreur
} EtatVoiture;

#endif
