/**
 * @file pathfinding.h
 * @brief Pathfinding algorithms for grid navigation
 * 
 * This module provides various pathfinding algorithms including A*, Dijkstra,
 * and breadth-first search for finding optimal paths through grids.
 */

#ifndef SYLVES_PATHFINDING_H
#define SYLVES_PATHFINDING_H

#include "types.h"
#include "grid.h"
#include "cell.h"
#include "connection.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct SylvesStep SylvesStep;
typedef struct SylvesCellPath SylvesCellPath;
typedef struct SylvesPathfindingContext SylvesPathfindingContext;
typedef struct SylvesHeap SylvesHeap;

/**
 * @brief Step in a path between cells
 * 
 * Represents a single movement from one cell to another,
 * including direction and connection information.
 */
struct SylvesStep {
    SylvesCell src;           /**< Source cell */
    SylvesCell dest;          /**< Destination cell */
    SylvesCellDir dir;        /**< Direction from source */
    SylvesCellDir inverse_dir; /**< Inverse direction from dest */
    SylvesConnection connection; /**< Connection between cells */
    float length;             /**< Length/cost of this step */
};

/**
 * @brief Path through a grid
 * 
 * Represents a complete path from source to destination,
 * consisting of a sequence of steps.
 */
struct SylvesCellPath {
    SylvesStep* steps;    /**< Array of steps */
    size_t step_count;    /**< Number of steps */
    float total_length;   /**< Total path length */
};

/**
 * @brief Callback for checking cell accessibility
 * 
 * @param cell Cell to check
 * @param user_data User-provided context
 * @return true if cell is accessible, false otherwise
 */
typedef bool (*SylvesIsAccessibleFunc)(SylvesCell cell, void* user_data);

/**
 * @brief Callback for computing step lengths/weights
 * 
 * @param step Step to evaluate
 * @param user_data User-provided context
 * @return Step length/cost, or negative value if step is invalid
 */
typedef float (*SylvesStepLengthFunc)(const SylvesStep* step, void* user_data);

/**
 * @brief Callback for heuristic functions (A* algorithm)
 * 
 * @param cell Cell to evaluate
 * @param user_data User-provided context
 * @return Heuristic estimate of distance to goal
 */
typedef float (*SylvesHeuristicFunc)(SylvesCell cell, void* user_data);

/* Basic pathfinding functions */

/**
 * @brief Find shortest path between two cells
 * 
 * @param grid Grid to search
 * @param src Source cell
 * @param dest Destination cell
 * @param is_accessible Optional accessibility check
 * @param step_lengths Optional step length function
 * @param user_data User data for callbacks
 * @return Path from src to dest, or NULL if no path exists
 */
SylvesCellPath* sylves_find_path(
    SylvesGrid* grid,
    SylvesCell src,
    SylvesCell dest,
    SylvesIsAccessibleFunc is_accessible,
    SylvesStepLengthFunc step_lengths,
    void* user_data);

/**
 * @brief Find distance between two cells
 * 
 * @param grid Grid to search
 * @param src Source cell
 * @param dest Destination cell
 * @param is_accessible Optional accessibility check
 * @param step_lengths Optional step length function
 * @param user_data User data for callbacks
 * @param distance Output distance
 * @return SYLVES_SUCCESS if path exists, error otherwise
 */
SylvesError sylves_find_distance(
    SylvesGrid* grid,
    SylvesCell src,
    SylvesCell dest,
    SylvesIsAccessibleFunc is_accessible,
    SylvesStepLengthFunc step_lengths,
    void* user_data,
    float* distance);

/* Path management */

/**
 * @brief Create a path from steps
 * 
 * @param steps Array of steps
 * @param step_count Number of steps
 * @return New path object
 */
SylvesCellPath* sylves_cell_path_create(
    const SylvesStep* steps,
    size_t step_count);

/**
 * @brief Destroy a path
 * 
 * @param path Path to destroy
 */
void sylves_cell_path_destroy(SylvesCellPath* path);

/**
 * @brief Get cells in path
 * 
 * @param path Path to query
 * @param cells Output array (must be pre-allocated with path->step_count + 1 elements)
 */
void sylves_cell_path_get_cells(
    const SylvesCellPath* path,
    SylvesCell* cells);

/* Step utilities */

/**
 * @brief Create a step from grid navigation
 * 
 * @param grid Grid to navigate
 * @param src Source cell
 * @param dir Direction to move
 * @param step_lengths Optional step length function
 * @param user_data User data for callback
 * @param step Output step
 * @return SYLVES_SUCCESS if step is valid, error otherwise
 */
SylvesError sylves_step_create(
    SylvesGrid* grid,
    SylvesCell src,
    SylvesCellDir dir,
    SylvesStepLengthFunc step_lengths,
    void* user_data,
    SylvesStep* step);

