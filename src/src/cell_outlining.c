/**
 * @file cell_outlining.c
 * @brief Cell outlining algorithms implementation
 */

#include "sylves/pathfinding.h"
#include "sylves/memory.h"
#include "sylves/errors.h"
#include "sylves/polygon.h"
#include <string.h>
#include <math.h>

/* Hash table for fast cell lookup */
typedef struct CellSetEntry {
    SylvesCell cell;
    struct CellSetEntry* next;
} CellSetEntry;

typedef struct {
    CellSetEntry** buckets;
    size_t bucket_count;
    size_t entry_count;
} CellSet;

/* Edge structure for outline building */
typedef struct OutlineEdge {
    SylvesVector3 start;
    SylvesVector3 end;
    SylvesCell cell;
    SylvesCellDir dir;
    bool used;
    struct OutlineEdge* next_in_chain;
} OutlineEdge;

/* Hash function for cells */
static size_t cell_hash(SylvesCell cell) {
    size_t hash = 0;
    hash ^= (size_t)cell.x + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    hash ^= (size_t)cell.y + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    hash ^= (size_t)cell.z + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    return hash;
}

/* Cell set operations */
static CellSet* cell_set_create(size_t initial_size) {
    CellSet* set = (CellSet*)sylves_alloc(sizeof(CellSet));
    if (!set) return NULL;
    
    set->buckets = (CellSetEntry**)sylves_calloc(initial_size, sizeof(CellSetEntry*));
    if (!set->buckets) {
        sylves_free(set);
        return NULL;
    }
    
    set->bucket_count = initial_size;
    set->entry_count = 0;
    
    return set;
}

static void cell_set_destroy(CellSet* set) {
    if (!set) return;
    
    for (size_t i = 0; i < set->bucket_count; i++) {
        CellSetEntry* entry = set->buckets[i];
        while (entry) {
            CellSetEntry* next = entry->next;
            sylves_free(entry);
            entry = next;
        }
    }
    
    sylves_free(set->buckets);
    sylves_free(set);
}

static bool cell_set_contains(CellSet* set, SylvesCell cell) {
    size_t index = cell_hash(cell) % set->bucket_count;
    CellSetEntry* entry = set->buckets[index];
    
    while (entry) {
        if (sylves_cell_equals(entry->cell, cell)) {
            return true;
        }
        entry = entry->next;
    }
    
    return false;
}

static bool cell_set_insert(CellSet* set, SylvesCell cell) {
    if (cell_set_contains(set, cell)) {
        return true; // Already exists
    }
    
    size_t index = cell_hash(cell) % set->bucket_count;
    
    CellSetEntry* entry = (CellSetEntry*)sylves_alloc(sizeof(CellSetEntry));
    if (!entry) return false;
    
    entry->cell = cell;
    entry->next = set->buckets[index];
    set->buckets[index] = entry;
    set->entry_count++;
    
    return true;
}

/* Check if points are approximately equal */
static bool vector3_approx_equal(SylvesVector3 a, SylvesVector3 b, double epsilon) {
    return fabs(a.x - b.x) < epsilon &&
           fabs(a.y - b.y) < epsilon &&
           fabs(a.z - b.z) < epsilon;
}

