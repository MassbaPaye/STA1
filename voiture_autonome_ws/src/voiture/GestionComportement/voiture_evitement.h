#ifndef VOITURE_EVITEMENT_H
#define VOITURE_EVITEMENT_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file voiture_evitement.h
 * @brief Module de réaction locale (stop/ralentir/contournement).
 *
 * Fonction d'orchestration principale (interface publique unique).
 * Implémentation dans voiture_evitement.c (aucune dépendance requise côté interface).
 */

/**
 * @brief Orchestration complète d'évitement.
 *
 * Étapes (résumé) :
 *  0) Si la trajectoire actuelle passe, ne rien faire (option : ralentir).
 *  1) Décider stop/ralentir selon la distance locale à l’obstacle.
 *  2) Convertir l’obstacle en repère absolu.
 *  3) Choisir le côté de contournement et vérifier la faisabilité.
 *  4) Générer la trajectoire et la publier.
 *
 * @return 0 si OK,
 *        -1 si données capteurs indisponibles,
 *        -2 si contournement impossible (trop étroit) → stop publié,
 *        -3 si marquage indisponible → stop publié.
 */
int voiture_evitement_main(void);

#ifdef __cplusplus
}
#endif

#endif /* VOITURE_EVITEMENT_H */
