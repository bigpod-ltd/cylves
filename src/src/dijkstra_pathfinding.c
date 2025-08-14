/**
 * @file dijkstra_pathfinding.c
 * @brief Dijkstra pathfinding algorithm implementation
 */

#include "sylves/pathfinding.h"
#include "sylves/memory.h"
#include "sylves/errors.h"
#include "sylves/cell.h"
#include <string.h>
#include <float.h>

/* Forward declaration */
static float default_step_length(const SylvesStep* step, void* user_data);

/* Hash table for cell lookups - reuse same structure as A* */
#define HASH_TABLE_INITIAL_SIZE 64

typedef struct CellHashEntry {
    SylvesCell cell;
    float distance;     // Distance from start
    SylvesStep step;    // How we got here
    bool has_step;      // Whether step is valid
    struct CellHashEntry* next;
} CellHashEntry;

typedef struct {
    CellHashEntry** buckets;
    size_t bucket_count;
    size_t entry_count;
} CellHashTable;

/* Dijkstra pathfinding context */
struct SylvesDijkstraPathfinding {
    SylvesGrid* grid;
    SylvesCell src;
    SylvesStepLengthFunc step_lengths;
    void* user_data;
    
    CellHashTable* visited;
    SylvesHeap* open_set;
};

/* Hash function for cells */
static size_t cell_hash(SylvesCell cell) {
    // Simple hash combining x, y, z coordinates
    size_t hash = 0;
    hash ^= (size_t)cell.x + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    hash ^= (size_t)cell.y + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    hash ^= (size_t)cell.z + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    return hash;
}

/* Hash table operations */
static CellHashTable* hash_table_create(size_t initial_size) {
    CellHashTable* table = (CellHashTable*)sylves_alloc(sizeof(CellHashTable));
    if (!table) return NULL;
    
    table->buckets = (CellHashEntry**)sylves_calloc(initial_size, sizeof(CellHashEntry*));
    if (!table->buckets) {
        sylves_free(table);
        return NULL;
    }
    
    table->bucket_count = initial_size;
    table->entry_count = 0;
    
    return table;
}

static void hash_table_destroy(CellHashTable* table) {
    if (!table) return;
    
    // Free all entries
    for (size_t i = 0; i < table->bucket_count; i++) {
        CellHashEntry* entry = table->buckets[i];
        while (entry) {
            CellHashEntry* next = entry->next;
            sylves_free(entry);
            entry = next;
        }
    }
    
    sylves_free(table->buckets);
    sylves_free(table);
}

static CellHashEntry* hash_table_find(CellHashTable* table, SylvesCell cell) {
    size_t index = cell_hash(cell) % table->bucket_count;
    CellHashEntry* entry = table->buckets[index];
    
    while (entry) {
        if (sylves_cell_equals(entry->cell, cell)) {
            return entry;
        }
        entry = entry->next;
    }
    
    return NULL;
}

static CellHashEntry* hash_table_insert(CellHashTable* table, SylvesCell cell) {
    size_t index = cell_hash(cell) % table->bucket_count;
    
    // Check if already exists
    CellHashEntry* existing = hash_table_find(table, cell);
    if (existing) return existing;
    
    // Create new entry
    CellHashEntry* entry = (CellHashEntry*)sylves_alloc(sizeof(CellHashEntry));
    if (!entry) return NULL;
    
    entry->cell = cell;
    entry->distance = FLT_MAX;
    entry->has_step = false;
    entry->next = table->buckets[index];
    table->buckets[index] = entry;
    table->entry_count++;
    
    // TODO: Resize table if load factor gets too high
    
    return entry;
}

/* Dijkstra implementation */

SylvesDijkstraPathfinding* sylves_dijkstra_create(
    SylvesGrid* grid,
    SylvesCell src,
    SylvesStepLengthFunc step_lengths,
    void* user_data) {
    
    if (!grid) return NULL;
    
    SylvesDijkstraPathfinding* dijkstra = (SylvesDijkstraPathfinding*)sylves_alloc(sizeof(SylvesDijkstraPathfinding));
    if (!dijkstra) return NULL;
    
    dijkstra->grid = grid;
    dijkstra->src = src;
    dijkstra->step_lengths = step_lengths ? step_lengths : default_step_length;
    dijkstra->user_data = user_data;
    
    dijkstra->visited = hash_table_create(HASH_TABLE_INITIAL_SIZE);
    dijkstra->open_set = sylves_heap_create(16);
    
    if (!dijkstra->visited || !dijkstra->open_set) {
        hash_table_destroy(dijkstra->visited);
        sylves_heap_destroy(dijkstra->open_set);
        sylves_free(dijkstra);
        return NULL;
    }
    
    // Initialize source cell
    CellHashEntry* src_entry = hash_table_insert(dijkstra->visited, src);
    if (src_entry) {
        src_entry->distance = 0.0f;
        
        // Add to open set
        sylves_heap_insert(dijkstra->open_set, src_entry, 0.0f);
    }
    
    return dijkstra;
}

void sylves_dijkstra_destroy(SylvesDijkstraPathfinding* dijkstra) {
    if (!dijkstra) return;
    
    hash_table_destroy(dijkstra->visited);
    sylves_heap_destroy(dijkstra->open_set);
    sylves_free(dijkstra);
}

