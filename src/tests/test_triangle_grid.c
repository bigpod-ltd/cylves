/**
 * @file test_triangle_grid.c
 * @brief Tests for triangle grid implementation
 */

#include <unity.h>
#include "sylves/triangle_grid.h"
#include "sylves/grid.h"
#include "sylves/vector.h"
#include <math.h>

void setUp(void) {}
void tearDown(void) {}

void test_triangle_grid_create(void) {
    SylvesGrid* grid = sylves_triangle_grid_create(1.0, SYLVES_TRIANGLE_ORIENTATION_FLAT_TOPPED);
    TEST_ASSERT_NOT_NULL(grid);
    
    TEST_ASSERT_TRUE(sylves_grid_is_2d(grid));
    TEST_ASSERT_FALSE(sylves_grid_is_3d(grid));
    TEST_ASSERT_TRUE(sylves_grid_is_planar(grid));
    TEST_ASSERT_TRUE(sylves_grid_is_repeating(grid));
    TEST_ASSERT_TRUE(sylves_grid_is_orientable(grid));
    TEST_ASSERT_FALSE(sylves_grid_is_finite(grid));
    TEST_ASSERT_EQUAL_INT(2, sylves_grid_get_coordinate_dimension(grid));
    
    sylves_grid_destroy(grid);
}

void test_triangle_grid_cell_center(void) {
    SylvesGrid* grid = sylves_triangle_grid_create(2.0, SYLVES_TRIANGLE_ORIENTATION_FLAT_TOPPED);
    TEST_ASSERT_NOT_NULL(grid);
    
    /* Test up-pointing triangle at (1,0,0) */
    SylvesCell cell = {1, 0, 0};
    SylvesVector3 center = sylves_grid_get_cell_center(grid, cell);
    
    /* Expected center calculation for flat-topped */
    double expected_x = (0.5 * 1 - 0.5 * 0) * 2.0;
    double expected_y = (-1/3.0 * 1 + 2/3.0 * 0 - 1/3.0 * 0) * 2.0;
    
    TEST_ASSERT_FLOAT_WITHIN(0.001, expected_x, center.x);
    TEST_ASSERT_FLOAT_WITHIN(0.001, expected_y, center.y);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 0.0, center.z);
    
    sylves_grid_destroy(grid);
}

void test_triangle_grid_try_move(void) {
    SylvesGrid* grid = sylves_triangle_grid_create(1.0, SYLVES_TRIANGLE_ORIENTATION_FLAT_TOPPED);
    TEST_ASSERT_NOT_NULL(grid);
    
    SylvesCell start = {0, 0, 1};
    SylvesCell dest;
    SylvesCellDir inverse_dir;
    SylvesConnection connection;
    
    /* Test move UpRight (dir 0) */
    bool result = sylves_grid_try_move(grid, start, 0, &dest, &inverse_dir, &connection);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(0, dest.x);
    TEST_ASSERT_EQUAL_INT(0, dest.y);
    TEST_ASSERT_EQUAL_INT(0, dest.z);
    TEST_ASSERT_EQUAL_INT(3, inverse_dir); /* (3 + 0) % 6 = 3 */
    
    sylves_grid_destroy(grid);
}

void test_triangle_grid_polygon(void) {
    SylvesGrid* grid = sylves_triangle_grid_create(1.0, SYLVES_TRIANGLE_ORIENTATION_FLAT_TOPPED);
    TEST_ASSERT_NOT_NULL(grid);
    
    SylvesCell cell = {0, 1, 0}; /* Should be up-pointing in flat-topped */
    SylvesVector3 vertices[3];
    
    int count = sylves_grid_get_polygon(grid, cell, vertices, 3);
    TEST_ASSERT_EQUAL_INT(3, count);
    
    /* Verify we got 3 distinct vertices */
    TEST_ASSERT_TRUE(fabs(vertices[0].x - vertices[1].x) > 0.001 ||
                     fabs(vertices[0].y - vertices[1].y) > 0.001);
    TEST_ASSERT_TRUE(fabs(vertices[1].x - vertices[2].x) > 0.001 ||
                     fabs(vertices[1].y - vertices[2].y) > 0.001);
    
    sylves_grid_destroy(grid);
}

void test_triangle_grid_find_cell(void) {
    SylvesGrid* grid = sylves_triangle_grid_create(1.0, SYLVES_TRIANGLE_ORIENTATION_FLAT_TOPPED);
    TEST_ASSERT_NOT_NULL(grid);
    
    /* Test finding cell at origin */
    SylvesVector3 pos = {0.0, 0.0, 0.0};
    SylvesCell cell;
    
    bool found = sylves_grid_find_cell(grid, pos, &cell);
    TEST_ASSERT_TRUE(found);
    
    /* Verify the cell coordinates sum to 1 or 2 (valid triangle) */
    int sum = cell.x + cell.y + cell.z;
    TEST_ASSERT_TRUE(sum == 1 || sum == 2);
    
    sylves_grid_destroy(grid);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_triangle_grid_create);
    RUN_TEST(test_triangle_grid_cell_center);
    RUN_TEST(test_triangle_grid_try_move);
    RUN_TEST(test_triangle_grid_polygon);
    RUN_TEST(test_triangle_grid_find_cell);
    
    return UNITY_END();
}
