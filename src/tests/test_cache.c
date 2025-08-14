#include "test_framework.h"
#include "sylves/cache.h"
#include "sylves/pathfinding.h"
#include "sylves/mesh_data.h"
#include <string.h>
#include <stdlib.h>

/* Test basic cache operations */
static void test_cache_basic(void) {
    SylvesCacheConfig config = {
        .max_entries = 10,
        .max_memory = 0,
        .policy = SYLVES_CACHE_POLICY_LRU,
        .thread_safe = false,
        .track_stats = true
    };
    
    /* Simple int key, string value cache */
    SylvesCache* cache = sylves_cache_create(&config, sizeof(int), NULL, NULL, free, strlen);
    TEST_ASSERT(cache != NULL);
    
    /* Put some values */
    int key1 = 1;
    char* value1 = strdup("Hello");
    SylvesError err = sylves_cache_put(cache, &key1, value1);
    TEST_ASSERT(err == SYLVES_SUCCESS);
    
    int key2 = 2;
    char* value2 = strdup("World");
    err = sylves_cache_put(cache, &key2, value2);
    TEST_ASSERT(err == SYLVES_SUCCESS);
    
    /* Get values */
    char* retrieved1 = (char*)sylves_cache_get(cache, &key1);
    TEST_ASSERT(retrieved1 != NULL);
    TEST_ASSERT(strcmp(retrieved1, "Hello") == 0);
    
    char* retrieved2 = (char*)sylves_cache_get(cache, &key2);
    TEST_ASSERT(retrieved2 != NULL);
    TEST_ASSERT(strcmp(retrieved2, "World") == 0);
    
    /* Test miss */
    int key3 = 3;
    char* retrieved3 = (char*)sylves_cache_get(cache, &key3);
    TEST_ASSERT(retrieved3 == NULL);
    
    /* Test statistics */
    SylvesCacheStats stats;
    sylves_cache_get_stats(cache, &stats);
    TEST_ASSERT(stats.hit_count == 2);
    TEST_ASSERT(stats.miss_count == 1);
    TEST_ASSERT(stats.total_entries == 2);
    TEST_ASSERT(stats.hit_rate > 60.0 && stats.hit_rate < 70.0);
    
    sylves_cache_destroy(cache);
}

/* Test LRU eviction */
static void test_cache_lru_eviction(void) {
    SylvesCacheConfig config = {
        .max_entries = 3,
        .max_memory = 0,
        .policy = SYLVES_CACHE_POLICY_LRU,
        .thread_safe = false,
        .track_stats = true
    };
    
    SylvesCache* cache = sylves_cache_create(&config, sizeof(int), NULL, NULL, NULL, NULL);
    TEST_ASSERT(cache != NULL);
    
    /* Fill cache to capacity */
    for (int i = 1; i <= 3; i++) {
        int* value = (int*)malloc(sizeof(int));
        *value = i * 10;
        sylves_cache_put(cache, &i, value);
    }
    
    /* Access first item to make it most recently used */
    int key1 = 1;
    int* val = (int*)sylves_cache_get(cache, &key1);
    TEST_ASSERT(val != NULL && *val == 10);
    
    /* Add new item, should evict item 2 (least recently used) */
    int key4 = 4;
    int* value4 = (int*)malloc(sizeof(int));
    *value4 = 40;
    sylves_cache_put(cache, &key4, value4);
    
    /* Check that item 2 was evicted */
    int key2 = 2;
    val = (int*)sylves_cache_get(cache, &key2);
    TEST_ASSERT(val == NULL);
    
    /* Items 1, 3, 4 should still be there */
    val = (int*)sylves_cache_get(cache, &key1);
    TEST_ASSERT(val != NULL && *val == 10);
    
    int key3 = 3;
    val = (int*)sylves_cache_get(cache, &key3);
    TEST_ASSERT(val != NULL && *val == 30);
    
    val = (int*)sylves_cache_get(cache, &key4);
    TEST_ASSERT(val != NULL && *val == 40);
    
    /* Check eviction count */
    SylvesCacheStats stats;
    sylves_cache_get_stats(cache, &stats);
    TEST_ASSERT(stats.eviction_count == 1);
    
    /* Cleanup */
    for (int i = 1; i <= 4; i++) {
        void* v = sylves_cache_get(cache, &i);
        if (v) free(v);
    }
    
    sylves_cache_destroy(cache);
}

