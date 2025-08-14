/**
 * @file pathfinding.c
 * @brief Basic pathfinding infrastructure implementation
 */

#include "sylves/pathfinding.h"
#include "sylves/memory.h"
#include "sylves/errors.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>

/* Heap implementation */

static inline size_t heap_parent(size_t i) {
    return (i - 1) >> 1;
}

static inline size_t heap_left(size_t i) {
    return (i << 1) + 1;
}

static inline size_t heap_right(size_t i) {
    return (i << 1) + 2;
}

SylvesHeap* sylves_heap_create(size_t initial_capacity) {
    SylvesHeap* heap = (SylvesHeap*)sylves_alloc(sizeof(SylvesHeap));
    if (!heap) return NULL;
    
    if (initial_capacity == 0) {
        initial_capacity = 4;
    }
    
    heap->items = (void**)sylves_alloc(sizeof(void*) * initial_capacity);
    heap->keys = (float*)sylves_alloc(sizeof(float) * initial_capacity);
    
    if (!heap->items || !heap->keys) {
        sylves_free(heap->items);
        sylves_free(heap->keys);
        sylves_free(heap);
        return NULL;
    }
    
    heap->capacity = initial_capacity;
    heap->size = 0;
    
    return heap;
}

void sylves_heap_destroy(SylvesHeap* heap) {
    if (!heap) return;
    
    sylves_free(heap->items);
    sylves_free(heap->keys);
    sylves_free(heap);
}

static void heap_decreased_key(SylvesHeap* heap, size_t i) {
    float key = heap->keys[i];
    void* item = heap->items[i];
    
    while (i > 0) {
        size_t p = heap_parent(i);
        float parent_key = heap->keys[p];
        
        if (parent_key > key) {
            // Swap with parent
            heap->keys[p] = key;
            heap->keys[i] = parent_key;
            heap->items[p] = item;
            heap->items[i] = heap->items[p];
            i = p;
        } else {
            break;
        }
    }
}

static void heap_heapify(SylvesHeap* heap, size_t i) {
    float key = heap->keys[i];
    void* item = heap->items[i];
    size_t smallest = i;
    float smallest_key = key;
    
    size_t l = heap_left(i);
    if (l < heap->size) {
        float left_key = heap->keys[l];
        if (left_key < smallest_key) {
            smallest = l;
            smallest_key = left_key;
        }
    }
    
    size_t r = heap_right(i);
    if (r < heap->size) {
        float right_key = heap->keys[r];
        if (right_key < smallest_key) {
            smallest = r;
            smallest_key = right_key;
        }
    }
    
    if (i != smallest) {
        // Swap with smallest child
        heap->keys[i] = heap->keys[smallest];
        heap->keys[smallest] = key;
        void* temp = heap->items[i];
        heap->items[i] = heap->items[smallest];
        heap->items[smallest] = temp;
        
        heap_heapify(heap, smallest);
    }
}

void sylves_heap_insert(SylvesHeap* heap, void* item, float key) {
    if (!heap) return;
    
    // Resize if necessary
    if (heap->size >= heap->capacity) {
        size_t new_capacity = heap->capacity * 2;
        void** new_items = (void**)sylves_realloc(heap->items, sizeof(void*) * new_capacity);
        float* new_keys = (float*)sylves_realloc(heap->keys, sizeof(float) * new_capacity);
        
        if (!new_items || !new_keys) {
            // Allocation failed - don't insert
            return;
        }
        
        heap->items = new_items;
        heap->keys = new_keys;
        heap->capacity = new_capacity;
    }
    
    // Insert at end
    heap->items[heap->size] = item;
    heap->keys[heap->size] = key;
    heap->size++;
    
    // Restore heap property
    heap_decreased_key(heap, heap->size - 1);
}

void* sylves_heap_pop(SylvesHeap* heap) {
    if (!heap || heap->size == 0) return NULL;
    
    void* result = heap->items[0];
    
    // Move last element to root
    heap->items[0] = heap->items[heap->size - 1];
    heap->keys[0] = heap->keys[heap->size - 1];
    heap->size--;
    
    // Restore heap property
    if (heap->size > 0) {
        heap_heapify(heap, 0);
    }
    
    return result;
}

bool sylves_heap_peek_key(SylvesHeap* heap, float* key) {
    if (!heap || heap->size == 0) return false;
    
    if (key) {
        *key = heap->keys[0];
    }
    return true;
}