/**
 * @brief Get inverse of a step
 * 
 * @param step Step to invert
 * @param inverse Output inverse step
 */
void sylves_step_inverse(
    const SylvesStep* step,
    SylvesStep* inverse);

/* Heuristic functions */

/**
 * @brief Manhattan distance heuristic
 * 
 * @param current Current cell
 * @param target Target cell
 * @param scale Scale factor
 * @return Manhattan distance estimate
 */
float sylves_heuristic_manhattan(
    SylvesCell current,
    SylvesCell target,
    float scale);

/**
 * @brief Euclidean distance heuristic
 * 
 * @param grid Grid for cell positions
 * @param current Current cell
 * @param target Target cell
 * @return Euclidean distance estimate
 */
float sylves_heuristic_euclidean(
    SylvesGrid* grid,
    SylvesCell current,
    SylvesCell target);

/**
 * @brief Get admissible heuristic for grid type
 * 
 * @param grid Grid to analyze
 * @param target Target cell
 * @return Heuristic function, or NULL if none available
 */
SylvesHeuristicFunc sylves_get_admissible_heuristic(
    SylvesGrid* grid,
    SylvesCell target,
    void** user_data);

/* A* Pathfinding */

/**
 * @brief A* pathfinding context
 */
typedef struct SylvesAStarPathfinding SylvesAStarPathfinding;

/**
 * @brief Create A* pathfinding context
 * 
 * @param grid Grid to search
 * @param src Source cell
 * @param step_lengths Step length function
 * @param heuristic Heuristic function
 * @param user_data User data for callbacks
 * @return New A* context
 */
SylvesAStarPathfinding* sylves_astar_create(
    SylvesGrid* grid,
    SylvesCell src,
    SylvesStepLengthFunc step_lengths,
    SylvesHeuristicFunc heuristic,
    void* user_data);

/**
 * @brief Run A* algorithm to target
 * 
 * @param astar A* context
 * @param target Target cell
 */
void sylves_astar_run(
    SylvesAStarPathfinding* astar,
    SylvesCell target);

/**
 * @brief Extract path to target
 * 
 * @param astar A* context
 * @param target Target cell
 * @return Path to target, or NULL if no path exists
 */
SylvesCellPath* sylves_astar_extract_path(
    SylvesAStarPathfinding* astar,
    SylvesCell target);

/**
 * @brief Destroy A* context
 * 
 * @param astar Context to destroy
 */
void sylves_astar_destroy(SylvesAStarPathfinding* astar);

/* Dijkstra Pathfinding */

/**
 * @brief Dijkstra pathfinding context
 */
typedef struct SylvesDijkstraPathfinding SylvesDijkstraPathfinding;

/**
 * @brief Create Dijkstra pathfinding context
 * 
 * @param grid Grid to search
 * @param src Source cell
 * @param step_lengths Step length function
 * @param user_data User data for callbacks
 * @return New Dijkstra context
 */
SylvesDijkstraPathfinding* sylves_dijkstra_create(
    SylvesGrid* grid,
    SylvesCell src,
    SylvesStepLengthFunc step_lengths,
    void* user_data);

/**
 * @brief Run Dijkstra algorithm
 * 
 * @param dijkstra Dijkstra context
 * @param target Optional target cell (pass NULL for all cells)
 * @param max_range Maximum range to search
 */
void sylves_dijkstra_run(
    SylvesDijkstraPathfinding* dijkstra,
    const SylvesCell* target,
    float max_range);

/**
 * @brief Get computed distances
 * 
 * @param dijkstra Dijkstra context
 * @param cells Output array of cells
 * @param distances Output array of distances
 * @param count Number of entries (input/output)
 * @return SYLVES_SUCCESS or error
 */
SylvesError sylves_dijkstra_get_distances(
    SylvesDijkstraPathfinding* dijkstra,
    SylvesCell* cells,
    float* distances,
    size_t* count);

/**
 * @brief Extract path to target
 * 
 * @param dijkstra Dijkstra context
 * @param target Target cell
 * @return Path to target, or NULL if no path exists
 */
SylvesCellPath* sylves_dijkstra_extract_path(
    SylvesDijkstraPathfinding* dijkstra,
    SylvesCell target);

/**
 * @brief Destroy Dijkstra context
 * 
 * @param dijkstra Context to destroy
 */
void sylves_dijkstra_destroy(SylvesDijkstraPathfinding* dijkstra);

/* Breadth-First Search */

/**
 * @brief BFS pathfinding context
 */
typedef struct SylvesBFSPathfinding SylvesBFSPathfinding;

