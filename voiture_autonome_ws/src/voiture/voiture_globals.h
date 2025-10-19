// Ce fichier contient uniquement les variables globales propres au processus "voiture". 
// Aucune donnée n'est partagée avec le processus "controleur".

#ifndef VOITURE_GLOBALS_H
#define VOITURE_GLOBALS_H

#include <pthread.h>
#include <time.h>
#include "messages.h"   // inclut Trajectoire, Itineraire, Consigne, etc.

extern const struct timespec TIMESPEC_UNDEFINED;

/* ==== Prototypes des fonctions ==== */

// Itineraire
int set_itineraire(const Itineraire* t, int blocking);
int get_itineraire(Itineraire* t, int blocking);
struct timespec get_itineraire_last_update(void);

// Consigne
int set_consigne(const Consigne* t, int blocking);
int get_consigne(Consigne* t, int blocking);
struct timespec get_consigne_last_update(void);

// Demande
int set_demande(const Demande* t, int blocking);
int get_demande(Demande* t, int blocking);
struct timespec get_demande_last_update(void);

// EtatVoiture
int set_etat(const EtatVoiture* t, int blocking);
int get_etat(EtatVoiture* t, int blocking);
struct timespec get_etat_last_update(void);

// PositionVoiture
int set_position(const PositionVoiture* t, int blocking);
int get_position(PositionVoiture* t, int blocking);
struct timespec get_position_last_update(void);

// Trajectoire
int set_trajectoire(const Trajectoire* t, int blocking);
int get_trajectoire(Trajectoire* t, int blocking);
struct timespec get_trajectoire_last_update(void);

// DonneesDetection
int set_donnees_detection(const DonneesDetection* t, int blocking);
int get_donnees_detection(DonneesDetection* t, int blocking);
struct timespec get_donnees_detection_last_update(void);

// SensorData
int set_sensor_data(const SensorData* t, int blocking);
int get_sensor_data(SensorData* t, int blocking);
struct timespec get_sensor_data_last_update(void);

void init_voiture_globals(void);



/* ==== Définition des variables globales ==== */
typedef struct {
    pthread_mutex_t mutex;
    Itineraire data;
    struct timespec last_update;
} GlobalItineraire;

typedef struct {
    pthread_mutex_t mutex;
    Consigne data;
    struct timespec last_update;
} GlobalConsigne;

typedef struct {
    pthread_mutex_t mutex;
    Demande data;
    struct timespec last_update;
} GlobalDemande;

typedef struct {
    pthread_mutex_t mutex;
    EtatVoiture data;
    struct timespec last_update;
} GlobalEtat;

typedef struct {
    pthread_mutex_t mutex;
    PositionVoiture data;
    struct timespec last_update;
} GlobalPosition;

typedef struct {
    pthread_mutex_t mutex;
    Trajectoire data;
    struct timespec last_update;
} GlobalTrajectoire;

typedef struct {
    pthread_mutex_t mutex;
    DonneesDetection data;
    struct timespec last_update;
} GlobalDonneesDetection;

typedef struct {
    pthread_mutex_t mutex;
    SensorData data;
    struct timespec last_update;
} GlobalSensorData;

/* ==== Instance unique de toutes les variables globales ==== */
typedef struct {
    GlobalItineraire itineraire;
    GlobalConsigne consigne;
    GlobalDemande demande;
    GlobalEtat etat_voiture;
    GlobalPosition position_voiture;
    GlobalTrajectoire trajectoire;
    GlobalDonneesDetection donnees_detection;
    GlobalSensorData sensor_data;
} GlobalsVoiture;
extern GlobalsVoiture g;



#endif
