#ifndef MARVELMIND_MANAGER_H
#define MARVELMIND_MANAGER_H

#include <stdbool.h>
#include <time.h>

typedef struct {
    float x;
    float y;
    float z;
    struct timespec t;
    bool valid;
    bool is_new;
} MarvelmindPosition;

// Démarre le thread Marvelmind (retourne 0 si OK)
int lancer_marvelmind();

// Arrête le thread et ferme la connexion
void stop_marvelmind();

// Attend une nouvelle position (timeout en secondes, -1 = illimité)
// retourne 0 si nouvelle position reçue, -1 en cas de timeout
int wait_for_position(int timeout_sec);

// Renvoie la dernière position enregistrée 
MarvelmindPosition get_marvelmind_position(bool change_to_read);
void _set_marvelmind_position(MarvelmindPosition pos);

#endif // MARVELMIND_MANAGER_H
