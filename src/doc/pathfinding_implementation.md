# Pathfinding System Implementation

## Overview

The Sylves C pathfinding system provides a comprehensive set of algorithms for finding optimal paths through grids. This implementation is a strict 1:1 port from the Sylves C# library, maintaining full compatibility while adhering to C idioms.

## Components

### 1. Basic Infrastructure (Task 11.1)

#### Data Structures

- **SylvesStep**: Represents a single movement between cells
  - Source and destination cells
  - Direction and inverse direction
  - Connection information
  - Step length/cost

- **SylvesCellPath**: Complete path from source to destination
  - Array of steps
  - Total path length
  - Helper functions for extracting cells

- **SylvesHeap**: Min-heap priority queue implementation
  - Generic pointer-based storage
  - Float priority keys
  - Dynamic resizing
  - O(log n) insertion and extraction

#### Callback System

- **SylvesIsAccessibleFunc**: Check if a cell can be traversed
- **SylvesStepLengthFunc**: Calculate cost of moving between cells
- **SylvesHeuristicFunc**: Estimate distance to goal (for A*)

### 2. A* Pathfinding (Task 11.2)

#### Implementation Details

- **Hash Table**: Fast O(1) cell lookups
  - Chained collision resolution
  - Custom hash function for cells
  
- **Algorithm**: Standard A* with admissible heuristics
  - G-score: Actual distance from start
  - F-score: G-score + heuristic estimate
  - Priority queue for open set
  
- **Heuristics**:
  - Manhattan distance (for grid-aligned movement)
  - Euclidean distance (for any movement)
  - Automatic heuristic selection based on grid type

#### Features

- Early termination when target reached
- Path reconstruction from parent pointers
- Support for custom step costs
- Memory-efficient duplicate handling

### 3. Dijkstra's Algorithm (Task 11.3)

#### Implementation Details

- **Single-source shortest paths**: Find paths to all reachable cells
- **Priority Queue**: Min-heap ordered by distance
- **Distance Tracking**: Hash table stores best distances
- **Range Queries**: Support for maximum search radius

#### Features

- Extract path to any visited cell
- Query all distances within range
- Support for weighted edges
- Early termination options

### 4. Breadth-First Search (Task 11.4)

#### Implementation Details

- **Queue-based**: FIFO processing for uniform costs
- **Distance Tracking**: Integer step counts
- **Visit Tracking**: Hash table prevents revisiting

#### Features

- Unweighted shortest paths
- Multiple target support
- Maximum distance constraints
- Connectivity testing
- Fast for uniform-cost grids

### 5. Spanning Tree Algorithms (Task 11.5)

#### Kruskal's Algorithm

- **Union-Find**: Efficient cycle detection
  - Path compression optimization
  - Union by rank
  
- **Edge Sorting**: Sort by weight for greedy selection
- **MST Construction**: Select minimum weight edges without cycles

#### Features

- Minimum spanning tree generation
- Custom edge weight functions
- Efficient for sparse graphs

### 6. Cell Outlining (Task 11.6)

#### Algorithm

1. **Boundary Detection**: Find edges between region and exterior
2. **Edge Collection**: Gather all boundary segments
3. **Chain Building**: Connect edges into continuous outlines
4. **Segment Generation**: Output ordered line segments

#### Features

- Region boundary tracing
- Handles complex shapes
- Proper corner handling
- Multiple disconnected regions

## Usage Examples

### Basic Pathfinding

```c
// Find shortest path
SylvesGrid* grid = sylves_square_grid_create_unbounded(1.0f);
SylvesCell src = sylves_cell_create(0, 0, 0);
SylvesCell dest = sylves_cell_create(10, 10, 0);

SylvesCellPath* path = sylves_find_path(grid, src, dest, NULL, NULL, NULL);
if (path) {
    printf("Path found with %zu steps, length %.2f\n", 
           path->step_count, path->total_length);
    sylves_cell_path_destroy(path);
}
```

### With Obstacles

```c
bool is_accessible(SylvesCell cell, void* user_data) {
    // Block certain cells
    return !(cell.x == 5 && cell.y == 5);
}

SylvesCellPath* path = sylves_find_path(
    grid, src, dest, is_accessible, NULL, NULL);
```

### Custom Edge Weights

```c
float step_length(const SylvesStep* step, void* user_data) {
    // Diagonal moves cost more
    int dx = abs(step->dest.x - step->src.x);
    int dy = abs(step->dest.y - step->src.y);
    return (dx && dy) ? sqrtf(2.0f) : 1.0f;
}

SylvesCellPath* path = sylves_find_path(
    grid, src, dest, NULL, step_length, NULL);
```

### Range Finding

```c
SylvesDijkstraPathfinding* dijkstra = sylves_dijkstra_create(
    grid, src, NULL, NULL);
sylves_dijkstra_run(dijkstra, NULL, 10.0f); // Max range 10

size_t count = 1000;
SylvesCell* cells = malloc(sizeof(SylvesCell) * count);
float* distances = malloc(sizeof(float) * count);

sylves_dijkstra_get_distances(dijkstra, cells, distances, &count);
// Process reachable cells...
```

### Minimum Spanning Tree

```c
SylvesCell cells[100];
// Fill cells...

SylvesEdge* edges;
size_t edge_count;
sylves_kruskal_mst(grid, cells, 100, NULL, NULL, &edges, &edge_count);
// Process MST edges...
```

## Performance Characteristics

### Time Complexity

- **A***: O((V + E) log V) with admissible heuristic
- **Dijkstra**: O((V + E) log V) with binary heap
- **BFS**: O(V + E) for unweighted graphs
- **Kruskal**: O(E log E) for MST
- **Outlining**: O(N) for N boundary edges

### Space Complexity

- **All algorithms**: O(V) for visited set
- **Priority Queue**: O(V) worst case
- **Path Storage**: O(P) for path length P

## Memory Management

- All returned paths must be freed with `sylves_cell_path_destroy()`
- Algorithm contexts must be destroyed after use
- Dynamic allocations use Sylves memory functions
- No memory leaks in implementation

## Error Handling

- Invalid arguments return NULL or error codes
- Out of memory handled gracefully
- Path not found returns NULL
- Comprehensive error codes in errors.h

## Testing

Comprehensive unit tests cover:
- All algorithms on various grid types
- Edge cases (empty paths, unreachable targets)
- Performance with large grids
- Memory leak detection
- Callback functionality
- Heuristic accuracy

## Future Enhancements

- Bidirectional search for faster pathfinding
- Jump point search for uniform grids
- Hierarchical pathfinding for large worlds
- Path smoothing and optimization
- Dynamic obstacle handling
