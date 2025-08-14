/**
 * @file spanning_tree.c
 * @brief Spanning tree algorithms implementation
 */

#include "sylves/pathfinding.h"
#include "sylves/memory.h"
#include "sylves/errors.h"
#include <string.h>
#include <stdlib.h>

/* Union-Find (Disjoint Set Union) data structure for Kruskal's algorithm */
typedef struct {
    size_t* parent;
    size_t* rank;
    size_t size;
} UnionFind;

/* Hash table for cell to index mapping */
typedef struct CellIndexEntry {
    SylvesCell cell;
    size_t index;
    struct CellIndexEntry* next;
} CellIndexEntry;

typedef struct {
    CellIndexEntry** buckets;
    size_t bucket_count;
    size_t entry_count;
} CellIndexTable;

/* Edge for sorting */
typedef struct {
    size_t src_index;
    size_t dest_index;
    SylvesEdge edge;
} IndexedEdge;

/* Hash function for cells */
static size_t cell_hash(SylvesCell cell) {
    size_t hash = 0;
    hash ^= (size_t)cell.x + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    hash ^= (size_t)cell.y + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    hash ^= (size_t)cell.z + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    return hash;
}

/* Union-Find operations */
static UnionFind* union_find_create(size_t size) {
    UnionFind* uf = (UnionFind*)sylves_alloc(sizeof(UnionFind));
    if (!uf) return NULL;
    
    uf->parent = (size_t*)sylves_alloc(sizeof(size_t) * size);
    uf->rank = (size_t*)sylves_calloc(size, sizeof(size_t));
    
    if (!uf->parent || !uf->rank) {
        sylves_free(uf->parent);
        sylves_free(uf->rank);
        sylves_free(uf);
        return NULL;
    }
    
    uf->size = size;
    
    // Initialize each element as its own parent
    for (size_t i = 0; i < size; i++) {
        uf->parent[i] = i;
    }
    
    return uf;
}

static void union_find_destroy(UnionFind* uf) {
    if (!uf) return;
    
    sylves_free(uf->parent);
    sylves_free(uf->rank);
    sylves_free(uf);
}

static size_t union_find_find(UnionFind* uf, size_t x) {
    if (uf->parent[x] != x) {
        // Path compression
        uf->parent[x] = union_find_find(uf, uf->parent[x]);
    }
    return uf->parent[x];
}

static bool union_find_union(UnionFind* uf, size_t x, size_t y) {
    size_t root_x = union_find_find(uf, x);
    size_t root_y = union_find_find(uf, y);
    
    if (root_x == root_y) {
        return false; // Already in same set
    }
    
    // Union by rank
    if (uf->rank[root_x] < uf->rank[root_y]) {
        uf->parent[root_x] = root_y;
    } else if (uf->rank[root_x] > uf->rank[root_y]) {
        uf->parent[root_y] = root_x;
    } else {
        uf->parent[root_y] = root_x;
        uf->rank[root_x]++;
    }
    
    return true;
}

/* Cell index table operations */
static CellIndexTable* cell_index_table_create(size_t initial_size) {
    CellIndexTable* table = (CellIndexTable*)sylves_alloc(sizeof(CellIndexTable));
    if (!table) return NULL;
    
    table->buckets = (CellIndexEntry**)sylves_calloc(initial_size, sizeof(CellIndexEntry*));
    if (!table->buckets) {
        sylves_free(table);
        return NULL;
    }
    
    table->bucket_count = initial_size;
    table->entry_count = 0;
    
    return table;
}

static void cell_index_table_destroy(CellIndexTable* table) {
    if (!table) return;
    
    for (size_t i = 0; i < table->bucket_count; i++) {
        CellIndexEntry* entry = table->buckets[i];
        while (entry) {
            CellIndexEntry* next = entry->next;
            sylves_free(entry);
            entry = next;
        }
    }
    
    sylves_free(table->buckets);
    sylves_free(table);
}

static bool cell_index_table_insert(CellIndexTable* table, SylvesCell cell, size_t index) {
    size_t hash_index = cell_hash(cell) % table->bucket_count;
    
    CellIndexEntry* entry = (CellIndexEntry*)sylves_alloc(sizeof(CellIndexEntry));
    if (!entry) return false;
    
    entry->cell = cell;
    entry->index = index;
    entry->next = table->buckets[hash_index];
    table->buckets[hash_index] = entry;
    table->entry_count++;
    
    return true;
}

static bool cell_index_table_find(CellIndexTable* table, SylvesCell cell, size_t* index) {
    size_t hash_index = cell_hash(cell) % table->bucket_count;
    CellIndexEntry* entry = table->buckets[hash_index];
    
    while (entry) {
        if (sylves_cell_equals(entry->cell, cell)) {
            if (index) *index = entry->index;
            return true;
        }
        entry = entry->next;
    }
    
    return false;
}