bool sylves_heap_is_empty(SylvesHeap* heap) {
    return !heap || heap->size == 0;
}

void sylves_heap_clear(SylvesHeap* heap) {
    if (!heap) return;
    heap->size = 0;
}

/* Step utilities */

SylvesError sylves_step_create(
    SylvesGrid* grid,
    SylvesCell src,
    SylvesCellDir dir,
    SylvesStepLengthFunc step_lengths,
    void* user_data,
    SylvesStep* step) {
    
    if (!grid || !step) {
        return SYLVES_ERROR_INVALID_ARGUMENT;
    }
    
    SylvesCell dest;
    SylvesCellDir inverse_dir;
    SylvesConnection connection;
    
    SylvesError err = sylves_grid_try_move(grid, src, dir, &dest, &inverse_dir, &connection);
    if (err != SYLVES_SUCCESS) {
        return err;
    }
    
    step->src = src;
    step->dest = dest;
    step->dir = dir;
    step->inverse_dir = inverse_dir;
    step->connection = connection;
    
    // Calculate length
    if (step_lengths) {
        float length = step_lengths(step, user_data);
        if (length < 0) {
            return SYLVES_ERROR_INVALID_ARGUMENT;
        }
        step->length = length;
    } else {
        step->length = 1.0f;
    }
    
    return SYLVES_SUCCESS;
}

void sylves_step_inverse(const SylvesStep* step, SylvesStep* inverse) {
    if (!step || !inverse) return;
    
    inverse->src = step->dest;
    inverse->dest = step->src;
    inverse->dir = step->inverse_dir;
    inverse->inverse_dir = step->dir;
    inverse->connection = sylves_connection_invert(step->connection);
    inverse->length = step->length;
}

/* Path management */

SylvesCellPath* sylves_cell_path_create(const SylvesStep* steps, size_t step_count) {
    SylvesCellPath* path = (SylvesCellPath*)sylves_alloc(sizeof(SylvesCellPath));
    if (!path) return NULL;
    
    if (step_count > 0) {
        path->steps = (SylvesStep*)sylves_alloc(sizeof(SylvesStep) * step_count);
        if (!path->steps) {
            sylves_free(path);
            return NULL;
        }
        
        memcpy(path->steps, steps, sizeof(SylvesStep) * step_count);
        
        // Calculate total length
        float total = 0.0f;
        for (size_t i = 0; i < step_count; i++) {
            total += steps[i].length;
        }
        path->total_length = total;
    } else {
        path->steps = NULL;
        path->total_length = 0.0f;
    }
    
    path->step_count = step_count;
    
    return path;
}

void sylves_cell_path_destroy(SylvesCellPath* path) {
    if (!path) return;
    
    sylves_free(path->steps);
    sylves_free(path);
}

void sylves_cell_path_get_cells(const SylvesCellPath* path, SylvesCell* cells) {
    if (!path || !cells) return;
    
    if (path->step_count > 0) {
        cells[0] = path->steps[0].src;
        for (size_t i = 0; i < path->step_count; i++) {
            cells[i + 1] = path->steps[i].dest;
        }
    }
}

/* Heuristic functions */

float sylves_heuristic_manhattan(SylvesCell current, SylvesCell target, float scale) {
    int dx = abs(current.x - target.x);
    int dy = abs(current.y - target.y);
    int dz = abs(current.z - target.z);
    return (float)(dx + dy + dz) * scale;
}

