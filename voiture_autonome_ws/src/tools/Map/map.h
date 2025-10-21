#ifndef MAP_H
#define MAP_H

#include <stddef.h>

typedef struct Arc Arc; // forward declaration

typedef struct Node {
    int id;
    double x, y;
    int n_out_arcs; // nombre d'arcs sortants
    Arc **out_arcs;    // tableau de pointeurs vers les arcs sortants
} Node;

struct Arc {
    int id;
    Node *u;        // noeud de départ
    Node *v;        // noeud d'arrivée
    double radius;
    double length;
    double cx, cy;
};

typedef struct {
    int version;
    int n_nodes;
    Node *nodes;
    int n_arcs;
    Arc *arcs;
} Graph;


// --- Fonctions ---
Graph *load_graph(const char *nodes_file, const char *arcs_file);
void free_graph(Graph *g);

#endif // MAP_H
