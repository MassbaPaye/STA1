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

    free_graph(graph);
    return 0;
}