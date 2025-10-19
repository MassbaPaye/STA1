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


#endif