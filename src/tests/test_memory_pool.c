#include "test_framework.h"
#include "sylves/memory_pool.h"
#include <string.h>
#include <stdlib.h>

/* Test basic memory pool operations */
static void test_memory_pool_basic(void) {
    SylvesPoolConfig config = {
        .block_size = 64,
        .initial_capacity = 10,
        .max_capacity = 100,
        .thread_safe = false,
        .track_stats = true,
        .zero_on_alloc = true
    };
    
    SylvesMemoryPool* pool = sylves_memory_pool_create(&config);
    TEST_ASSERT(pool != NULL);
    
    /* Test allocation */
    void* ptr1 = sylves_pool_alloc(pool);
    TEST_ASSERT(ptr1 != NULL);
    
    /* Test that memory is zeroed */
    char* bytes = (char*)ptr1;
    bool all_zero = true;
    for (size_t i = 0; i < config.block_size; i++) {
        if (bytes[i] != 0) {
            all_zero = false;
            break;
        }
    }
    TEST_ASSERT(all_zero);
    
    /* Test multiple allocations */
    void* ptrs[20];
    for (int i = 0; i < 20; i++) {
        ptrs[i] = sylves_pool_alloc(pool);
        TEST_ASSERT(ptrs[i] != NULL);
    }
    
    /* Test free and reuse */
    sylves_pool_free(pool, ptr1);
    void* ptr2 = sylves_pool_alloc(pool);
    TEST_ASSERT(ptr2 == ptr1); /* Should reuse the same block */
    
    /* Free all */
    for (int i = 0; i < 20; i++) {
        sylves_pool_free(pool, ptrs[i]);
    }
    sylves_pool_free(pool, ptr2);
    
    /* Test statistics */
    SylvesPoolStats stats;
    sylves_pool_get_stats(pool, &stats);
    TEST_ASSERT(stats.total_allocations == 21);
    TEST_ASSERT(stats.active_allocations == 0);
    TEST_ASSERT(stats.reuse_count > 0);
    
    sylves_memory_pool_destroy(pool);
}

/* Test cell pool operations */
static void test_cell_pool(void) {
    SylvesCellPool* pool = sylves_cell_pool_create(100, false);
    TEST_ASSERT(pool != NULL);
    
    /* Allocate cells */
    SylvesCell* cell1 = sylves_cell_pool_alloc(pool);
    TEST_ASSERT(cell1 != NULL);
    
    cell1->x = 10;
    cell1->y = 20;
    cell1->z = 30;
    
    /* Allocate array */
    size_t array_size = 50;
    SylvesCell* cell_array = sylves_cell_pool_alloc_array(pool, array_size);
    TEST_ASSERT(cell_array != NULL);
    
    for (size_t i = 0; i < array_size; i++) {
        cell_array[i].x = (int32_t)i;
        cell_array[i].y = (int32_t)(i * 2);
        cell_array[i].z = (int32_t)(i * 3);
    }
    
    /* Free and reuse */
    sylves_cell_pool_free(pool, cell1);
    SylvesCell* cell2 = sylves_cell_pool_alloc(pool);
    TEST_ASSERT(cell2 == cell1); /* Should reuse */
    
    sylves_cell_pool_free_array(pool, cell_array, array_size);
    sylves_cell_pool_free(pool, cell2);
    
    sylves_cell_pool_destroy(pool);
}

/* Test generic pool with various sizes */
static void test_generic_pool(void) {
    SylvesGenericPool* pool = sylves_generic_pool_create(16, 1024, false);
    TEST_ASSERT(pool != NULL);
    
    /* Test various allocation sizes */
    void* small = sylves_generic_pool_alloc(pool, 8);
    TEST_ASSERT(small != NULL);
    
    void* medium = sylves_generic_pool_alloc(pool, 100);
    TEST_ASSERT(medium != NULL);
    
    void* large = sylves_generic_pool_alloc(pool, 512);
    TEST_ASSERT(large != NULL);
    
    /* Test out-of-range allocations */
    void* too_small = sylves_generic_pool_alloc(pool, 4);
    TEST_ASSERT(too_small != NULL); /* Should fall back to regular alloc */
    
    void* too_large = sylves_generic_pool_alloc(pool, 2048);
    TEST_ASSERT(too_large != NULL); /* Should fall back to regular alloc */
    
    /* Free all */
    sylves_generic_pool_free(pool, small, 8);
    sylves_generic_pool_free(pool, medium, 100);
    sylves_generic_pool_free(pool, large, 512);
    sylves_generic_pool_free(pool, too_small, 4);
    sylves_generic_pool_free(pool, too_large, 2048);
    
    sylves_generic_pool_destroy(pool);
}

