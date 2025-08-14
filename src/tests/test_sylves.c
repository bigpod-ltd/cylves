/**
 * @file test_sylves.c
 * @brief Test program for Sylves library
 */

#include <sylves/sylves.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

void test_square_grid_basic() {
    printf("Testing square grid basic operations...\n");
    
    // Create a square grid with cell size 1.0
    SylvesGrid* grid = sylves_square_grid_create(1.0);
    assert(grid != NULL);
    
    // Test grid properties
    assert(sylves_grid_is_2d(grid) == true);
    assert(sylves_grid_is_3d(grid) == false);
    assert(sylves_grid_is_planar(grid) == true);
    assert(sylves_grid_is_repeating(grid) == true);
    assert(sylves_grid_is_orientable(grid) == true);
    assert(sylves_grid_is_finite(grid) == false);  // Unbounded grid
    assert(sylves_grid_get_coordinate_dimension(grid) == 2);
    
    // Test cell operations
    SylvesCell cell = sylves_cell_create_2d(0, 0);
    assert(sylves_grid_is_cell_in_grid(grid, cell) == true);
    
    // Test cell center
    SylvesVector3 center = sylves_grid_get_cell_center(grid, cell);
    assert(fabs(center.x - 0.5) < 1e-6);
    assert(fabs(center.y - 0.5) < 1e-6);
    assert(fabs(center.z - 0.0) < 1e-6);
    
    // Test corners
    SylvesCellCorner corners[4];
    int corner_count = sylves_grid_get_cell_corners(grid, cell, corners, 4);
    assert(corner_count == 4);
    
    // Test corner positions
    SylvesVector3 corner0 = sylves_grid_get_cell_corner(grid, cell, SYLVES_SQUARE_CORNER_BOTTOM_LEFT);
    assert(fabs(corner0.x - 0.0) < 1e-6);
    assert(fabs(corner0.y - 0.0) < 1e-6);
    
    SylvesVector3 corner1 = sylves_grid_get_cell_corner(grid, cell, SYLVES_SQUARE_CORNER_TOP_RIGHT);
    assert(fabs(corner1.x - 1.0) < 1e-6);
    assert(fabs(corner1.y - 1.0) < 1e-6);
    
    // Clean up
    sylves_grid_destroy(grid);
    printf("  Basic operations: PASSED\n");
}

void test_square_grid_navigation() {
    printf("Testing square grid navigation...\n");
    
    SylvesGrid* grid = sylves_square_grid_create(1.0);
    assert(grid != NULL);
    
    SylvesCell start = sylves_cell_create_2d(0, 0);
    
    // Test movement in each direction
    SylvesCell dest;
    SylvesCellDir inverse_dir;
    SylvesConnection connection;
    
    // Move right
    bool moved = sylves_grid_try_move(grid, start, SYLVES_SQUARE_DIR_RIGHT, 
                                      &dest, &inverse_dir, &connection);
    assert(moved == true);
    assert(dest.x == 1 && dest.y == 0);
    assert(inverse_dir == SYLVES_SQUARE_DIR_LEFT);
    assert(connection.is_mirror == false);
    
    // Move up
    moved = sylves_grid_try_move(grid, start, SYLVES_SQUARE_DIR_UP,
                                 &dest, &inverse_dir, &connection);
    assert(moved == true);
    assert(dest.x == 0 && dest.y == 1);
    assert(inverse_dir == SYLVES_SQUARE_DIR_DOWN);
    
    // Move left
    moved = sylves_grid_try_move(grid, start, SYLVES_SQUARE_DIR_LEFT,
                                 &dest, &inverse_dir, &connection);
    assert(moved == true);
    assert(dest.x == -1 && dest.y == 0);
    assert(inverse_dir == SYLVES_SQUARE_DIR_RIGHT);
    
    // Move down
    moved = sylves_grid_try_move(grid, start, SYLVES_SQUARE_DIR_DOWN,
                                 &dest, &inverse_dir, &connection);
    assert(moved == true);
    assert(dest.x == 0 && dest.y == -1);
    assert(inverse_dir == SYLVES_SQUARE_DIR_UP);
    
    // Test getting all directions
    SylvesCellDir dirs[4];
    int dir_count = sylves_grid_get_cell_dirs(grid, start, dirs, 4);
    assert(dir_count == 4);
    
    sylves_grid_destroy(grid);
    printf("  Navigation: PASSED\n");
}

