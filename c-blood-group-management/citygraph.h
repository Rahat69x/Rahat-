#ifndef CITYGRAPH_H
#define CITYGRAPH_H

#include "common.h"
#include "utils.h"

/*
 * citygraph.h - Feature F1: Weighted undirected graph and Dijkstra nearest-donor ranking.
 * Layer 3 structure module: does NOT own Donor memory and NEVER includes context.h.
 * Fixed array of up to MAX_CITIES (30) with adjacency lists.
 */

typedef struct Edge {
    int dest;                   /* Destination city index (0..city_count-1) */
    int weight_km;              /* Road distance in kilometers (1..MAX_ROAD_KM) */
    struct Edge *next;          /* Next edge in adjacency list */
} Edge;

typedef struct CityGraph {
    int city_count;                             /* Current number of registered cities */
    char names[MAX_CITIES][MAX_CITY_LEN];       /* City names array */
    Edge *adj[MAX_CITIES];                      /* Adjacency list heads */
} CityGraph;

/*
 * Initializes an empty CityGraph.
 * Time: O(MAX_CITIES), Space: O(1)
 */
void citygraph_init(CityGraph *g);

/*
 * Adds a city to the graph or returns its existing index.
 * Returns city index (0..MAX_CITIES-1) on success, -1 if graph is full (MAX_CITIES).
 * Time: O(city_count), Space: O(1).
 */
int citygraph_add_city(CityGraph *g, const char *name);

/*
 * Finds city index by name.
 * Returns city index if found, or -1 if not found.
 * Time: O(city_count), Space: O(1).
 */
int citygraph_find_city(const CityGraph *g, const char *name);

/*
 * Adds an undirected weighted edge between city_a and city_b with road distance in km.
 * Returns ST_OK on success, ST_ERR_INVALID on bad arguments or cities not found,
 * ST_ERR_NOMEM if Edge allocation fails.
 * Time: O(degree), Space: O(1) heap allocation.
 */
Status citygraph_add_road(CityGraph *g, const char *city_a, const char *city_b, int km);

/*
 * Computes shortest road distances from src city to all other cities using Dijkstra's algorithm.
 * dist[src] = 0; unreachable cities receive INF_KM.
 * Big-O: Time O(V^2 + E), Space O(V).
 */
void citygraph_dijkstra(const CityGraph *g, int src, int dist[MAX_CITIES]);

/*
 * Loads standard Bangladesh road network with major divisional and regional cities.
 */
void citygraph_load_default_network(CityGraph *g);

/*
 * Deallocates all Edge nodes in the graph and resets city_count to 0.
 * Time: O(V + E), Space: O(1).
 */
void citygraph_free(CityGraph *g);

#endif /* CITYGRAPH_H */
