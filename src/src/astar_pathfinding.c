/**
 * @file astar_pathfinding.c
 * @brief A* pathfinding algorithm implementation
 */

#include "sylves/pathfinding.h"
#include "sylves/memory.h"
#include "sylves/errors.h"
#include "sylves/cell.h"
#include "sylves/grid.h"
#include "sylves/cell_type.h"
#include <string.h>
#include <float.h>

/* Forward declaration */
static float default_step_length(const SylvesStep* step, void* user_data);

/* Hash table for cell lookups */
#define HASH_TABLE_INITIAL_SIZE 64

typedef struct CellHashEntry {
    SylvesCell cell;
    float g_score;      // Distance from start
    float f_score;      // g_score + heuristic
    SylvesStep step;    // How we got here
    bool has_step;      // Whether step is valid
    struct CellHashEntry* next;
} CellHashEntry;

typedef struct {
    CellHashEntry** buckets;
    size_t bucket_count;
    size_t entry_count;
} CellHashTable;

/* A* pathfinding context */
struct SylvesAStarPathfinding {
    SylvesGrid* grid;
    SylvesCell src;
    SylvesStepLengthFunc step_lengths;
    SylvesHeuristicFunc heuristic;
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
    entry->g_score = FLT_MAX;
    entry->f_score = FLT_MAX;
    entry->has_step = false;
    entry->next = table->buckets[index];
    table->buckets[index] = entry;
    table->entry_count++;
    
    // TODO: Resize table if load factor gets too high
    
    return entry;
}

/* A* implementation */

SylvesAStarPathfinding* sylves_astar_create(
    SylvesGrid* grid,
    SylvesCell src,
    SylvesStepLengthFunc step_lengths,
    SylvesHeuristicFunc heuristic,
    void* user_data) {
    
    if (!grid || !heuristic) return NULL;
    
    SylvesAStarPathfinding* astar = (SylvesAStarPathfinding*)sylves_alloc(sizeof(SylvesAStarPathfinding));
    if (!astar) return NULL;
    
    astar->grid = grid;
    astar->src = src;
    astar->step_lengths = step_lengths ? step_lengths : default_step_length;
    astar->heuristic = heuristic;
    astar->user_data = user_data;
    
    astar->visited = hash_table_create(HASH_TABLE_INITIAL_SIZE);
    astar->open_set = sylves_heap_create(16);
    
    if (!astar->visited || !astar->open_set) {
        hash_table_destroy(astar->visited);
        sylves_heap_destroy(astar->open_set);
        sylves_free(astar);
        return NULL;
    }
    
    // Initialize source cell
    CellHashEntry* src_entry = hash_table_insert(astar->visited, src);
    if (src_entry) {
        src_entry->g_score = 0.0f;
        src_entry->f_score = heuristic(src, user_data);
        
        // Add to open set
        sylves_heap_insert(astar->open_set, src_entry, src_entry->f_score);
    }
    
    return astar;
}

void sylves_astar_destroy(SylvesAStarPathfinding* astar) {
    if (!astar) return;
    
    hash_table_destroy(astar->visited);
    sylves_heap_destroy(astar->open_set);
    sylves_free(astar);
}

void sylves_astar_run(SylvesAStarPathfinding* astar, SylvesCell target) {
    if (!astar) return;
    
    while (!sylves_heap_is_empty(astar->open_set)) {
        float current_f;
        if (!sylves_heap_peek_key(astar->open_set, &current_f)) {
            break;
        }
        
        CellHashEntry* current_entry = (CellHashEntry*)sylves_heap_pop(astar->open_set);
        if (!current_entry) break;
        
        SylvesCell current = current_entry->cell;
        float g_score = current_entry->g_score;
        float f_score = current_entry->f_score;
        
        // Check if we've reached the target
        if (sylves_cell_equals(current, target)) {
            break;
        }
        
        // Skip if this is a redundant entry (we've already visited with lower score)
        if (f_score < current_f) {
            continue;
        }
        
        // Explore neighbors
        const SylvesCellType* ct = sylves_grid_get_cell_type(astar->grid, current);
        if (!ct) {
            continue;
        }
        int max_dirs_i = sylves_cell_type_get_dir_count(ct);
        if (max_dirs_i <= 0) {
            continue;
        }
        size_t max_dirs = (size_t)max_dirs_i;
        SylvesCellDir stack_dirs[16];
        SylvesCellDir* dirs_buf = stack_dirs;
        bool heap_dirs = false;
        if (max_dirs > (sizeof(stack_dirs) / sizeof(stack_dirs[0]))) {
            dirs_buf = (SylvesCellDir*)sylves_alloc(sizeof(SylvesCellDir) * max_dirs);
            if (!dirs_buf) {
                continue;
            }
            heap_dirs = true;
        }
        int dir_count_i = sylves_grid_get_cell_dirs(astar->grid, current, dirs_buf, max_dirs);
        if (dir_count_i < 0) {
            if (heap_dirs) sylves_free(dirs_buf);
            continue;
        }
        size_t dir_count = (size_t)dir_count_i;
        
        for (size_t i = 0; i < dir_count; i++) {
            SylvesStep step;
            SylvesError err = sylves_step_create(
                astar->grid, current, dirs_buf[i], 
                astar->step_lengths, astar->user_data, &step);
            
            if (err != SYLVES_SUCCESS) continue;
            
            // Check if step is valid (non-negative length)
            if (step.length < 0) continue;
            
            float tentative_g = g_score + step.length;
            SylvesCell neighbor = step.dest;
            
            // Get or create neighbor entry
            CellHashEntry* neighbor_entry = hash_table_find(astar->visited, neighbor);
            if (!neighbor_entry) {
                neighbor_entry = hash_table_insert(astar->visited, neighbor);
                if (!neighbor_entry) continue;
            }
            
            // Check if this path is better
            if (tentative_g < neighbor_entry->g_score) {
                neighbor_entry->g_score = tentative_g;
                neighbor_entry->f_score = tentative_g + astar->heuristic(neighbor, astar->user_data);
                neighbor_entry->step = step;
                neighbor_entry->has_step = true;
                
                // Add to open set
                sylves_heap_insert(astar->open_set, neighbor_entry, neighbor_entry->f_score);
            }
        }
        
        if (heap_dirs) sylves_free(dirs_buf);
    }
}

SylvesCellPath* sylves_astar_extract_path(SylvesAStarPathfinding* astar, SylvesCell target) {
    if (!astar) return NULL;
    
    // Check if we reached the target
    CellHashEntry* target_entry = hash_table_find(astar->visited, target);
    if (!target_entry || !target_entry->has_step) {
        // Check if target is the source
        if (sylves_cell_equals(target, astar->src)) {
            return sylves_cell_path_create(NULL, 0);
        }
        return NULL;
    }
    
    // Count steps
    size_t step_count = 0;
    SylvesCell current = target;
    while (!sylves_cell_equals(current, astar->src)) {
        CellHashEntry* entry = hash_table_find(astar->visited, current);
        if (!entry || !entry->has_step) break;
        step_count++;
        current = entry->step.src;
    }
    
    if (!sylves_cell_equals(current, astar->src)) {
        return NULL; // Path reconstruction failed
    }
    
    // Build path
    SylvesStep* steps = (SylvesStep*)sylves_alloc(sizeof(SylvesStep) * step_count);
    if (!steps) return NULL;
    
    current = target;
    for (size_t i = step_count; i > 0; i--) {
        CellHashEntry* entry = hash_table_find(astar->visited, current);
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