/**
 * @brief Create BFS pathfinding context
 * 
 * @param grid Grid to search
 * @param src Source cell
 * @param is_accessible Accessibility check
 * @param user_data User data for callbacks
 * @return New BFS context
 */
SylvesBFSPathfinding* sylves_bfs_create(
    SylvesGrid* grid,
    SylvesCell src,
    SylvesIsAccessibleFunc is_accessible,
    void* user_data);

/**
 * @brief Run BFS algorithm
 * 
 * @param bfs BFS context
 * @param targets Optional target cells
 * @param target_count Number of targets
 * @param max_distance Maximum distance to search
 */
void sylves_bfs_run(
    SylvesBFSPathfinding* bfs,
    const SylvesCell* targets,
    size_t target_count,
    int max_distance);

/**
 * @brief Check if cell was reached
 * 
 * @param bfs BFS context
 * @param cell Cell to check
 * @param distance Output distance to cell
 * @return true if cell was reached
 */
bool sylves_bfs_is_reachable(
    SylvesBFSPathfinding* bfs,
    SylvesCell cell,
    int* distance);

/**
 * @brief Extract path to target
 * 
 * @param bfs BFS context
 * @param target Target cell
 * @return Path to target, or NULL if no path exists
 */
SylvesCellPath* sylves_bfs_extract_path(
    SylvesBFSPathfinding* bfs,
    SylvesCell target);

/**
 * @brief Destroy BFS context
 * 
 * @param bfs Context to destroy
 */
void sylves_bfs_destroy(SylvesBFSPathfinding* bfs);

/* Spanning Tree Algorithms */

/**
 * @brief Edge in a graph
 */
typedef struct SylvesEdge {
    SylvesCell src;
    SylvesCell dest;
    float weight;
} SylvesEdge;

/**
 * @brief Generate minimum spanning tree using Kruskal's algorithm
 * 
 * @param grid Grid to analyze
 * @param cells Cells to include in tree
 * @param cell_count Number of cells
 * @param step_lengths Edge weight function
 * @param user_data User data for callback
 * @param edges Output edges of spanning tree
 * @param edge_count Output number of edges
 * @return SYLVES_SUCCESS or error
 */
SylvesError sylves_kruskal_mst(
    SylvesGrid* grid,
    const SylvesCell* cells,
    size_t cell_count,
    SylvesStepLengthFunc step_lengths,
    void* user_data,
    SylvesEdge** edges,
    size_t* edge_count);

/* Cell Outlining */

/**
 * @brief Outline segment
 */
typedef struct SylvesOutlineSegment {
    SylvesVector3 start;
    SylvesVector3 end;
} SylvesOutlineSegment;

/**
 * @brief Generate outline of cell region
 * 
 * @param grid Grid containing cells
 * @param cells Cells to outline
 * @param cell_count Number of cells
 * @param segments Output outline segments
 * @param segment_count Output number of segments
 * @return SYLVES_SUCCESS or error
 */
SylvesError sylves_outline_cells(
    SylvesGrid* grid,
    const SylvesCell* cells,
    size_t cell_count,
    SylvesOutlineSegment** segments,
    size_t* segment_count);

/* Priority Queue (Heap) Implementation */

/**
 * @brief Min-heap for priority queue operations
 */
struct SylvesHeap {
    void** items;
    float* keys;
    size_t capacity;
    size_t size;
};

/**
 * @brief Create a new heap
 * 
 * @param initial_capacity Initial capacity
 * @return New heap
 */
SylvesHeap* sylves_heap_create(size_t initial_capacity);

/**
 * @brief Destroy a heap
 * 
 * @param heap Heap to destroy
 */
void sylves_heap_destroy(SylvesHeap* heap);

/**
 * @brief Insert item into heap
 * 
 * @param heap Heap to insert into
 * @param item Item to insert
 * @param key Priority key
 */
void sylves_heap_insert(SylvesHeap* heap, void* item, float key);

/**
 * @brief Pop minimum item from heap
 * 
 * @param heap Heap to pop from
 * @return Minimum item, or NULL if empty
 */
void* sylves_heap_pop(SylvesHeap* heap);

/**
 * @brief Peek at minimum key
 * 
 * @param heap Heap to peek at
 * @param key Output key
 * @return true if heap is non-empty
 */
bool sylves_heap_peek_key(SylvesHeap* heap, float* key);

/**
 * @brief Check if heap is empty
 * 
 * @param heap Heap to check
 * @return true if empty
 */
bool sylves_heap_is_empty(SylvesHeap* heap);

/**
 * @brief Clear heap contents
 * 
 * @param heap Heap to clear
 */
void sylves_heap_clear(SylvesHeap* heap);

#ifdef __cplusplus
}
#endif

#endif /* SYLVES_PATHFINDING_H */
