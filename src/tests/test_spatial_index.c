#include "test_framework.h"
#include "sylves/spatial_index.h"
#include "sylves/grid.h"
#include "sylves/square_grid.h"
#include <math.h>
#include <stdlib.h>

/* Test basic spatial index operations */
static void test_spatial_index_basic(void) {
    SylvesSpatialIndexConfig config = {
        .type = SYLVES_SPATIAL_INDEX_GRID_HASH,
        .bucket_size = 1024,
        .thread_safe = false
    };
    
    SylvesSpatialIndex* index = sylves_spatial_index_create(&config, 2);
    TEST_ASSERT(index != NULL);
    
    /* Insert some cells */
    SylvesCell cell1 = {1, 2, 0};
    SylvesVector3 center1 = {1.5, 2.5, 0};
    
    SylvesError err = sylves_spatial_index_insert(index, &cell1, &center1, NULL);
    TEST_ASSERT(err == SYLVES_SUCCESS);
    
    SylvesCell cell2 = {3, 4, 0};
    SylvesVector3 center2 = {3.5, 4.5, 0};
    
    err = sylves_spatial_index_insert(index, &cell2, &center2, NULL);
    TEST_ASSERT(err == SYLVES_SUCCESS);
    
    /* Query AABB */
    struct QueryResult {
        SylvesCell* found_cells;
        int count;
        int capacity;
    } result = { NULL, 0, 10 };
    
    result.found_cells = (SylvesCell*)malloc(sizeof(SylvesCell) * result.capacity);
    
    bool query_visitor(const SylvesCell* cell, void* data, void* user_data) {
        struct QueryResult* res = (struct QueryResult*)user_data;
        if (res->count < res->capacity) {
            res->found_cells[res->count++] = *cell;
        }
        return true;
    }
    
    SylvesAabb aabb = {
        .min = {0, 0, 0},
        .max = {3, 3, 1}
    };
    
    err = sylves_spatial_index_query_aabb(index, &aabb, query_visitor, &result);
    TEST_ASSERT(err == SYLVES_SUCCESS);
    TEST_ASSERT(result.count == 1); /* Only cell1 should be in this AABB */
    TEST_ASSERT(result.found_cells[0].x == 1);
    TEST_ASSERT(result.found_cells[0].y == 2);
    
    /* Test removal */
    err = sylves_spatial_index_remove(index, &cell1);
    TEST_ASSERT(err == SYLVES_SUCCESS);
    
    result.count = 0;
    err = sylves_spatial_index_query_aabb(index, &aabb, query_visitor, &result);
    TEST_ASSERT(err == SYLVES_SUCCESS);
    TEST_ASSERT(result.count == 0); /* No cells should be found after removal */
    
    free(result.found_cells);
    sylves_spatial_index_destroy(index);
}

/* Test grid spatial hash */
static void test_grid_spatial_hash(void) {
    /* Create a square grid */
    SylvesVector2 cell_size = {1.0, 1.0};
    SylvesGrid* grid = sylves_square_grid_create(&cell_size);
    TEST_ASSERT(grid != NULL);
    
    /* Create bounded grid */
    SylvesBound* bounds = sylves_bound_create_rectangle(0, 0, 10, 10);
    SylvesGrid* bounded_grid = sylves_grid_bound_by(grid, bounds);
    
    /* Create spatial hash */
    SylvesGridSpatialHash* hash = sylves_grid_spatial_hash_create(bounded_grid, 0, false);
    TEST_ASSERT(hash != NULL);
    
    /* Insert all cells from bounds */
    SylvesError err = sylves_grid_spatial_hash_insert_bounds(hash, bounds);
    TEST_ASSERT(err == SYLVES_SUCCESS);
    
    /* Query a region */
    struct CellCollector {
        SylvesCell* cells;
        int count;
        int capacity;
    } collector = { NULL, 0, 100 };
    
    collector.cells = (SylvesCell*)malloc(sizeof(SylvesCell) * collector.capacity);
    
    bool collect_visitor(const SylvesCell* cell, void* user_data) {
        struct CellCollector* col = (struct CellCollector*)user_data;
        if (col->count < col->capacity) {
            col->cells[col->count++] = *cell;
        }
        return true;
    }
    
    SylvesVector3 min = {2.5, 2.5, -1};
    SylvesVector3 max = {5.5, 5.5, 1};
    
    err = sylves_grid_spatial_hash_query_aabb(hash, &min, &max, collect_visitor, &collector);
    TEST_ASSERT(err == SYLVES_SUCCESS);
    
    /* Should find cells (3,3), (3,4), (3,5), (4,3), (4,4), (4,5), (5,3), (5,4), (5,5) = 9 cells */
    TEST_ASSERT(collector.count == 9);
    
    /* Verify all found cells are in the expected range */
    for (int i = 0; i < collector.count; i++) {
        TEST_ASSERT(collector.cells[i].x >= 3 && collector.cells[i].x <= 5);
        TEST_ASSERT(collector.cells[i].y >= 3 && collector.cells[i].y <= 5);
    }
    
    free(collector.cells);
    sylves_grid_spatial_hash_destroy(hash);
    sylves_grid_destroy(bounded_grid);
    sylves_bound_destroy(bounds);
    sylves_grid_destroy(grid);
}

