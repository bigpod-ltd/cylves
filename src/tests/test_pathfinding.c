/**
 * @file test_pathfinding.c
 * @brief Unit tests for pathfinding algorithms
 */

#include <sylves/sylves.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

/* Test basic path infrastructure */
static void test_path_infrastructure(void) {
    printf("Testing path infrastructure...\n");
    
    // Test step creation
    SylvesStep steps[3];
    steps[0].src = sylves_cell_create(0, 0, 0);
    steps[0].dest = sylves_cell_create(1, 0, 0);
    steps[0].dir = (SylvesCellDir)0;
    steps[0].inverse_dir = (SylvesCellDir)2;
    steps[0].length = 1.0f;
    
    steps[1].src = sylves_cell_create(1, 0, 0);
    steps[1].dest = sylves_cell_create(1, 1, 0);
    steps[1].dir = (SylvesCellDir)1;
    steps[1].inverse_dir = (SylvesCellDir)3;
    steps[1].length = 1.0f;
    
    steps[2].src = sylves_cell_create(1, 1, 0);
    steps[2].dest = sylves_cell_create(2, 1, 0);
    steps[2].dir = (SylvesCellDir)0;
    steps[2].inverse_dir = (SylvesCellDir)2;
    steps[2].length = 1.0f;
    
    // Create path
    SylvesCellPath* path = sylves_cell_path_create(steps, 3);
    assert(path != NULL);
    assert(path->step_count == 3);
    assert(path->total_length == 3.0f);
    
    // Get cells
    SylvesCell cells[4];
    sylves_cell_path_get_cells(path, cells);
    assert(cells[0].x == 0 && cells[0].y == 0);
    assert(cells[1].x == 1 && cells[1].y == 0);
    assert(cells[2].x == 1 && cells[2].y == 1);
    assert(cells[3].x == 2 && cells[3].y == 1);
    
    sylves_cell_path_destroy(path);
    
    // Test empty path
    path = sylves_cell_path_create(NULL, 0);
    assert(path != NULL);
    assert(path->step_count == 0);
    assert(path->total_length == 0.0f);
    sylves_cell_path_destroy(path);
    
    printf("  Path infrastructure tests passed\n");
}

/* Test heap operations */
static void test_heap(void) {
    printf("Testing heap operations...\n");
    
    SylvesHeap* heap = sylves_heap_create(4);
    assert(heap != NULL);
    
    // Test empty heap
    assert(sylves_heap_is_empty(heap));
    assert(sylves_heap_pop(heap) == NULL);
    
    // Test insertion and retrieval
    int values[10] = {5, 3, 7, 1, 9, 2, 8, 4, 6, 0};
    for (int i = 0; i < 10; i++) {
        sylves_heap_insert(heap, &values[i], (float)values[i]);
    }
    
    assert(!sylves_heap_is_empty(heap));
    
    // Test peek
    float key;
    assert(sylves_heap_peek_key(heap, &key));
    assert(key == 0.0f);
    
    // Test extraction in sorted order
    for (int i = 0; i < 10; i++) {
        int* val = (int*)sylves_heap_pop(heap);
        assert(val != NULL);
        assert(*val == i);
    }
    
    assert(sylves_heap_is_empty(heap));
    
    // Test clear
    for (int i = 0; i < 5; i++) {
        sylves_heap_insert(heap, &values[i], (float)values[i]);
    }
    sylves_heap_clear(heap);
    assert(sylves_heap_is_empty(heap));
    
    sylves_heap_destroy(heap);
    
    printf("  Heap tests passed\n");
}

/* Test heuristic functions */
static void test_heuristics(void) {
    printf("Testing heuristic functions...\n");
    
    // Test Manhattan distance
    SylvesCell c1 = sylves_cell_create(0, 0, 0);
    SylvesCell c2 = sylves_cell_create(3, 4, 0);
    float dist = sylves_heuristic_manhattan(c1, c2, 1.0f);
    assert(dist == 7.0f);
    
    // Test with scale
    dist = sylves_heuristic_manhattan(c1, c2, 2.0f);
    assert(dist == 14.0f);
    
    // Test 3D
    c1 = sylves_cell_create(1, 2, 3);
    c2 = sylves_cell_create(4, 6, 8);
    dist = sylves_heuristic_manhattan(c1, c2, 1.0f);
    assert(dist == 12.0f); // |4-1| + |6-2| + |8-3| = 3 + 4 + 5 = 12
    
    // Test Euclidean distance
    SylvesGrid* grid = sylves_square_grid_create_unbounded(1.0f);
    c1 = sylves_cell_create(0, 0, 0);
    c2 = sylves_cell_create(3, 4, 0);
    dist = sylves_heuristic_euclidean(grid, c1, c2);
    assert(fabsf(dist - 5.0f) < 0.001f); // 3-4-5 triangle
    
    sylves_grid_destroy(grid);
    
    printf("  Heuristic tests passed\n");
}

