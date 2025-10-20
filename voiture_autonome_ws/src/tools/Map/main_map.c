#include "map.h"
#include <stdio.h>

#define NODES_FILE "nodes.csv"
#define ARCS_FILE "arcs_oriented.csv"

Graph* graph;

int main(void)
{
    graph = load_graph(NODES_FILE, ARCS_FILE);
    if (!graph) return 1;
    printf("n_arcs=%d, n_nodes=%d\n", graph->n_arcs, graph->n_nodes);

    ShortestPath sp = dijkstra(graph, 0, 10); // du nœud 0 au nœud 10
    if (sp.n_path > 0) {
        printf("Distance minimale : %.3f\n", sp.distance);
        printf("Chemin : ");
        for (size_t i = 0; i < sp.n_path; i++)
            printf("%d ", sp.path[i]->id);
        printf("\n");
    } else {
        printf("Aucun chemin trouvé\n");
    }

    free_shortest_path(&sp);
    free_graph(graph);
    return 0;
}