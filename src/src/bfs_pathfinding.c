/**
 * @file bfs_pathfinding.c
 * @brief Breadth-first search pathfinding implementation
 */

#include "sylves/pathfinding.h"
#include "sylves/memory.h"
#include "sylves/errors.h"
#include "sylves/grid.h"
#include "sylves/cell_type.h"
#include <string.h>
#include <limits.h>

/* Hash table for cell lookups */
#define HASH_TABLE_INITIAL_SIZE 64

typedef struct CellHashEntry {
    SylvesCell cell;
    int distance;       // Distance from start (in steps)
    SylvesStep step;    // How we got here
    bool has_step;      // Whether step is valid
    struct CellHashEntry* next;
} CellHashEntry;

typedef struct {
    CellHashEntry** buckets;
    size_t bucket_count;
    size_t entry_count;
} CellHashTable;

/* Queue for BFS */
typedef struct QueueNode {
    SylvesCell cell;
    struct QueueNode* next;
} QueueNode;

typedef struct {
    QueueNode* head;
    QueueNode* tail;
} Queue;

/* BFS pathfinding context */
struct SylvesBFSPathfinding {
    SylvesGrid* grid;
    SylvesCell src;
    SylvesIsAccessibleFunc is_accessible;
    void* user_data;
    
    CellHashTable* visited;
    bool early_termination;
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
    entry->distance = INT_MAX;
    entry->has_step = false;
    entry->next = table->buckets[index];
    table->buckets[index] = entry;
    table->entry_count++;
    
    // TODO: Resize table if load factor gets too high
    
    return entry;
}

/* Queue operations */
static Queue* queue_create(void) {
    Queue* queue = (Queue*)sylves_alloc(sizeof(Queue));
    if (!queue) return NULL;
    
    queue->head = NULL;
    queue->tail = NULL;
    
    return queue;
}

static void queue_destroy(Queue* queue) {
    if (!queue) return;
    
    QueueNode* node = queue->head;
    while (node) {
        QueueNode* next = node->next;
        sylves_free(node);
        node = next;
    }
    
    sylves_free(queue);
}

static bool queue_enqueue(Queue* queue, SylvesCell cell) {
    if (!queue) return false;
    
    QueueNode* node = (QueueNode*)sylves_alloc(sizeof(QueueNode));
    if (!node) return false;
    
    node->cell = cell;
    node->next = NULL;
    
    if (queue->tail) {
        queue->tail->next = node;
    } else {
        queue->head = node;
    }
    queue->tail = node;
    
    return true;
}

static bool queue_dequeue(Queue* queue, SylvesCell* cell) {
    if (!queue || !queue->head) return false;
    
    QueueNode* node = queue->head;
    *cell = node->cell;
    
    queue->head = node->next;
    if (!queue->head) {
        queue->tail = NULL;
    }
    
    sylves_free(node);
    return true;
}

static bool queue_is_empty(Queue* queue) {
    return !queue || !queue->head;
}

/* BFS implementation */

SylvesBFSPathfinding* sylves_bfs_create(
    SylvesGrid* grid,
    SylvesCell src,
    SylvesIsAccessibleFunc is_accessible,
    void* user_data) {
    
    if (!grid) return NULL;
    
    SylvesBFSPathfinding* bfs = (SylvesBFSPathfinding*)sylves_alloc(sizeof(SylvesBFSPathfinding));
    if (!bfs) return NULL;
    
    bfs->grid = grid;
    bfs->src = src;
    bfs->is_accessible = is_accessible;
    bfs->user_data = user_data;
    bfs->early_termination = false;
    
    bfs->visited = hash_table_create(HASH_TABLE_INITIAL_SIZE);
    if (!bfs->visited) {
        sylves_free(bfs);
        return NULL;
    }
    
    // Initialize source cell
    CellHashEntry* src_entry = hash_table_insert(bfs->visited, src);
    if (src_entry) {
        src_entry->distance = 0;
    }
    
    return bfs;
}

void sylves_bfs_destroy(SylvesBFSPathfinding* bfs) {
    if (!bfs) return;
    
    hash_table_destroy(bfs->visited);
    sylves_free(bfs);
}