/* Test cell cache */
static void test_cell_cache(void) {
    /* Create a dummy grid (NULL is fine for testing) */
    SylvesCellCache* cache = sylves_cell_cache_create(NULL, 100, false);
    TEST_ASSERT(cache != NULL);
    
    /* Test polygon caching */
    SylvesCell cell = {1, 2, 0};
    SylvesVector3 vertices[] = {
        {0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}
    };
    SylvesMatrix4x4 transform = sylves_matrix4x4_identity();
    
    SylvesError err = sylves_cell_cache_put_polygon(cache, &cell, vertices, 4, &transform);
    TEST_ASSERT(err == SYLVES_SUCCESS);
    
    /* Retrieve polygon */
    SylvesVector3* out_vertices;
    int out_count;
    SylvesMatrix4x4 out_transform;
    
    bool found = sylves_cell_cache_get_polygon(cache, &cell, &out_vertices, 
                                               &out_count, &out_transform);
    TEST_ASSERT(found);
    TEST_ASSERT(out_count == 4);
    TEST_ASSERT(out_vertices[0].x == 0);
    TEST_ASSERT(out_vertices[1].x == 1);
    
    /* Test miss */
    SylvesCell other_cell = {3, 4, 0};
    found = sylves_cell_cache_get_polygon(cache, &other_cell, &out_vertices, 
                                         &out_count, &out_transform);
    TEST_ASSERT(!found);
    
    sylves_cell_cache_destroy(cache);
}

/* Test path cache */
static void test_path_cache(void) {
    SylvesPathCache* cache = sylves_path_cache_create(50, false);
    TEST_ASSERT(cache != NULL);
    
    /* Create a path */
    SylvesCell start = {0, 0, 0};
    SylvesCell goal = {5, 5, 0};
    
    SylvesCellPath* path = sylves_cell_path_create();
    TEST_ASSERT(path != NULL);
    
    /* Add some steps */
    for (int i = 0; i <= 5; i++) {
        SylvesStep step = {
            .cell = {i, i, 0},
            .cost = 1.0
        };
        sylves_cell_path_add_step(path, &step);
    }
    
    /* Cache the path */
    SylvesError err = sylves_path_cache_put(cache, &start, &goal, path);
    TEST_ASSERT(err == SYLVES_SUCCESS);
    
    /* Retrieve path */
    SylvesCellPath* retrieved = sylves_path_cache_get(cache, &start, &goal);
    TEST_ASSERT(retrieved == path);
    TEST_ASSERT(retrieved->step_count == 6);
    
    /* Test different path not found */
    SylvesCell other_start = {1, 1, 0};
    retrieved = sylves_path_cache_get(cache, &other_start, &goal);
    TEST_ASSERT(retrieved == NULL);
    
    /* Test invalidation */
    SylvesCell changed = {3, 3, 0};
    sylves_path_cache_invalidate_cell(cache, &changed);
    
    /* Path should be invalidated */
    retrieved = sylves_path_cache_get(cache, &start, &goal);
    TEST_ASSERT(retrieved == NULL);
    
    sylves_path_cache_destroy(cache);
}

/* Test mesh cache */
static void test_mesh_cache(void) {
    SylvesMeshCache* cache = sylves_mesh_cache_create(10 * 1024 * 1024, false);
    TEST_ASSERT(cache != NULL);
    
    /* Create a simple mesh */
    SylvesMeshData* mesh = sylves_mesh_data_create(4, 2);
    TEST_ASSERT(mesh != NULL);
    
    /* Add vertices */
    mesh->vertices[0] = (SylvesVector3){0, 0, 0};
    mesh->vertices[1] = (SylvesVector3){1, 0, 0};
    mesh->vertices[2] = (SylvesVector3){1, 1, 0};
    mesh->vertices[3] = (SylvesVector3){0, 1, 0};
    
    /* Cache the mesh */
    uint64_t mesh_id = 12345;
    SylvesError err = sylves_mesh_cache_put(cache, mesh_id, mesh);
    TEST_ASSERT(err == SYLVES_SUCCESS);
    
    /* Retrieve mesh */
    SylvesMeshData* retrieved = sylves_mesh_cache_get(cache, mesh_id);
    TEST_ASSERT(retrieved == mesh);
    TEST_ASSERT(retrieved->vertex_count == 4);
    
    /* Test miss */
    uint64_t other_id = 99999;
    retrieved = sylves_mesh_cache_get(cache, other_id);
    TEST_ASSERT(retrieved == NULL);
    
    sylves_mesh_cache_destroy(cache);
}