void test_square_grid_bounded() {
    printf("Testing bounded square grid...\n");
    
    // Create a bounded grid from (-2, -2) to (2, 2)
    SylvesGrid* grid = sylves_square_grid_create_bounded(1.0, -2, -2, 2, 2);
    assert(grid != NULL);
    
    assert(sylves_grid_is_finite(grid) == true);
    
    // Test cells in bounds
    SylvesCell cell_in = sylves_cell_create_2d(0, 0);
    assert(sylves_grid_is_cell_in_grid(grid, cell_in) == true);
    
    cell_in = sylves_cell_create_2d(2, 2);
    assert(sylves_grid_is_cell_in_grid(grid, cell_in) == true);
    
    cell_in = sylves_cell_create_2d(-2, -2);
    assert(sylves_grid_is_cell_in_grid(grid, cell_in) == true);
    
    // Test cells out of bounds
    SylvesCell cell_out = sylves_cell_create_2d(3, 0);
    assert(sylves_grid_is_cell_in_grid(grid, cell_out) == false);
    
    cell_out = sylves_cell_create_2d(0, -3);
    assert(sylves_grid_is_cell_in_grid(grid, cell_out) == false);
    
    // Test movement at boundary
    SylvesCell boundary = sylves_cell_create_2d(2, 0);
    SylvesCell dest;
    bool moved = sylves_grid_try_move(grid, boundary, SYLVES_SQUARE_DIR_RIGHT,
                                      &dest, NULL, NULL);
    assert(moved == false);  // Can't move outside bounds
    
    moved = sylves_grid_try_move(grid, boundary, SYLVES_SQUARE_DIR_LEFT,
                                 &dest, NULL, NULL);
    assert(moved == true);   // Can move inside bounds
    assert(dest.x == 1 && dest.y == 0);
    
    // Test enumeration count
    int count = sylves_grid_get_cell_count(grid);
    assert(count == 25);

    SylvesCell cells[25];
    int written = sylves_grid_get_cells(grid, cells, 25);
    assert(written == 25);

    // Test cell type is available and named
    const SylvesCellType* ct = sylves_grid_get_cell_type(grid, sylves_cell_create_2d(0,0));
    assert(ct != NULL);

    // AABB query inside bounds
    SylvesVector3 min = sylves_vector3_create(-0.1, -0.1, 0.0);
    SylvesVector3 max = sylves_vector3_create(1.9, 1.9, 0.0);
    SylvesCell aabb_cells[16];
    int aabb_written = sylves_grid_get_cells_in_aabb(grid, min, max, aabb_cells, 16);
    // Should include cells (0,0), (1,0), (0,1), (1,1)
    assert(aabb_written == 4);

    sylves_grid_destroy(grid);
    printf("  Bounded grid: PASSED\n");
}

void test_square_grid_find_cell() {
    printf("Testing find cell...\n");
    
    SylvesGrid* grid = sylves_square_grid_create(2.0);  // Cell size 2.0
    assert(grid != NULL);
    
    // Test various positions
    SylvesVector3 pos;
    SylvesCell found;
    
    // Position in cell (0, 0)
    pos = sylves_vector3_create(1.0, 1.0, 0.0);
    assert(sylves_grid_find_cell(grid, pos, &found) == true);
    assert(found.x == 0 && found.y == 0);
    
    // Position in cell (1, 1)
    pos = sylves_vector3_create(3.0, 3.0, 0.0);
    assert(sylves_grid_find_cell(grid, pos, &found) == true);
    assert(found.x == 1 && found.y == 1);
    
    // Position in cell (-1, -1)
    pos = sylves_vector3_create(-1.0, -1.0, 0.0);
    assert(sylves_grid_find_cell(grid, pos, &found) == true);
    assert(found.x == -1 && found.y == -1);
    
    // Edge case: exactly on boundary (should go to lower cell)
    pos = sylves_vector3_create(2.0, 2.0, 0.0);
    assert(sylves_grid_find_cell(grid, pos, &found) == true);
    assert(found.x == 1 && found.y == 1);
    
    sylves_grid_destroy(grid);
    printf("  Find cell: PASSED\n");
}

void test_square_grid_polygon() {
    printf("Testing polygon extraction...\n");
    
    SylvesGrid* grid = sylves_square_grid_create(1.0);
    assert(grid != NULL);
    
    SylvesCell cell = sylves_cell_create_2d(1, 2);
    SylvesVector3 vertices[10];
    
    int vertex_count = sylves_grid_get_polygon(grid, cell, vertices, 10);
    assert(vertex_count == 4);
    
    // Check vertices (should be in order: bottom-right, top-right, top-left, bottom-left)
    assert(fabs(vertices[0].x - 2.0) < 1e-6 && fabs(vertices[0].y - 2.0) < 1e-6);
    assert(fabs(vertices[1].x - 2.0) < 1e-6 && fabs(vertices[1].y - 3.0) < 1e-6);
    assert(fabs(vertices[2].x - 1.0) < 1e-6 && fabs(vertices[2].y - 3.0) < 1e-6);
    assert(fabs(vertices[3].x - 1.0) < 1e-6 && fabs(vertices[3].y - 2.0) < 1e-6);
    
    sylves_grid_destroy(grid);
    printf("  Polygon extraction: PASSED\n");
}