/* Test accessibility callback */
static bool is_accessible_not_2_2(SylvesCell cell, void* user_data) {
    (void)user_data;
    // Block cell (2, 2)
    return !(cell.x == 2 && cell.y == 2);
}

/* Test custom step length */
static float custom_step_length(const SylvesStep* step, void* user_data) {
    (void)user_data;
    // Make diagonal moves cost sqrt(2)
    int dx = abs(step->dest.x - step->src.x);
    int dy = abs(step->dest.y - step->src.y);
    if (dx != 0 && dy != 0) {
        return sqrtf(2.0f);
    }
    return 1.0f;
}

/* Test BFS pathfinding */
static void test_bfs_pathfinding(void) {
    printf("Testing BFS pathfinding...\n");
    
    SylvesGrid* grid = sylves_square_grid_create_unbounded(1.0f);
    SylvesCell src = sylves_cell_create(0, 0, 0);
    SylvesCell dest = sylves_cell_create(3, 3, 0);
    
    // Test basic BFS
    SylvesBFSPathfinding* bfs = sylves_bfs_create(grid, src, NULL, NULL);
    assert(bfs != NULL);
    
    sylves_bfs_run(bfs, &dest, 1, -1);
    
    // Check if destination is reachable
    int distance;
    assert(sylves_bfs_is_reachable(bfs, dest, &distance));
    assert(distance == 6); // Manhattan distance on square grid
    
    // Extract path
    SylvesCellPath* path = sylves_bfs_extract_path(bfs, dest);
    assert(path != NULL);
    assert(path->step_count == 6);
    
    sylves_cell_path_destroy(path);
    sylves_bfs_destroy(bfs);
    
    // Test with obstacle
    bfs = sylves_bfs_create(grid, src, is_accessible_not_2_2, NULL);
    sylves_bfs_run(bfs, &dest, 1, -1);
    
    assert(sylves_bfs_is_reachable(bfs, dest, &distance));
    assert(distance > 6); // Should take longer path around obstacle
    
    path = sylves_bfs_extract_path(bfs, dest);
    assert(path != NULL);
    assert(path->step_count > 6);
    
    // Verify path doesn't go through (2,2)
    SylvesCell* cells = (SylvesCell*)malloc(sizeof(SylvesCell) * (path->step_count + 1));
    sylves_cell_path_get_cells(path, cells);
    for (size_t i = 0; i <= path->step_count; i++) {
        assert(!(cells[i].x == 2 && cells[i].y == 2));
    }
    free(cells);
    
    sylves_cell_path_destroy(path);
    sylves_bfs_destroy(bfs);
    
    // Test max distance
    bfs = sylves_bfs_create(grid, src, NULL, NULL);
    sylves_bfs_run(bfs, NULL, 0, 3); // Max distance 3
    
    assert(sylves_bfs_is_reachable(bfs, sylves_cell_create(2, 1, 0), &distance));
    assert(distance == 3);
    
    assert(!sylves_bfs_is_reachable(bfs, sylves_cell_create(3, 3, 0), &distance));
    
    sylves_bfs_destroy(bfs);
    sylves_grid_destroy(grid);
    
    printf("  BFS pathfinding tests passed\n");
}

/* Test Dijkstra pathfinding */
static void test_dijkstra_pathfinding(void) {
    printf("Testing Dijkstra pathfinding...\n");
    
    SylvesGrid* grid = sylves_square_grid_create_unbounded(1.0f);
    SylvesCell src = sylves_cell_create(0, 0, 0);
    SylvesCell dest = sylves_cell_create(3, 3, 0);
    
    // Test basic Dijkstra
    SylvesDijkstraPathfinding* dijkstra = sylves_dijkstra_create(grid, src, NULL, NULL);
    assert(dijkstra != NULL);
    
    sylves_dijkstra_run(dijkstra, &dest, FLT_MAX);
    
    // Extract path
    SylvesCellPath* path = sylves_dijkstra_extract_path(dijkstra, dest);
    assert(path != NULL);
    assert(path->step_count == 6);
    assert(path->total_length == 6.0f);
    
    sylves_cell_path_destroy(path);
    
    // Test distance query
    size_t count = 100;
    SylvesCell* cells = (SylvesCell*)malloc(sizeof(SylvesCell) * count);
    float* distances = (float*)malloc(sizeof(float) * count);
    
    SylvesError err = sylves_dijkstra_get_distances(dijkstra, cells, distances, &count);
    assert(err == SYLVES_SUCCESS);
    assert(count > 0);
    
    // Find destination in results
    bool found = false;
    for (size_t i = 0; i < count; i++) {
        if (sylves_cell_equals(cells[i], dest)) {
            assert(distances[i] == 6.0f);
            found = true;
            break;
        }
    }
    assert(found);
    
    free(cells);
    free(distances);
    sylves_dijkstra_destroy(dijkstra);
    
    // Test with custom step lengths (diagonal movement)
    // This test would need a grid that supports diagonal movement
    // For now, we'll just test the infrastructure works
    
    dijkstra = sylves_dijkstra_create(grid, src, custom_step_length, NULL);
    sylves_dijkstra_run(dijkstra, &dest, 10.0f); // Max range 10
    
    path = sylves_dijkstra_extract_path(dijkstra, dest);
    assert(path != NULL);
    
    sylves_cell_path_destroy(path);
    sylves_dijkstra_destroy(dijkstra);
    
    sylves_grid_destroy(grid);
    
    printf("  Dijkstra pathfinding tests passed\n");
}

