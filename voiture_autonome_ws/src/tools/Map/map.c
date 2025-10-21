#include "map.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// --- Fonctions auxiliaires ---
static double parse_double(const char *s) {
    if (!s || strlen(s) == 0) return NAN;
    if (strcmp(s, "inf") == 0) return INFINITY;
    return atof(s);
}

// --- Chargement du graphe depuis CSV ---
Graph *load_graph(const char *nodes_file, const char *arcs_file) {
    FILE *f;
    char line[512];
    int n_nodes = 0, n_arcs = 0;
    Node *nodes = NULL;
    Arc *arcs = NULL;

    // --- Lecture des nœuds ---
    f = fopen(nodes_file, "r");
    if (!f) return NULL;

    fgets(line, sizeof(line), f); // skip header
    while (fgets(line, sizeof(line), f)) n_nodes++;

    rewind(f);
    fgets(line, sizeof(line), f); // skip header

    nodes = malloc(n_nodes * sizeof(Node));
    int idx = 0;
    while (fgets(line, sizeof(line), f)) {
        int id;
        double x, y;
        if (sscanf(line, "%d,%lf,%lf", &id, &x, &y) == 3) {
            nodes[idx].id = id;
            nodes[idx].x = x;
            nodes[idx].y = y;
            nodes[idx].n_out_arcs = 0;
            nodes[idx].out_arcs = NULL;
            idx++;
        }
    }
    fclose(f);

    // --- Lecture des arcs ---
    f = fopen(arcs_file, "r");
    if (!f) {
        free(nodes);
        return NULL;
    }

    fgets(line, sizeof(line), f); // skip header
    while (fgets(line, sizeof(line), f)) n_arcs++;
    rewind(f);
    fgets(line, sizeof(line), f); // skip header

    arcs = malloc(n_arcs * sizeof(Arc));
    idx = 0;
    while (fgets(line, sizeof(line), f)) {
        int u_id, v_id;
        double radius, length, cx, cy;
        cx = cy = NAN;

        char buf_radius[64], buf_length[64], buf_cx[64], buf_cy[64];
        if (sscanf(line, "%d,%d,%63[^,],%63[^,],%63[^,],%63[^,\n]",
                   &u_id, &v_id, buf_radius, buf_length, buf_cx, buf_cy) >= 4) {
            radius = parse_double(buf_radius);
            length = parse_double(buf_length);
            if (strlen(buf_cx) > 0) cx = atof(buf_cx);
            if (strlen(buf_cy) > 0) cy = atof(buf_cy);

            // Trouver pointeurs vers les nœuds
            Node *u = NULL, *v = NULL;
            for (int i = 0; i < n_nodes; i++) {
                if (nodes[i].id == u_id) u = &nodes[i];
                if (nodes[i].id == v_id) v = &nodes[i];
            }
            if (!u || !v) continue;

            arcs[idx].id = idx;
            arcs[idx].u = u;
            arcs[idx].v = v;
            arcs[idx].radius = radius;
            arcs[idx].length = length;
            arcs[idx].cx = cx;
            arcs[idx].cy = cy;
            idx++;

            // Compter arcs sortants
            u->n_out_arcs++;
        }
    }
    fclose(f);

    // --- Allocation des tableaux d'arcs sortants ---
    for (int i = 0; i < n_nodes; i++) {
        if (nodes[i].n_out_arcs > 0) {
            nodes[i].out_arcs = malloc(nodes[i].n_out_arcs * sizeof(Arc*));
            nodes[i].n_out_arcs = 0; // reset pour remplissage
        }
    }

    // --- Remplissage des out_arcs ---
    for (int i = 0; i < n_arcs; i++) {
        Arc *a = &arcs[i];
        Node *u = a->u;
        u->out_arcs[u->n_out_arcs++] = a;
    }

    Graph *g = malloc(sizeof(Graph));
    g->version = 1;
    g->nodes = nodes;
    g->n_nodes = n_nodes;
    g->arcs = arcs;
    g->n_arcs = n_arcs;
    return g;
}

// --- Libération mémoire ---
void free_graph(Graph *g) {
    if (!g) return;
    if (g->nodes) {
        for (int i = 0; i < g->n_nodes; i++)
            free(g->nodes[i].out_arcs);
        free(g->nodes);
    }
    free(g->arcs);
    free(g);
}


#include <float.h> // pour DBL_MAX

// --- Fonction Dijkstra ---
ShortestPath dijkstra(const Graph *g, int src_id, int dst_id) {
    ShortestPath sp = {0, NULL, -1.0};
    if (!g) return sp;

    int n = g->n_nodes;
    double *dist = malloc(n * sizeof(double));
    int *prev = malloc(n * sizeof(int));
    int *visited = calloc(n, sizeof(int));

    for (int i = 0; i < n; i++) {
        dist[i] = DBL_MAX;
        prev[i] = -1;
    }

    // Trouver indices des nœuds source et destination
    int src_idx = -1, dst_idx = -1;
    for (int i = 0; i < n; i++) {
        if (g->nodes[i].id == src_id) src_idx = i;
        if (g->nodes[i].id == dst_id) dst_idx = i;
    }
    if (src_idx == -1 || dst_idx == -1) goto cleanup;

    dist[src_idx] = 0.0;

    // --- Boucle principale ---
    for (int iter = 0; iter < n; iter++) {
        // Chercher le nœud non visité avec distance minimale
        double min_dist = DBL_MAX;
        int u_idx = -1;
        for (int i = 0; i < n; i++) {
            if (!visited[i] && dist[i] < min_dist) {
                min_dist = dist[i];
                u_idx = i;
            }
        }
        if (u_idx == -1) break; // aucun nœud restant
        visited[u_idx] = 1;

        Node *u = &g->nodes[u_idx];
        // Parcourir les arcs sortants
        for (int k = 0; k < u->n_out_arcs; k++) {
            Arc *a = u->out_arcs[k];
            Node *v = a->v;

            // Trouver index de v
            int v_idx = 0;
            for (; v_idx < n; v_idx++) if (&g->nodes[v_idx] == v) break;

            double alt = dist[u_idx] + a->length;
            if (alt < dist[v_idx]) {
                dist[v_idx] = alt;
                prev[v_idx] = u_idx;
            }
        }
    }

    // --- Reconstituer le chemin ---
    if (dist[dst_idx] < DBL_MAX) {
        // compter le nombre de nœuds
        int count = 0;
        for (int i = dst_idx; i != -1; i = prev[i]) count++;
        sp.n_path = count;
        sp.path = malloc(count * sizeof(Node*));

        int idx = (int)count - 1;
        for (int i = dst_idx; i != -1; i = prev[i]) {
            sp.path[idx--] = &g->nodes[i];
        }
        sp.distance = dist[dst_idx];
    }

cleanup:
    free(dist);
    free(prev);
    free(visited);
    return sp;
}

void free_shortest_path(ShortestPath *sp) {
    if (!sp) return;
    free(sp->path);
    sp->n_path = 0;
    sp->path = NULL;
    sp->distance = -1.0;
}