/* Test thread-local pools */
static void test_thread_local_pools(void) {
    /* Get thread-local cell pool */
    SylvesCellPool* cell_pool1 = sylves_get_thread_cell_pool();
    TEST_ASSERT(cell_pool1 != NULL);
    
    SylvesCellPool* cell_pool2 = sylves_get_thread_cell_pool();
    TEST_ASSERT(cell_pool1 == cell_pool2); /* Should be same instance */
    
    /* Use the pool */
    SylvesCell* cell = sylves_cell_pool_alloc(cell_pool1);
    TEST_ASSERT(cell != NULL);
    sylves_cell_pool_free(cell_pool1, cell);
    
    /* Get thread-local path pool */
    SylvesPathPool* path_pool1 = sylves_get_thread_path_pool();
    TEST_ASSERT(path_pool1 != NULL);
    
    SylvesPathPool* path_pool2 = sylves_get_thread_path_pool();
    TEST_ASSERT(path_pool1 == path_pool2); /* Should be same instance */
}

/* Test pool expansion and limits */
static void test_pool_limits(void) {
    SylvesPoolConfig config = {
        .block_size = 32,
        .initial_capacity = 5,
        .max_capacity = 10,
        .thread_safe = false,
        .track_stats = true,
        .zero_on_alloc = false
    };
    
    SylvesMemoryPool* pool = sylves_memory_pool_create(&config);
    TEST_ASSERT(pool != NULL);
    
    /* Allocate up to max capacity */
    void* ptrs[15];
    for (int i = 0; i < 10; i++) {
        ptrs[i] = sylves_pool_alloc(pool);
        TEST_ASSERT(ptrs[i] != NULL);
    }
    
    /* Should fail after max capacity */
    for (int i = 10; i < 15; i++) {
        ptrs[i] = sylves_pool_alloc(pool);
        TEST_ASSERT(ptrs[i] == NULL);
    }
    
    /* Free some and try again */
    sylves_pool_free(pool, ptrs[0]);
    sylves_pool_free(pool, ptrs[1]);
    
    void* ptr1 = sylves_pool_alloc(pool);
    TEST_ASSERT(ptr1 != NULL);
    
    void* ptr2 = sylves_pool_alloc(pool);
    TEST_ASSERT(ptr2 != NULL);
    
    /* Clean up */
    sylves_pool_free(pool, ptr1);
    sylves_pool_free(pool, ptr2);
    for (int i = 2; i < 10; i++) {
        sylves_pool_free(pool, ptrs[i]);
    }
    
    sylves_memory_pool_destroy(pool);
}

/* Performance benchmark */
static void test_pool_performance(void) {
    const size_t iterations = 100000;
    const size_t block_size = 64;
    
    /* Test regular allocation */
    clock_t start = clock();
    for (size_t i = 0; i < iterations; i++) {
        void* ptr = malloc(block_size);
        memset(ptr, 0, block_size);
        free(ptr);
    }
    clock_t regular_time = clock() - start;
    
    /* Test pool allocation */
    SylvesPoolConfig config = {
        .block_size = block_size,
        .initial_capacity = 1000,
        .max_capacity = 0,
        .thread_safe = false,
        .track_stats = false,
        .zero_on_alloc = true
    };
    
    SylvesMemoryPool* pool = sylves_memory_pool_create(&config);
    TEST_ASSERT(pool != NULL);
    
    start = clock();
    for (size_t i = 0; i < iterations; i++) {
        void* ptr = sylves_pool_alloc(pool);
        sylves_pool_free(pool, ptr);
    }
    clock_t pool_time = clock() - start;
    
    sylves_memory_pool_destroy(pool);
    
    /* Pool should be significantly faster */
    double speedup = (double)regular_time / (double)pool_time;
    TEST_MESSAGE("Pool speedup: %.2fx (regular: %ld, pool: %ld)", 
                 speedup, regular_time, pool_time);
    TEST_ASSERT(speedup > 2.0); /* At least 2x faster */
}

/* Main test runner */
void test_memory_pool_main(void) {
    TEST_RUN(test_memory_pool_basic);
    TEST_RUN(test_cell_pool);
    TEST_RUN(test_generic_pool);
    TEST_RUN(test_thread_local_pools);
    TEST_RUN(test_pool_limits);
    TEST_RUN(test_pool_performance);
}
