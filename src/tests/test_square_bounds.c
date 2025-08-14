/**
 * @file test_square_bounds.c
 * @brief Tests for Square bounds integration
 */

#include <sylves/sylves.h>
#include <assert.h>
#include <stdio.h>

void test_square_bounds_basic() {
    printf("Testing Square bounds...\n");
    SylvesBound* b = sylves_bound_create_rectangle(-2, -1, 3, 4);
    assert(b != NULL);

    // Contains
    assert(sylves_bound_contains(b, (SylvesCell){0,0,0}));
    assert(!sylves_bound_contains(b, (SylvesCell){4,0,0}));
    assert(!sylves_bound_contains(b, (SylvesCell){0,0,1}));

    // Get cells count
    SylvesCell cells[64];
    int n = sylves_bound_get_cells(b, cells, 64);
    assert(n == (3 - (-2) + 1) * (4 - (-1) + 1));

    // Intersect and union
    SylvesBound* b2 = sylves_bound_create_rectangle(0, 0, 1, 1);
    SylvesBound* bi = sylves_bound_intersect(b, b2);
    assert(bi != NULL);
    assert(sylves_bound_contains(bi, (SylvesCell){0,0,0}));
    assert(!sylves_bound_contains(bi, (SylvesCell){-1,0,0}));

    SylvesBound* bu = sylves_bound_union(b, b2);
    assert(bu != NULL);
    assert(sylves_bound_contains(bu, (SylvesCell){3,4,0}));

    sylves_bound_destroy(bu);
    sylves_bound_destroy(bi);
    sylves_bound_destroy(b2);
    sylves_bound_destroy(b);
    printf("  Square bounds: PASSED\n");
}

int main(void) {
    test_square_bounds_basic();
    return 0;
}

