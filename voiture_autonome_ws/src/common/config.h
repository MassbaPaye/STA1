#ifndef CONFIG_H
#define CONFIG_H


// === Configuration réseau du controleur ===
#define TCP_PORT              5000
#define CONTROLEUR_IP         "127.0.0.1"


// === Configuration des ports USB ===
#define USE_MARVELMIND           1
#define DEFAULT_MEGAPI_PORT      "stdin"
#define DEFAULT_MARVELMIND_PORT  "/dev/ttyACM0"
#define MEGAPI_BAUDRATE          115200


// === Paramètres système ===
#define MAX_VOITURES 2 // Nombre de voiture maximal qui peuvent etre géré par le controleur
#define MAX_VITESSE 100 // [mm/s]


// === Paramètres Types/Messages ===
#define MAX_POINTS_TRAJECTOIRE 5
#define MAX_POINTS_MARQUAGE 64
#define MAX_PANNEAUX_SIMULTANES 5

// === Parametre géométriques de la voiture ===
#define RAYON_ROUE 30.0 // mm
#define ECARTEMENT_ROUE 150 // mm

// === Coefficients de fusion de données de localisation ===
#define FUSION_ODO_ALPHA 0.5   // Pondération de l'historique de la position marvelmind lors d'un nouvelle position
#define FUSION_POSITION_GLOBALE_WEIGHT 0.8  // Pondération odométrie dans la fusion avec les données marvelmind

#endif