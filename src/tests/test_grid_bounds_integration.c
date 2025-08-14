/**
 * @file test_grid_bounds_integration.c
 * @brief Grid-level tests for bound_by and unbounded operations.
 */

#include <sylves/sylves.h>
#include <assert.h>
#include <stdio.h>

void test_grid_bound_by_and_unbounded() {
    printf("Testing grid bound_by and unbounded...\n");

    // Start with an unbounded grid of cell size 1.0
    SylvesGrid* g = sylves_square_grid_create(1.0);
    assert(g != NULL);
    assert(!sylves_grid_is_finite(g));

    // Bound by rectangle [0,0]..[2,1]
    SylvesBound* b = sylves_bound_create_rectangle(0,0,2,1);
    SylvesGrid* gb = sylves_grid_bound_by(g, b);
    assert(gb != NULL);
    assert(sylves_grid_is_finite(gb));

    // Check inclusion
    assert(sylves_grid_is_cell_in_grid(gb, (SylvesCell){0,0,0}));
    assert(!sylves_grid_is_cell_in_grid(gb, (SylvesCell){3,0,0}));

    // Check cell count = 3 * 2 = 6
    int cnt = sylves_grid_get_cell_count(gb);
    assert(cnt == 6);

    // Now unbounded clone from bounded grid
    SylvesGrid* gu = sylves_grid_unbounded(gb);
    assert(gu != NULL);
    assert(!sylves_grid_is_finite(gu));

    // Cleanup
    sylves_bound_destroy(b);
    sylves_grid_destroy(gu);
    sylves_grid_destroy(gb);
    sylves_grid_destroy(g);

    printf("  grid bound_by/unbounded: PASSED\n");
}

int main(void) {
    test_grid_bound_by_and_unbounded();
    return 0;
}