/* Get edge between two cells */
static bool get_shared_edge(
    SylvesGrid* grid,
    SylvesCell cell1,
    SylvesCell cell2,
    SylvesVector3* start,
    SylvesVector3* end) {
    
    // Get corners of both cells
    SylvesVector3 corners1_buf[16];
    SylvesVector3 corners2_buf[16];
    int corners1_count = sylves_grid_get_cell_corners(grid, cell1, corners1_buf, 16);
    int corners2_count = sylves_grid_get_cell_corners(grid, cell2, corners2_buf, 16);
    
    if (corners1_count <= 0 || corners2_count <= 0) return false;
    
    SylvesVector3* corners1 = (SylvesVector3*)sylves_alloc(sizeof(SylvesVector3) * corners1_count);
    SylvesVector3* corners2 = (SylvesVector3*)sylves_alloc(sizeof(SylvesVector3) * corners2_count);
    
    if (!corners1 || !corners2) {
        sylves_free(corners1);
        sylves_free(corners2);
        return false;
    }
    
    // Get actual corners
    for (int i = 0; i < corners1_count; i++) {
        corners1[i] = corners1_buf[i];
    }
    for (int i = 0; i < corners2_count; i++) {
        corners2[i] = corners2_buf[i];
    }
    
    // Find shared corners
    SylvesVector3 shared[2];
    size_t shared_count = 0;
    const float epsilon = 1e-6f;
    
    for (size_t i = 0; i < corners1_count && shared_count < 2; i++) {
        for (size_t j = 0; j < corners2_count; j++) {
            if (vector3_approx_equal(corners1[i], corners2[j], epsilon)) {
                shared[shared_count++] = corners1[i];
                break;
            }
        }
    }
    
    sylves_free(corners1);
    sylves_free(corners2);
    
    if (shared_count == 2) {
        *start = shared[0];
        *end = shared[1];
        return true;
    }
    
    return false;
}

/* Collect boundary edges */
static OutlineEdge* collect_boundary_edges(
    SylvesGrid* grid,
    const CellSet* cell_set,
    size_t* edge_count) {
    
    size_t capacity = cell_set->entry_count * 6; // Overestimate
    OutlineEdge* edges = (OutlineEdge*)sylves_alloc(sizeof(OutlineEdge) * capacity);
    if (!edges) return NULL;
    
    size_t count = 0;
    
    // For each cell in the set
    for (size_t i = 0; i < cell_set->bucket_count; i++) {
        CellSetEntry* entry = cell_set->buckets[i];
        while (entry) {
            SylvesCell cell = entry->cell;
            
            // Check each direction
            size_t dir_count = 0;
            SylvesCellDir dirs_buf[16];
            int got = sylves_grid_get_cell_dirs(grid, cell, dirs_buf, 16);
            if (got <= 0) { entry = entry->next; continue; }
            dir_count = (size_t)got;
            
            for (size_t j = 0; j < dir_count; j++) {
                SylvesCell neighbor;
                SylvesCellDir inverse_dir;
                SylvesConnection connection;
                
                    SylvesError err = sylves_grid_try_move(
                    grid, cell, dirs_buf[j], &neighbor, &inverse_dir, &connection);
                
                if (err == SYLVES_SUCCESS) {
                    // Check if neighbor is NOT in the set (boundary edge)
                    if (!cell_set_contains((CellSet*)cell_set, neighbor)) {
                        // This is a boundary edge
                        SylvesVector3 start, end;
                        if (get_shared_edge(grid, cell, neighbor, &start, &end)) {
                            if (count < capacity) {
                                edges[count].start = start;
                                edges[count].end = end;
                                edges[count].cell = cell;
                                edges[count].dir = dirs_buf[j];
                                edges[count].used = false;
                                edges[count].next_in_chain = NULL;
                                count++;
                            }
                        }
                    }
                } else {
                    // No neighbor = boundary edge
                    // Get the edge for this direction
                    // This is more complex and depends on the cell type
                    // For now, we'll skip edges without neighbors
                }
            }
            
            entry = entry->next;
        }
    }
    
    *edge_count = count;
    return edges;
}

/* Find edge that connects to the end of the current edge */
static OutlineEdge* find_next_edge(
    OutlineEdge* edges,
    size_t edge_count,
    SylvesVector3 end_point,
    float epsilon) {
    
    for (size_t i = 0; i < edge_count; i++) {
        if (!edges[i].used) {
            if (vector3_approx_equal(edges[i].start, end_point, epsilon)) {
                return &edges[i];
            }
            // Check if edge needs to be reversed
            if (vector3_approx_equal(edges[i].end, end_point, epsilon)) {
                // Swap start and end
                SylvesVector3 temp = edges[i].start;
                edges[i].start = edges[i].end;
                edges[i].end = temp;
                return &edges[i];
            }
        }
    }
    
    return NULL;
}

