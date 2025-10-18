#ifndef MESSAGES_H
#define MESSAGES_H

#include <stdbool.h>
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

/* Enumération des types de panneaux */
typedef enum {
    PANNEAU_LIMITATION_30 = 1,
    PANNEAU_DANGER = 2,
    PANNEAU_SENS_UNIQUE = 3,
    PANNEAU_FIN_LIMITATION = 4 // A completer / modifier
} PanneauType;

typedef enum {
    VoitureObstacle = 1,
    PietonObstacle = 2,
    AutreObstacle = 3,
} ObstacleType;

// === TYPES UTILITAIRES ===

/*  Type Utilitaire : Point
    Description : Le type Point représente un point dans l'espace + un angle.
    Utilisation : Il peut etre utilisé comme
     - simple coordonnée (x, y) ou (x, y, z)
     - Position d'une voiture (x, y, theta) ou (x, y, z, theta)
*/
typedef struct {
    int x; // mm
    int y; // mm
    int z; // mm
    float theta; // degrés
} Point;


// === MESSAGES COMMUNIQUES PAR TCP ===

/*  Message TCP : Demande 
    Tache émetrice : Gestion de comportement
    Tache réceptrice : Controleur routier
    Description : Permet à une voiture de formuler une demande de réservation
        ou le libération d'une structure auprès du controleur routier
*/
typedef struct {
    int structure_id;
    int type_operation; // LIBERATION_STRUCTURE ou RESERVATION_STRUCTURE
    char direction; // ex: "n" ou "s"
} Demande;


/*  Message TCP : Itinéraire 
    Tache émetrice : Planificateur d'itinéraire
    Tache réceptrice : Gestion de comportement
    Description : Sert à communiquer à une voiture l'itinéraire qu'elle doit suivre.
    /!\ Mémoire : Planificateur d'itinéraire alloue le tableau avec malloc 
        et Gestion de comportement le libère
*/
typedef struct {
    int nb_points;
    Point* points;
} Itineraire;
// Le type Point sera surement redéfini avec les metadonnées de la carte
// Certains champs seront peut-etre ajoutés (ex: position de parking, nécessité de se garer)


/*  Message TCP : PositionVoiture 
    Tache émetrice : Localisation
    Tache réceptrice : Planificateur d'itinéraire + IHM
    Description : Permet aux voiture de partager leur position + vitesse au
        planificateur d'itinéraire et l'utilisateur (via l'IHM) 
*/
typedef struct {
    int x; // mm
    int y; // mm
    int z; // mm
    float theta; // degrés
    float vx; // mm/s
    float vy; // mm/s
    float vz; // mm/s
} PositionVoiture;


/*  Message TCP : Consigne 
    Tache émetrice : Controleur routier
    Tache réceptrice : Gestion de comportement
    Description : Permet au controleur routier de répondre à la demande d'une voiture
*/
typedef struct {
    int id_structure;
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
    Point points[MAX_POINTS_TRAJECTOIRE]; // Tableau des points absolus représentant la trajectoire que la voiture doit suivre
    float vitesse; // [mm/s] vitesse vers laquelle la voiture doit converger
    float vitesse_max; // [mm/s] vitesse maximale autorisée que la voiture ne doit jamais dépasser
    bool arreter_fin; // Indique si la voiture doit conserver sa vitesse ou si elle doit prévoir de l'arreter  
} Trajectoire;

// Note : Pour l'IA (Obstacle, Panneau, MarquageSol) est-ce qu'on fait un autre processus ? 

/*  A définir : Obstacle 
    Tache émetrice/ecrivaine : Detection Environnement
    Tache réceptrice/lectrice : Gestion de comportement
    Description : Signal un obstacle sur la chaussée
*/

/*  A définir : Obstacle 
    Tache émetrice/ecrivaine : Detection Environnement
    Tache réceptrice/lectrice : Gestion de comportement
    Description : Indique la présence d'un panneau sur la voie
*/
typedef struct {
    int distance;
    PanneauType type;
} Panneau;

typedef struct {
    int distance;
    ObstacleType type;
    Point pointd;
    Point pointg;
} Obstacle;

/*  A définir : MarquageSol 
    Tache émetrice/ecrivaine : Detection Environnement
    Tache réceptrice/lectrice : Gestion de comportement
    Description : Position des lignes de marquage au sol sur la route
    /!\ Mémoire : 
*/
typedef struct {
    int nb_points_gauche;
    int nb_points_droite;
    Point ligne_gauche[MAX_POINTS_MARQUAGE];
    Point ligne_droite[MAX_POINTS_MARQUAGE];
} MarquageSol;

typedef struct {
    bool obstacle_present;
    Obstacle obstacle;
    int nb_panneaux;
    Panneau panneaux[MAX_PANNEAUX_SIMULTANES];
} DonneesDetection;


// Annexe : Messages pas utiles pour le moment

/*  Message TCP : EtatVoiture 
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