/* Edge comparison for sorting */
static int edge_compare(const void* a, const void* b) {
    const IndexedEdge* edge_a = (const IndexedEdge*)a;
    const IndexedEdge* edge_b = (const IndexedEdge*)b;
    
    if (edge_a->edge.weight < edge_b->edge.weight) return -1;
    if (edge_a->edge.weight > edge_b->edge.weight) return 1;
    return 0;
}

/* Kruskal's minimum spanning tree algorithm */
SylvesError sylves_kruskal_mst(
    SylvesGrid* grid,
    const SylvesCell* cells,
    size_t cell_count,
    SylvesStepLengthFunc step_lengths,
    void* user_data,
    SylvesEdge** edges,
    size_t* edge_count) {
    
    if (!grid || !cells || cell_count == 0 || !edges || !edge_count) {
        return SYLVES_ERROR_INVALID_ARGUMENT;
    }
    
    // Create cell to index mapping
    CellIndexTable* cell_indices = cell_index_table_create(cell_count * 2);
    if (!cell_indices) {
        return SYLVES_ERROR_OUT_OF_MEMORY;
    }
    
    // Map cells to indices
    for (size_t i = 0; i < cell_count; i++) {
        if (!cell_index_table_insert(cell_indices, cells[i], i)) {
            cell_index_table_destroy(cell_indices);
            return SYLVES_ERROR_OUT_OF_MEMORY;
        }
    }
    
    // Collect all edges
    size_t max_edges = cell_count * 6; // Overestimate
    IndexedEdge* all_edges = (IndexedEdge*)sylves_alloc(sizeof(IndexedEdge) * max_edges);
    if (!all_edges) {
        cell_index_table_destroy(cell_indices);
        return SYLVES_ERROR_OUT_OF_MEMORY;
    }
    
    size_t total_edges = 0;
    
    // Generate edges from each cell
    for (size_t i = 0; i < cell_count; i++) {
        SylvesCell cell = cells[i];
        
        SylvesCellDir dirs_buf[16];
        int n = sylves_grid_get_cell_dirs(grid, cell, dirs_buf, 16);
        if (n <= 0) continue;
        size_t dir_count = (size_t)n;
        
        for (size_t j = 0; j < dir_count; j++) {
            SylvesStep step;
            SylvesError err = sylves_step_create(
                grid, cell, dirs_buf[j], step_lengths, user_data, &step);
            
            if (err != SYLVES_SUCCESS || step.length < 0) continue;
            
            size_t dest_index;
            if (!cell_index_table_find(cell_indices, step.dest, &dest_index)) {
                continue; // Destination not in our cell set
            }
            
            // Only add edge if source index < dest index (to avoid duplicates)
            if (i < dest_index) {
                if (total_edges < max_edges) {
                    all_edges[total_edges].src_index = i;
                    all_edges[total_edges].dest_index = dest_index;
                    all_edges[total_edges].edge.src = cell;
                    all_edges[total_edges].edge.dest = step.dest;
                    all_edges[total_edges].edge.weight = step.length;
                    total_edges++;
                }
            }
        }
        
    }
    
    // Sort edges by weight
    qsort(all_edges, total_edges, sizeof(IndexedEdge), edge_compare);
    
    // Create union-find structure
    UnionFind* uf = union_find_create(cell_count);
    if (!uf) {
        sylves_free(all_edges);
        cell_index_table_destroy(cell_indices);
        return SYLVES_ERROR_OUT_OF_MEMORY;
    }
    
    // Kruskal's algorithm: select edges that don't create cycles
    SylvesEdge* mst_edges = (SylvesEdge*)sylves_alloc(sizeof(SylvesEdge) * (cell_count - 1));
    if (!mst_edges) {
        union_find_destroy(uf);
        sylves_free(all_edges);
        cell_index_table_destroy(cell_indices);
        return SYLVES_ERROR_OUT_OF_MEMORY;
    }
    
    size_t mst_edge_count = 0;
    
    for (size_t i = 0; i < total_edges && mst_edge_count < cell_count - 1; i++) {
        IndexedEdge* edge = &all_edges[i];
        
        if (union_find_union(uf, edge->src_index, edge->dest_index)) {
            mst_edges[mst_edge_count] = edge->edge;
            mst_edge_count++;
        }
    }
    
    // Clean up
    union_find_destroy(uf);
    sylves_free(all_edges);
    cell_index_table_destroy(cell_indices);
    
    // Return results
    *edges = mst_edges;
    *edge_count = mst_edge_count;
    
    return SYLVES_SUCCESS;
}
