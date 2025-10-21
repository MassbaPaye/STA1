// Ce fichier contient uniquement les variables globales propres au processus "controleur". 
// Aucune donnée n'est partagée avec le processus "voiture".

#ifndef CONTROLEUR_GLOBALS_H
#define CONTROLEUR_GLOBALS_H

#include <pthread.h>
#include <time.h>
#include <stdbool.h>
#include "messages.h"
#include "queue.h"
#include "config.h"  // contient MAX_VOITURES

extern const struct timespec TIMESPEC_UNDEFINED;

/* ==== Structure Voiture ==== */
typedef struct {
    pthread_mutex_t mutex;    // protège toutes les données de cette voiture
    EtatVoiture etat;
    PositionVoiture position;
    Itineraire itineraire;
    struct timespec etat_last_update;
    struct timespec position_last_update;
    struct timespec itineraire_last_update;
} Voiture;

/* ==== Variables globales ==== */
extern Voiture voitures[MAX_VOITURES];
extern Queue demandes_queue;

/* ==== Fonctions d'accès aux variables des voitures ==== */

// Etat
int set_voiture_etat(int id, const EtatVoiture* etat);
int get_voiture_etat(int id, EtatVoiture* etat);
struct timespec get_voiture_etat_last_update(int id);

// Position
int set_voiture_position(int id, const PositionVoiture* pos);
int get_voiture_position(int id, PositionVoiture* pos);
struct timespec get_voiture_position_last_update(int id);

// Itineraire
int set_voiture_itineraire(int id, const Itineraire* itin);
int get_voiture_itineraire(int id, Itineraire* itin);
struct timespec get_voiture_itineraire_last_update(int id);

/* ==== Fonctions pour la queue globale ==== */
int enqueue_demande(const Demande* d);
int dequeue_demande(Demande* d);
int size_demande_queue(void);

/* ==== Initialisation globale ==== */
void init_controleur_globals(void);

#endif