/* Test cache policies */
static void test_cache_policies(void) {
    /* Test LFU policy */
    SylvesCacheConfig config = {
        .max_entries = 3,
        .max_memory = 0,
        .policy = SYLVES_CACHE_POLICY_LFU,
        .thread_safe = false,
        .track_stats = true
    };
    
    SylvesCache* cache = sylves_cache_create(&config, sizeof(int), NULL, NULL, NULL, NULL);
    TEST_ASSERT(cache != NULL);
    
    /* Add items and access them different numbers of times */
    for (int i = 1; i <= 3; i++) {
        int* value = (int*)malloc(sizeof(int));
        *value = i;
        sylves_cache_put(cache, &i, value);
    }
    
    /* Access item 1 three times */
    int key1 = 1;
    for (int i = 0; i < 3; i++) {
        sylves_cache_get(cache, &key1);
    }
    
    /* Access item 2 twice */
    int key2 = 2;
    for (int i = 0; i < 2; i++) {
        sylves_cache_get(cache, &key2);
    }
    
    /* Item 3 accessed only once (during put) */
    
    /* Add new item, should evict item 3 (least frequently used) */
    int key4 = 4;
    int* value4 = (int*)malloc(sizeof(int));
    *value4 = 4;
    sylves_cache_put(cache, &key4, value4);
    
    /* Check that item 3 was evicted */
    int key3 = 3;
    void* val = sylves_cache_get(cache, &key3);
    TEST_ASSERT(val == NULL);
    
    /* Items 1, 2, 4 should still be there */
    val = sylves_cache_get(cache, &key1);
    TEST_ASSERT(val != NULL);
    
    val = sylves_cache_get(cache, &key2);
    TEST_ASSERT(val != NULL);
    
    val = sylves_cache_get(cache, &key4);
    TEST_ASSERT(val != NULL);
    
    /* Cleanup */
    for (int i = 1; i <= 4; i++) {
        void* v = sylves_cache_get(cache, &i);
        if (v) free(v);
    }
    
    sylves_cache_destroy(cache);
}

/* Performance test */
static void test_cache_performance(void) {
    SylvesCacheConfig config = {
        .max_entries = 1000,
        .max_memory = 0,
        .policy = SYLVES_CACHE_POLICY_LRU,
        .thread_safe = false,
        .track_stats = true
    };
    
    SylvesCache* cache = sylves_cache_create(&config, sizeof(int), NULL, NULL, NULL, NULL);
    TEST_ASSERT(cache != NULL);
    
    const int num_operations = 10000;
    const int num_keys = 2000; /* Larger than cache to force evictions */
    
    clock_t start = clock();
    
    /* Perform mixed get/put operations */
    for (int i = 0; i < num_operations; i++) {
        int key = rand() % num_keys;
        
        if (rand() % 2 == 0 || i < 1000) {
            /* Put operation */
            int* value = (int*)malloc(sizeof(int));
            *value = key * 10;
            sylves_cache_put(cache, &key, value);
        } else {
            /* Get operation */
            int* val = (int*)sylves_cache_get(cache, &key);
            if (val) {
                TEST_ASSERT(*val == key * 10);
            }
        }
    }
    
    clock_t elapsed = clock() - start;
    
    /* Get final statistics */
    SylvesCacheStats stats;
    sylves_cache_get_stats(cache, &stats);
    
    TEST_MESSAGE("Cache performance: %d operations in %ld ms, "
                 "hit rate: %.1f%%, evictions: %zu, avg access: %.2f us",
                 num_operations, elapsed * 1000 / CLOCKS_PER_SEC,
                 stats.hit_rate, stats.eviction_count,
                 stats.average_access_time_us);
    
    /* Cleanup remaining entries */
    sylves_cache_clear(cache);
    sylves_cache_destroy(cache);
}

/* Main test runner */
void test_cache_main(void) {
    TEST_RUN(test_cache_basic);
    TEST_RUN(test_cache_lru_eviction);
    TEST_RUN(test_cell_cache);
    TEST_RUN(test_path_cache);
    TEST_RUN(test_mesh_cache);
    TEST_RUN(test_cache_policies);
    TEST_RUN(test_cache_performance);
}