/* Test spatial index statistics */
static void test_spatial_index_stats(void) {
    SylvesSpatialIndexConfig config = {
        .type = SYLVES_SPATIAL_INDEX_GRID_HASH,
        .bucket_size = 100,
        .thread_safe = false
    };
    
    SylvesSpatialIndex* index = sylves_spatial_index_create(&config, 2);
    TEST_ASSERT(index != NULL);
    
    /* Insert many cells */
    for (int x = 0; x < 20; x++) {
        for (int y = 0; y < 20; y++) {
            SylvesCell cell = {x, y, 0};
            SylvesVector3 center = {x + 0.5, y + 0.5, 0};
            sylves_spatial_index_insert(index, &cell, &center, NULL);
        }
    }
    
    /* Get statistics */
    SylvesSpatialIndexStats stats;
    sylves_spatial_index_get_stats(index, &stats);
    
    TEST_ASSERT(stats.item_count == 400);
    TEST_ASSERT(stats.bucket_count == 100);
    TEST_ASSERT(stats.node_count > 0);
    TEST_ASSERT(stats.average_items_per_node > 0);
    
    sylves_spatial_index_destroy(index);
}

/* Test radius queries */
static void test_spatial_index_radius(void) {
    SylvesSpatialIndexConfig config = {
        .type = SYLVES_SPATIAL_INDEX_GRID_HASH,
        .bucket_size = 1024,
        .thread_safe = false
    };
    
    SylvesSpatialIndex* index = sylves_spatial_index_create(&config, 2);
    TEST_ASSERT(index != NULL);
    
    /* Insert cells in a pattern */
    for (int i = 0; i < 10; i++) {
        SylvesCell cell = {i, 0, 0};
        SylvesVector3 center = {i * 2.0, 0, 0};
        sylves_spatial_index_insert(index, &cell, &center, NULL);
    }
    
    /* Query with radius */
    struct RadiusResult {
        int count;
    } radius_result = { 0 };
    
    bool radius_visitor(const SylvesCell* cell, void* data, void* user_data) {
        struct RadiusResult* res = (struct RadiusResult*)user_data;
        res->count++;
        return true;
    }
    
    SylvesVector3 query_center = {5.0, 0, 0};
    double radius = 3.5;
    
    SylvesError err = sylves_spatial_index_query_radius(index, &query_center, radius, 
                                                       radius_visitor, &radius_result);
    TEST_ASSERT(err == SYLVES_SUCCESS);
    
    /* Should find cells at x=4 and x=6 (centers at 4.0 and 6.0) */
    TEST_ASSERT(radius_result.count == 2);
    
    sylves_spatial_index_destroy(index);
}

/* Test optimal hash size calculation */
static void test_optimal_hash_size(void) {
    /* Create a square grid with specific cell size */
    SylvesVector2 cell_size = {2.5, 2.5};
    SylvesGrid* grid = sylves_square_grid_create(&cell_size);
    TEST_ASSERT(grid != NULL);
    
    double optimal = sylves_grid_spatial_hash_optimal_size(grid, 10);
    
    /* Optimal size should be related to cell size */
    TEST_ASSERT(optimal > 0);
    TEST_ASSERT(optimal >= cell_size.x); /* Should be at least as large as cell size */
    
    sylves_grid_destroy(grid);
}

/* Performance test */
static void test_spatial_index_performance(void) {
    SylvesSpatialIndexConfig config = {
        .type = SYLVES_SPATIAL_INDEX_GRID_HASH,
        .bucket_size = 4096,
        .thread_safe = false
    };
    
    SylvesSpatialIndex* index = sylves_spatial_index_create(&config, 2);
    TEST_ASSERT(index != NULL);
    
    /* Insert many cells */
    const int grid_size = 100;
    clock_t start = clock();
    
    for (int x = 0; x < grid_size; x++) {
        for (int y = 0; y < grid_size; y++) {
            SylvesCell cell = {x, y, 0};
            SylvesVector3 center = {x + 0.5, y + 0.5, 0};
            sylves_spatial_index_insert(index, &cell, &center, NULL);
        }
    }
    
    clock_t insert_time = clock() - start;
    
    /* Perform many queries */
    int total_found = 0;
    start = clock();
    
    for (int i = 0; i < 1000; i++) {
        double x = (rand() / (double)RAND_MAX) * grid_size;
        double y = (rand() / (double)RAND_MAX) * grid_size;
        double size = 5.0;
        
        SylvesAabb aabb = {
            .min = {x - size, y - size, -1},
            .max = {x + size, y + size, 1}
        };
        
        struct CountResult {
            int count;
        } count_result = { 0 };
        
        bool count_visitor(const SylvesCell* cell, void* data, void* user_data) {
            struct CountResult* res = (struct CountResult*)user_data;
            res->count++;
            return true;
        }
        
        sylves_spatial_index_query_aabb(index, &aabb, count_visitor, &count_result);
        total_found += count_result.count;
    }
    
    clock_t query_time = clock() - start;
    
    TEST_MESSAGE("Spatial index performance: %d cells inserted in %ld ms, "
                 "1000 queries in %ld ms (found %d total cells)",
                 grid_size * grid_size, insert_time * 1000 / CLOCKS_PER_SEC,
                 query_time * 1000 / CLOCKS_PER_SEC, total_found);
    
    sylves_spatial_index_destroy(index);
}

/* Main test runner */
void test_spatial_index_main(void) {
    TEST_RUN(test_spatial_index_basic);
    TEST_RUN(test_grid_spatial_hash);
    TEST_RUN(test_spatial_index_stats);
    TEST_RUN(test_spatial_index_radius);
    TEST_RUN(test_optimal_hash_size);
    TEST_RUN(test_spatial_index_performance);
}
