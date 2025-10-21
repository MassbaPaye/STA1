// graph.h

#ifndef GRAPH_H
#define GRAPH_H

#include <stddef.h>

typedef struct {
    int id;
    double x, y;
} Node;

typedef struct {
    int id;          // identifiant unique de l'arc
    int u;           // index du noeud de départ
    int v;           // index du noeud d'arrivée
    double radius;   // rayon (infini = segment droit, signe = sens)
    double length;   // longueur de l'arc (métrique)
    double cx, cy;   // centre du cercle (optionnel si segment : peut être NAN)
} Arc;

typedef struct {
    int version;
    size_t n_nodes;
    Node *nodes;
    size_t n_arcs;
    Arc *arcs;
} Graph;

#endif // GRAPH_H