/* Build continuous chains from edges */
static size_t build_chains(
    OutlineEdge* edges,
    size_t edge_count,
    OutlineEdge** chain_starts,
    size_t max_chains) {
    
    const float epsilon = 1e-6f;
    size_t chain_count = 0;
    
    for (size_t i = 0; i < edge_count && chain_count < max_chains; i++) {
        if (!edges[i].used) {
            // Start a new chain
            chain_starts[chain_count++] = &edges[i];
            edges[i].used = true;
            
            // Follow the chain
            OutlineEdge* current = &edges[i];
            SylvesVector3 next_start = current->end;
            
            while (true) {
                OutlineEdge* next = find_next_edge(edges, edge_count, next_start, epsilon);
                if (!next) break;
                
                next->used = true;
                current->next_in_chain = next;
                current = next;
                next_start = current->end;
                
                // Check if we've closed the loop
                if (vector3_approx_equal(next_start, edges[i].start, epsilon)) {
                    break;
                }
            }
        }
    }
    
    return chain_count;
}

/* Main outlining function */
SylvesError sylves_outline_cells(
    SylvesGrid* grid,
    const SylvesCell* cells,
    size_t cell_count,
    SylvesOutlineSegment** segments,
    size_t* segment_count) {
    
    if (!grid || !cells || cell_count == 0 || !segments || !segment_count) {
        return SYLVES_ERROR_INVALID_ARGUMENT;
    }
    
    // Create cell set for fast lookup
    CellSet* cell_set = cell_set_create(cell_count * 2);
    if (!cell_set) {
        return SYLVES_ERROR_OUT_OF_MEMORY;
    }
    
    // Add all cells to set
    for (size_t i = 0; i < cell_count; i++) {
        if (!cell_set_insert(cell_set, cells[i])) {
            cell_set_destroy(cell_set);
            return SYLVES_ERROR_OUT_OF_MEMORY;
        }
    }
    
    // Collect boundary edges
    size_t edge_count;
    OutlineEdge* edges = collect_boundary_edges(grid, cell_set, &edge_count);
    if (!edges && edge_count > 0) {
        cell_set_destroy(cell_set);
        return SYLVES_ERROR_OUT_OF_MEMORY;
    }
    
    // Build chains
    OutlineEdge** chain_starts = (OutlineEdge**)sylves_alloc(sizeof(OutlineEdge*) * edge_count);
    if (!chain_starts) {
        sylves_free(edges);
        cell_set_destroy(cell_set);
        return SYLVES_ERROR_OUT_OF_MEMORY;
    }
    
    size_t chain_count = build_chains(edges, edge_count, chain_starts, edge_count);
    
    // Count total segments needed
    size_t total_segments = 0;
    for (size_t i = 0; i < chain_count; i++) {
        OutlineEdge* edge = chain_starts[i];
        while (edge) {
            total_segments++;
            edge = edge->next_in_chain;
        }
    }
    
    // Allocate output segments
    SylvesOutlineSegment* output_segments = (SylvesOutlineSegment*)sylves_alloc(
        sizeof(SylvesOutlineSegment) * total_segments);
    if (!output_segments) {
        sylves_free(chain_starts);
        sylves_free(edges);
        cell_set_destroy(cell_set);
        return SYLVES_ERROR_OUT_OF_MEMORY;
    }
    
    // Fill output segments
    size_t segment_index = 0;
    for (size_t i = 0; i < chain_count; i++) {
        OutlineEdge* edge = chain_starts[i];
        while (edge && segment_index < total_segments) {
            output_segments[segment_index].start = edge->start;
            output_segments[segment_index].end = edge->end;
            segment_index++;
            edge = edge->next_in_chain;
        }
    }
    
    // Clean up
    sylves_free(chain_starts);
    sylves_free(edges);
    cell_set_destroy(cell_set);
    
    // Return results
    *segments = output_segments;
    *segment_count = segment_index;
    
    return SYLVES_SUCCESS;
}