void sylves_dijkstra_run(
    SylvesDijkstraPathfinding* dijkstra,
    const SylvesCell* target,
    float max_range) {
    
    if (!dijkstra) return;
    
    while (!sylves_heap_is_empty(dijkstra->open_set)) {
        float current_dist;
        if (!sylves_heap_peek_key(dijkstra->open_set, &current_dist)) {
            break;
        }
        
        // Check if we've exceeded max range
        if (current_dist > max_range) {
            break;
        }
        
        CellHashEntry* current_entry = (CellHashEntry*)sylves_heap_pop(dijkstra->open_set);
        if (!current_entry) break;
        
        SylvesCell current = current_entry->cell;
        float distance = current_entry->distance;
        
        // Check if we've reached the target
        if (target && sylves_cell_equals(current, *target)) {
            break;
        }
        
        // Skip if this is a redundant entry (we've already visited with lower distance)
        if (distance < current_dist) {
            continue;
        }
        
        // Explore neighbors
        size_t dir_count = 0;
        SylvesCellDir dirs_buf[16];
        int got = sylves_grid_get_cell_dirs(dijkstra->grid, current, dirs_buf, 16);
        if (got <= 0) {
            continue;
        }
        dir_count = (size_t)got;
        
        for (size_t i = 0; i < dir_count; i++) {
            SylvesStep step;
            SylvesError err = sylves_step_create(
                dijkstra->grid, current, dirs_buf[i], 
                dijkstra->step_lengths, dijkstra->user_data, &step);
            
            if (err != SYLVES_SUCCESS) continue;
            
            // Check if step is valid (non-negative length)
            if (step.length < 0) continue;
            
            float tentative_dist = distance + step.length;
            
            // Check if within max range
            if (tentative_dist > max_range) continue;
            
            SylvesCell neighbor = step.dest;
            
            // Get or create neighbor entry
            CellHashEntry* neighbor_entry = hash_table_find(dijkstra->visited, neighbor);
            if (!neighbor_entry) {
                neighbor_entry = hash_table_insert(dijkstra->visited, neighbor);
                if (!neighbor_entry) continue;
            }
            
            // Check if this path is better
            if (tentative_dist < neighbor_entry->distance) {
                neighbor_entry->distance = tentative_dist;
                neighbor_entry->step = step;
                neighbor_entry->has_step = true;
                
                // Add to open set
                sylves_heap_insert(dijkstra->open_set, neighbor_entry, tentative_dist);
            }
        }
        
    }
}

SylvesError sylves_dijkstra_get_distances(
    SylvesDijkstraPathfinding* dijkstra,
    SylvesCell* cells,
    float* distances,
    size_t* count) {
    
    if (!dijkstra || !count) {
        return SYLVES_ERROR_INVALID_ARGUMENT;
    }
    
    size_t max_count = *count;
    size_t actual_count = 0;
    
    // If output arrays are provided, fill them
    if (cells && distances && max_count > 0) {
        // Iterate through all visited cells
        for (size_t i = 0; i < dijkstra->visited->bucket_count && actual_count < max_count; i++) {
            CellHashEntry* entry = dijkstra->visited->buckets[i];
            while (entry && actual_count < max_count) {
                if (entry->distance < FLT_MAX) {
                    cells[actual_count] = entry->cell;
                    distances[actual_count] = entry->distance;
                    actual_count++;
                }
                entry = entry->next;
            }
        }
    } else {
        // Just count the reachable cells
        for (size_t i = 0; i < dijkstra->visited->bucket_count; i++) {
            CellHashEntry* entry = dijkstra->visited->buckets[i];
            while (entry) {
                if (entry->distance < FLT_MAX) {
                    actual_count++;
                }
                entry = entry->next;
            }
        }
    }
    
    *count = actual_count;
    return SYLVES_SUCCESS;
}

SylvesCellPath* sylves_dijkstra_extract_path(SylvesDijkstraPathfinding* dijkstra, SylvesCell target) {
    if (!dijkstra) return NULL;
    
    // Check if we reached the target
    CellHashEntry* target_entry = hash_table_find(dijkstra->visited, target);
    if (!target_entry || target_entry->distance == FLT_MAX) {
        // Check if target is the source
        if (sylves_cell_equals(target, dijkstra->src)) {
            return sylves_cell_path_create(NULL, 0);
        }
        return NULL;
    }
    
    // Count steps
    size_t step_count = 0;
    SylvesCell current = target;
    while (!sylves_cell_equals(current, dijkstra->src)) {
        CellHashEntry* entry = hash_table_find(dijkstra->visited, current);
        if (!entry || !entry->has_step) break;
        step_count++;
        current = entry->step.src;
    }
    
    if (!sylves_cell_equals(current, dijkstra->src)) {
        return NULL; // Path reconstruction failed
    }
    
    // Build path
    SylvesStep* steps = (SylvesStep*)sylves_alloc(sizeof(SylvesStep) * step_count);
    if (!steps) return NULL;
    
    current = target;
    for (size_t i = step_count; i > 0; i--) {
        CellHashEntry* entry = hash_table_find(dijkstra->visited, current);
        steps[i - 1] = entry->step;
        current = entry->step.src;
    }
    
    SylvesCellPath* path = sylves_cell_path_create(steps, step_count);
    sylves_free(steps);
    
    return path;
}

/* Default step length function */
static float default_step_length(const SylvesStep* step, void* user_data) {
    (void)step;
    (void)user_data;
    return 1.0f;
}