void sylves_bfs_run(
    SylvesBFSPathfinding* bfs,
    const SylvesCell* targets,
    size_t target_count,
    int max_distance) {
    
    if (!bfs) return;
    
    Queue* queue = queue_create();
    if (!queue) return;
    
    // Start from source
    queue_enqueue(queue, bfs->src);
    
    // Check if source is accessible
    if (bfs->is_accessible && !bfs->is_accessible(bfs->src, bfs->user_data)) {
        queue_destroy(queue);
        return;
    }
    
    while (!queue_is_empty(queue)) {
        SylvesCell current;
        if (!queue_dequeue(queue, &current)) break;
        
        CellHashEntry* current_entry = hash_table_find(bfs->visited, current);
        if (!current_entry) continue;
        
        int distance = current_entry->distance;
        
        // Check if we've reached max distance
        if (max_distance >= 0 && distance >= max_distance) {
            continue;
        }
        
        // Check if we've reached a target
        if (targets && target_count > 0) {
            bool found_target = false;
            for (size_t i = 0; i < target_count; i++) {
                if (sylves_cell_equals(current, targets[i])) {
                    found_target = true;
                    break;
                }
            }
            if (found_target && bfs->early_termination) {
                break;
            }
        }
        
        // Explore neighbors
        const SylvesCellType* ct = sylves_grid_get_cell_type(bfs->grid, current);
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
        int dir_count_i = sylves_grid_get_cell_dirs(bfs->grid, current, dirs_buf, max_dirs);
        if (dir_count_i < 0) {
            if (heap_dirs) sylves_free(dirs_buf);
            continue;
        }
        size_t dir_count = (size_t)dir_count_i;
        
        for (size_t i = 0; i < dir_count; i++) {
            SylvesCell neighbor;
            SylvesCellDir inverse_dir;
            SylvesConnection connection;
            
            bool moved = sylves_grid_try_move(
                bfs->grid, current, dirs_buf[i], 
                &neighbor, &inverse_dir, &connection);
            
            if (!moved) continue;
            
            // Check if neighbor is accessible
            if (bfs->is_accessible && !bfs->is_accessible(neighbor, bfs->user_data)) {
                continue;
            }
            
            // Check if already visited
            CellHashEntry* neighbor_entry = hash_table_find(bfs->visited, neighbor);
            if (neighbor_entry && neighbor_entry->distance < INT_MAX) {
                continue;
            }
            
            // Add neighbor to visited
            if (!neighbor_entry) {
                neighbor_entry = hash_table_insert(bfs->visited, neighbor);
                if (!neighbor_entry) continue;
            }
            
            // Create step
            SylvesStep step;
            step.src = current;
            step.dest = neighbor;
            step.dir = dirs_buf[i];
            step.inverse_dir = inverse_dir;
            step.connection = connection;
            step.length = 1.0f;
            
            neighbor_entry->distance = distance + 1;
            neighbor_entry->step = step;
            neighbor_entry->has_step = true;
            
            // Add to queue
            queue_enqueue(queue, neighbor);
        }
        
        if (heap_dirs) sylves_free(dirs_buf);
    }
    
    queue_destroy(queue);
}

bool sylves_bfs_is_reachable(
    SylvesBFSPathfinding* bfs,
    SylvesCell cell,
    int* distance) {
    
    if (!bfs) return false;
    
    CellHashEntry* entry = hash_table_find(bfs->visited, cell);
    if (!entry || entry->distance == INT_MAX) {
        return false;
    }
    
    if (distance) {
        *distance = entry->distance;
    }
    
    return true;
}

SylvesCellPath* sylves_bfs_extract_path(SylvesBFSPathfinding* bfs, SylvesCell target) {
    if (!bfs) return NULL;
    
    // Check if we reached the target
    CellHashEntry* target_entry = hash_table_find(bfs->visited, target);
    if (!target_entry || target_entry->distance == INT_MAX) {
        // Check if target is the source
        if (sylves_cell_equals(target, bfs->src)) {
            return sylves_cell_path_create(NULL, 0);
        }
        return NULL;
    }
    
    // Count steps
    size_t step_count = 0;
    SylvesCell current = target;
    while (!sylves_cell_equals(current, bfs->src)) {
        CellHashEntry* entry = hash_table_find(bfs->visited, current);
        if (!entry || !entry->has_step) break;
        step_count++;
        current = entry->step.src;
    }
    
    if (!sylves_cell_equals(current, bfs->src)) {
        return NULL; // Path reconstruction failed
    }
    
    // Build path
    SylvesStep* steps = (SylvesStep*)sylves_alloc(sizeof(SylvesStep) * step_count);
    if (!steps) return NULL;
    
    current = target;
    for (size_t i = step_count; i > 0; i--) {
        CellHashEntry* entry = hash_table_find(bfs->visited, current);
        steps[i - 1] = entry->step;
        current = entry->step.src;
    }
    
    SylvesCellPath* path = sylves_cell_path_create(steps, step_count);
    sylves_free(steps);
    
    return path;
}