/* Test A* pathfinding */
static void test_astar_pathfinding(void) {
    printf("Testing A* pathfinding...\n");
    
    SylvesGrid* grid = sylves_square_grid_create_unbounded(1.0f);
    SylvesCell src = sylves_cell_create(0, 0, 0);
    SylvesCell dest = sylves_cell_create(10, 10, 0);
    
    // Get admissible heuristic
    void* heuristic_data = NULL;
    SylvesHeuristicFunc heuristic = sylves_get_admissible_heuristic(grid, dest, &heuristic_data);
    assert(heuristic != NULL);
    
    // Test A* pathfinding
    SylvesAStarPathfinding* astar = sylves_astar_create(grid, src, NULL, heuristic, heuristic_data);
    assert(astar != NULL);
    
    sylves_astar_run(astar, dest);
    
    // Extract path
    SylvesCellPath* path = sylves_astar_extract_path(astar, dest);
    assert(path != NULL);
    assert(path->step_count == 20); // Manhattan distance
    assert(path->total_length == 20.0f);
    
    sylves_cell_path_destroy(path);
    sylves_astar_destroy(astar);
    free(heuristic_data);
    
    // Test high-level API
    path = sylves_find_path(grid, src, dest, NULL, NULL, NULL);
    assert(path != NULL);
    assert(path->step_count == 20);
    
    sylves_cell_path_destroy(path);
    
    // Test find_distance
    float distance;
    SylvesError err = sylves_find_distance(grid, src, dest, NULL, NULL, NULL, &distance);
    assert(err == SYLVES_SUCCESS);
    assert(distance == 20.0f);
    
    sylves_grid_destroy(grid);
    
    printf("  A* pathfinding tests passed\n");
}

/* Test spanning tree algorithms */
static void test_spanning_tree(void) {
    printf("Testing spanning tree algorithms...\n");
    
    SylvesGrid* grid = sylves_square_grid_create_unbounded(1.0f);
    
    // Create a small set of cells
    SylvesCell cells[9];
    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < 3; x++) {
            cells[y * 3 + x] = sylves_cell_create(x, y, 0);
        }
    }
    
    SylvesEdge* edges;
    size_t edge_count;
    
    SylvesError err = sylves_kruskal_mst(grid, cells, 9, NULL, NULL, &edges, &edge_count);
    assert(err == SYLVES_SUCCESS);
    assert(edge_count == 8); // n-1 edges for n nodes
    
    // Verify all edges have weight 1 (uniform grid)
    for (size_t i = 0; i < edge_count; i++) {
        assert(edges[i].weight == 1.0f);
    }
    
    free(edges);
    sylves_grid_destroy(grid);
    
    printf("  Spanning tree tests passed\n");
}

/* Test cell outlining */
static void test_cell_outlining(void) {
    printf("Testing cell outlining...\n");
    
    SylvesGrid* grid = sylves_square_grid_create_unbounded(1.0f);
    
    // Create a 3x3 block of cells
    SylvesCell cells[9];
    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < 3; x++) {
            cells[y * 3 + x] = sylves_cell_create(x, y, 0);
        }
    }
    
    SylvesOutlineSegment* segments;
    size_t segment_count;
    
    SylvesError err = sylves_outline_cells(grid, cells, 9, &segments, &segment_count);
    assert(err == SYLVES_SUCCESS);
    assert(segment_count > 0);
    
    // A 3x3 square should have 12 boundary edges
    // (4 sides * 3 edges per side = 12)
    assert(segment_count == 12);
    
    free(segments);
    
    // Test single cell
    err = sylves_outline_cells(grid, cells, 1, &segments, &segment_count);
    assert(err == SYLVES_SUCCESS);
    assert(segment_count == 4); // 4 edges for a square
    
    free(segments);
    sylves_grid_destroy(grid);
    
    printf("  Cell outlining tests passed\n");
}

int main(void) {
    printf("Running pathfinding tests...\n\n");
    
    test_path_infrastructure();
    test_heap();
    test_heuristics();
    test_bfs_pathfinding();
    test_dijkstra_pathfinding();
    test_astar_pathfinding();
    test_spanning_tree();
    test_cell_outlining();
    
    printf("\nAll pathfinding tests passed!\n");
    return 0;
}