void test_vector_operations() {
    printf("Testing vector operations...\n");
    
    // Test vector creation and basic operations
    SylvesVector3 v1 = sylves_vector3_create(1.0, 2.0, 3.0);
    SylvesVector3 v2 = sylves_vector3_create(4.0, 5.0, 6.0);
    
    // Addition
    SylvesVector3 sum = sylves_vector3_add(v1, v2);
    assert(fabs(sum.x - 5.0) < 1e-6);
    assert(fabs(sum.y - 7.0) < 1e-6);
    assert(fabs(sum.z - 9.0) < 1e-6);
    
    // Subtraction
    SylvesVector3 diff = sylves_vector3_subtract(v2, v1);
    assert(fabs(diff.x - 3.0) < 1e-6);
    assert(fabs(diff.y - 3.0) < 1e-6);
    assert(fabs(diff.z - 3.0) < 1e-6);
    
    // Dot product
    double dot = sylves_vector3_dot(v1, v2);
    assert(fabs(dot - 32.0) < 1e-6);  // 1*4 + 2*5 + 3*6 = 32
    
    // Cross product
    SylvesVector3 cross = sylves_vector3_cross(
        sylves_vector3_unit_x(),
        sylves_vector3_unit_y()
    );
    assert(fabs(cross.x - 0.0) < 1e-6);
    assert(fabs(cross.y - 0.0) < 1e-6);
    assert(fabs(cross.z - 1.0) < 1e-6);
    
    // Length
    SylvesVector3 v3 = sylves_vector3_create(3.0, 4.0, 0.0);
    double len = sylves_vector3_length(v3);
    assert(fabs(len - 5.0) < 1e-6);
    
    // Normalize
    SylvesVector3 norm = sylves_vector3_normalize(v3);
    assert(fabs(norm.x - 0.6) < 1e-6);
    assert(fabs(norm.y - 0.8) < 1e-6);
    assert(fabs(sylves_vector3_length(norm) - 1.0) < 1e-6);
    
    printf("  Vector operations: PASSED\n");
}

void test_find_basic_path() {
    printf("Testing find_basic_path (BFS)...\n");
    SylvesGrid* grid = sylves_square_grid_create_bounded(1.0, 0, 0, 2, 2);
    SylvesCell start = sylves_cell_create_2d(0,0);
    SylvesCell dest  = sylves_cell_create_2d(2,2);
    SylvesCell path[16];
    SylvesCellDir dirs[16];
    int len = sylves_grid_find_basic_path(grid, start, dest, path, dirs, 16);
    assert(len >= 0); /* non-negative length expected */
    assert(len != SYLVES_ERROR_PATH_NOT_FOUND);
    assert(len != SYLVES_ERROR_INFINITE_GRID);
    assert(len != SYLVES_ERROR_BUFFER_TOO_SMALL);
    assert(len != SYLVES_ERROR_CELL_NOT_IN_GRID);
    assert(len != SYLVES_ERROR_NOT_IMPLEMENTED);
    // Path should start at start and end at dest
    assert(path[0].x == 0 && path[0].y == 0);
    assert(path[len-1].x == 2 && path[len-1].y == 2);
    sylves_grid_destroy(grid);
    printf("  find_basic_path: PASSED\n");
}

int main() {
    printf("\n=== Sylves C Library Test Suite ===\n\n");
    
    test_vector_operations();
    test_square_grid_basic();
    test_square_grid_navigation();
    test_square_grid_bounded();
    test_square_grid_find_cell();
    test_square_grid_polygon();
    
    printf("\n=== All tests PASSED ===\n\n");
    
    // Example usage as shown in README
    printf("Example usage:\n");
    printf("--------------\n");
    
    SylvesGrid* grid = sylves_square_grid_create(1.0);
    SylvesCell cell = sylves_cell_create_2d(0, 0);
    
    SylvesVector3 center = sylves_grid_get_cell_center(grid, cell);
    printf("Cell (0,0) center: (%f, %f, %f)\n", center.x, center.y, center.z);
    
    SylvesCellDir dirs[4];
    size_t dir_count = sylves_grid_get_cell_dirs(grid, cell, dirs, 4);
    
    for (size_t i = 0; i < dir_count; i++) {
        SylvesCell neighbor;
        SylvesCellDir inverse_dir;
        SylvesConnection connection;
        
        if (sylves_grid_try_move(grid, cell, dirs[i], 
                                 &neighbor, &inverse_dir, &connection)) {
            printf("Neighbor at direction %d: (%d, %d, %d)\n", 
                   dirs[i], neighbor.x, neighbor.y, neighbor.z);
        }
    }
    
    sylves_grid_destroy(grid);
    
    return 0;
}