float sylves_heuristic_euclidean(SylvesGrid* grid, SylvesCell current, SylvesCell target) {
    if (!grid) return 0.0f;
    
    SylvesVector3 current_pos = sylves_grid_get_cell_center(grid, current);
    SylvesVector3 target_pos = sylves_grid_get_cell_center(grid, target);
    
    float dx = current_pos.x - target_pos.x;
    float dy = current_pos.y - target_pos.y;
    float dz = current_pos.z - target_pos.z;
    
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

/* Heuristic data for grid types */
typedef struct {
    SylvesCell target;
    float scale;
} ManhattanHeuristicData;

static float manhattan_heuristic_func(SylvesCell cell, void* user_data) {
    ManhattanHeuristicData* data = (ManhattanHeuristicData*)user_data;
    return sylves_heuristic_manhattan(cell, data->target, data->scale);
}

SylvesHeuristicFunc sylves_get_admissible_heuristic(
    SylvesGrid* grid,
    SylvesCell target,
    void** user_data) {
    
    if (!grid) return NULL;
    
    SylvesGridType type = sylves_grid_get_type(grid);
    
    // Check for grid types that support Manhattan distance
    if (type == SYLVES_GRID_TYPE_SQUARE || 
        type == SYLVES_GRID_TYPE_CUBE ||
        type == SYLVES_GRID_TYPE_TRIANGLE ||
        type == SYLVES_GRID_TYPE_HEX) {
        
        ManhattanHeuristicData* data = (ManhattanHeuristicData*)sylves_alloc(sizeof(ManhattanHeuristicData));
        if (!data) return NULL;
        
        data->target = target;
        data->scale = 1.0f;
        
        if (user_data) {
            *user_data = data;
        }
        
        return manhattan_heuristic_func;
    }
    
    // Check for modifiers
    if (type == SYLVES_GRID_TYPE_MODIFIER) {
        // TODO: Handle modifiers - for now just return NULL
        return NULL;
    }
    
    // Unknown grid type
    return NULL;
}

/* High-level pathfinding functions */

static float default_step_length(const SylvesStep* step, void* user_data) {
    (void)step;
    (void)user_data;
    return 1.0f;
}

typedef struct {
    SylvesIsAccessibleFunc is_accessible;
    SylvesStepLengthFunc step_lengths;
    void* user_data;
} CombinedStepData;

static float combined_step_length(const SylvesStep* step, void* user_data) {
    CombinedStepData* data = (CombinedStepData*)user_data;
    
    if (data->is_accessible) {
        if (!data->is_accessible(step->dest, data->user_data)) {
            return -1.0f; // Inaccessible
        }
    }
    
    if (data->step_lengths) {
        return data->step_lengths(step, data->user_data);
    }
    
    return 1.0f;
}

SylvesCellPath* sylves_find_path(
    SylvesGrid* grid,
    SylvesCell src,
    SylvesCell dest,
    SylvesIsAccessibleFunc is_accessible,
    SylvesStepLengthFunc step_lengths,
    void* user_data) {
    
    if (!grid) return NULL;
    
    // Combine accessibility and step length functions
    CombinedStepData combined_data = {
        .is_accessible = is_accessible,
        .step_lengths = step_lengths,
        .user_data = user_data
    };
    
    // Try to use A* if we can get a heuristic
    if (!step_lengths) {
        void* heuristic_data = NULL;
        SylvesHeuristicFunc heuristic = sylves_get_admissible_heuristic(grid, dest, &heuristic_data);
        
        if (heuristic) {
            SylvesAStarPathfinding* astar = sylves_astar_create(
                grid, src, combined_step_length, heuristic, 
                is_accessible || step_lengths ? &combined_data : NULL);
            
            if (astar) {
                sylves_astar_run(astar, dest);
                SylvesCellPath* path = sylves_astar_extract_path(astar, dest);
                sylves_astar_destroy(astar);
                sylves_free(heuristic_data);
                return path;
            }
            
            sylves_free(heuristic_data);
        }
    }
    
    // Fall back to Dijkstra
    SylvesDijkstraPathfinding* dijkstra = sylves_dijkstra_create(
        grid, src, combined_step_length, 
        is_accessible || step_lengths ? &combined_data : NULL);
    
    if (!dijkstra) return NULL;
    
    sylves_dijkstra_run(dijkstra, &dest, FLT_MAX);
    SylvesCellPath* path = sylves_dijkstra_extract_path(dijkstra, dest);
    sylves_dijkstra_destroy(dijkstra);
    
    return path;
}

SylvesError sylves_find_distance(
    SylvesGrid* grid,
    SylvesCell src,
    SylvesCell dest,
    SylvesIsAccessibleFunc is_accessible,
    SylvesStepLengthFunc step_lengths,
    void* user_data,
    float* distance) {
    
    if (!grid || !distance) {
        return SYLVES_ERROR_INVALID_ARGUMENT;
    }
    
    SylvesCellPath* path = sylves_find_path(grid, src, dest, is_accessible, step_lengths, user_data);
    if (!path) {
        return SYLVES_ERROR_CELL_NOT_FOUND;
    }
    
    *distance = path->total_length;
    sylves_cell_path_destroy(path);
    
    return SYLVES_SUCCESS;
}
