#include "citygraph.h"

/*
 * citygraph.c - Implementation of Feature F1: City Road Graph & Dijkstra Algorithm.
 */

/* Time: O(MAX_CITIES), Space: O(1) */
void citygraph_init(CityGraph *g) {
    if (!g) return;
    g->city_count = 0;
    for (int i = 0; i < MAX_CITIES; i++) {
        g->names[i][0] = '\0';
        g->adj[i] = NULL;
    }
}

/* Time: O(city_count), Space: O(1) */
int citygraph_find_city(const CityGraph *g, const char *name) {
    if (!g || !name) return -1;
    for (int i = 0; i < g->city_count; i++) {
        if (utils_strcasecmp(g->names[i], name) == 0) {
            return i;
        }
    }
    return -1;
}

/* Time: O(city_count), Space: O(1) */
int citygraph_add_city(CityGraph *g, const char *name) {
    if (!g || !name) return -1;
    if (!utils_is_valid_text(name, MAX_CITY_LEN - 1)) {
        return -1;
    }

    int existing = citygraph_find_city(g, name);
    if (existing != -1) {
        return existing;
    }

    if (g->city_count >= MAX_CITIES) {
        return -1; /* Capacity limit */
    }

    int idx = g->city_count;
    utils_copy_string(g->names[idx], name, sizeof(g->names[idx]));
    g->adj[idx] = NULL;
    g->city_count++;

    return idx;
}

/* Time: O(degree), Space: O(1) */
static Status add_directed_edge(CityGraph *g, int src, int dest, int km) {
    Edge *e = (Edge *)malloc(sizeof(Edge));
    if (!e) return ST_ERR_NOMEM;

    e->dest = dest;
    e->weight_km = km;
    e->next = g->adj[src];
    g->adj[src] = e;

    return ST_OK;
}

/* Time: O(degree), Space: O(1) heap allocation */
Status citygraph_add_road(CityGraph *g, const char *city_a, const char *city_b, int km) {
    if (!g || !city_a || !city_b || km < 1 || km > MAX_ROAD_KM) {
        return ST_ERR_INVALID;
    }

    int idx_a = citygraph_add_city(g, city_a);
    int idx_b = citygraph_add_city(g, city_b);
    if (idx_a == -1 || idx_b == -1) {
        return ST_ERR_LIMIT;
    }

    if (idx_a == idx_b) {
        return ST_OK; /* Self-loop */
    }

    /* Add undirected edge: A -> B and B -> A */
    Status st1 = add_directed_edge(g, idx_a, idx_b, km);
    if (st1 != ST_OK) return st1;

    Status st2 = add_directed_edge(g, idx_b, idx_a, km);
    if (st2 != ST_OK) {
        /* Rollback first edge */
        Edge *top = g->adj[idx_a];
        g->adj[idx_a] = top->next;
        free(top);
        return st2;
    }

    return ST_OK;
}

/* Time: O(V^2 + E), Space: O(V) */
void citygraph_dijkstra(const CityGraph *g, int src, int dist[MAX_CITIES]) {
    if (!g || !dist) return;

    int visited[MAX_CITIES];
    for (int i = 0; i < MAX_CITIES; i++) {
        dist[i] = INF_KM;
        visited[i] = 0;
    }

    if (src < 0 || src >= g->city_count) {
        return;
    }

    dist[src] = 0;

    for (int step = 0; step < g->city_count; step++) {
        /* Find unvisited vertex with smallest distance */
        int min_dist = INF_KM;
        int u = -1;

        for (int i = 0; i < g->city_count; i++) {
            if (!visited[i] && dist[i] < min_dist) {
                min_dist = dist[i];
                u = i;
            }
        }

        if (u == -1 || min_dist == INF_KM) {
            break; /* Remaining vertices unreachable */
        }

        visited[u] = 1;

        /* Relax adjacent edges */
        const Edge *curr = g->adj[u];
        while (curr) {
            int v = curr->dest;
            int weight = curr->weight_km;

            if (!visited[v] && dist[u] + weight < dist[v]) {
                dist[v] = dist[u] + weight;
            }
            curr = curr->next;
        }
    }
}

/* Time: O(E), Space: O(V + E) */
void citygraph_load_default_network(CityGraph *g) {
    if (!g) return;
    citygraph_init(g);

    /* National Highway Network (Major Bangladeshi Cities) */
    citygraph_add_road(g, "Dhaka", "Comilla", 97);
    citygraph_add_road(g, "Comilla", "Chittagong", 150);
    citygraph_add_road(g, "Chittagong", "Cox's Bazar", 152);
    citygraph_add_road(g, "Dhaka", "Mymensingh", 120);
    citygraph_add_road(g, "Dhaka", "Tangail", 85);
    citygraph_add_road(g, "Tangail", "Bogra", 110);
    citygraph_add_road(g, "Bogra", "Rangpur", 105);
    citygraph_add_road(g, "Bogra", "Rajshahi", 108);
    citygraph_add_road(g, "Dhaka", "Faridpur", 75);
    citygraph_add_road(g, "Faridpur", "Barisal", 125);
    citygraph_add_road(g, "Faridpur", "Jessore", 95);
    citygraph_add_road(g, "Jessore", "Khulna", 60);
    citygraph_add_road(g, "Dhaka", "Sylhet", 240);
    citygraph_add_road(g, "Comilla", "Sylhet", 215);
}

/* Time: O(V + E), Space: O(1) */
void citygraph_free(CityGraph *g) {
    if (!g) return;
    for (int i = 0; i < g->city_count; i++) {
        Edge *curr = g->adj[i];
        while (curr) {
            Edge *next_edge = curr->next;
            free(curr);
            curr = next_edge;
        }
        g->adj[i] = NULL;
    }
    g->city_count = 0;
}
