/**
 * @file test_hex_grid.c
 * @brief Tests for Hex grid core (5.1) and coordinate conversions (5.2)
 */

#include <sylves/sylves.h>
#include <assert.h>
#include <cstdio.h>
#include <math.h>

static void test_hex_conversions() {
    printf("Testing hex axial<->cube conversions...\n");
    for (int q = -3; q <= 3; q++) {
        for (int r = -3; r <= 3; r++) {
            int x,y,z; sylves_hex_axial_to_cube(q,r,&x,&y,&z);
            assert(x + y + z == 0);
            int q2,r2; sylves_hex_cube_to_axial(x,y,z,&q2,&r2);
            assert(q2 == q && r2 == r);
        }
    }
    printf("  axial<->cube: PASSED\n");

    printf("Testing hex even-q offset conversions...\n");
    for (int q = -3; q <= 3; q++) {
        for (int r = -3; r <= 3; r++) {
            int col,row; sylves_hex_axial_to_offset_evenq(q,r,&col,&row);
            int q2,r2; sylves_hex_offset_evenq_to_axial(col,row,&q2,&r2);
            assert(q2 == q && r2 == r);
        }
    }
    printf("  even-q offset: PASSED\n");
}

static void test_hex_creation_and_properties() {
    printf("Testing hex grid creation and properties...\n");
    SylvesGrid* g = sylves_hex_grid_create(SYLVES_HEX_ORIENTATION_FLAT_TOP, 1.0);
    assert(g != NULL);
    assert(sylves_grid_get_type(g) == SYLVES_GRID_TYPE_HEX);
    assert(sylves_grid_is_2d(g));
    assert(!sylves_grid_is_3d(g));
    assert(sylves_grid_is_planar(g));
    assert(sylves_grid_is_orientable(g));
    assert(!sylves_grid_is_finite(g));

    SylvesGrid* gb = sylves_hex_grid_create_bounded(SYLVES_HEX_ORIENTATION_POINTY_TOP, 1.5,
                                                    -2,-1, 3,4);
    assert(gb != NULL);
    assert(sylves_grid_is_finite(gb));

    // In-bounds cell (interpret cell.x,cell.y as axial q,r; z=0)
    assert(sylves_grid_is_cell_in_grid(gb, (SylvesCell){0,0,0}));
    // Out-of-bounds axial
    assert(!sylves_grid_is_cell_in_grid(gb, (SylvesCell){10,0,0}));

    // Directions and try_move
    SylvesCellDir dirs[6];
    int dcount = sylves_grid_get_cell_dirs(gb, (SylvesCell){0,0,0}, dirs, 6);
    assert(dcount == 6);
    SylvesCell dest;
    bool moved = sylves_grid_try_move(gb, (SylvesCell){0,0,0}, SYLVES_HEX_DIR_E, &dest, NULL, NULL);
    assert(moved);
    assert(dest.x == 1 && dest.y == 0 && dest.z == 0);

    sylves_grid_destroy(gb);
    sylves_grid_destroy(g);
    printf("  creation/properties: PASSED\n");
}

static void test_hex_spatial() {
    printf("Testing hex spatial operations...\n");
    // Validate polygon vertices equal corner positions order
    SylvesGrid* gcheck = sylves_hex_grid_create(SYLVES_HEX_ORIENTATION_FLAT_TOP, 1.0);
    SylvesCell cc = (SylvesCell){0,0,0};
    SylvesVector3 poly[6];
    assert(sylves_grid_get_polygon(gcheck, cc, poly, 6) == 6);
    for (int i=0;i<6;i++) {
        SylvesVector3 cp = sylves_grid_get_cell_corner(gcheck, cc, i);
        assert(fabs(cp.x - poly[i].x) < 1e-9);
        assert(fabs(cp.y - poly[i].y) < 1e-9);
    }
    sylves_grid_destroy(gcheck);
    printf("Testing hex spatial operations...\n");
    // Flat-top at cell (q=2,r=-1)
    SylvesGrid* gf = sylves_hex_grid_create(SYLVES_HEX_ORIENTATION_FLAT_TOP, 1.0);
    SylvesCell c = {2,-1,0};
    SylvesVector3 center = sylves_grid_get_cell_center(gf, c);
    // Expected: x = 3/2 * 2 = 3.0; y = sqrt(3) * (-1 + 1) = 0
    assert(fabs(center.x - 3.0) < 1e-9);
    assert(fabs(center.y - 0.0) < 1e-9);

    // Polygon: 6 vertices
    SylvesVector3 verts[6];
    int v = sylves_grid_get_polygon(gf, c, verts, 6);
    assert(v == 6);

    // find_cell should invert center
    SylvesCell found;
    assert(sylves_grid_find_cell(gf, center, &found));
    assert(found.x == c.x && found.y == c.y && found.z == 0);

    sylves_grid_destroy(gf);

    // Pointy-top sample
    SylvesGrid* gp = sylves_hex_grid_create(SYLVES_HEX_ORIENTATION_POINTY_TOP, 2.0);
    c = (SylvesCell){-1,3,0};
    center = sylves_grid_get_cell_center(gp, c);
    // Expected: x = sqrt(3) * 2 * (-1 + 3/2) = sqrt(3) * 2 * 0.5 = sqrt(3)
    //           y = 3/2 * 2 * 3 = 9
    assert(fabs(center.x - sqrt(3.0)) < 1e-9);
    assert(fabs(center.y - 9.0) < 1e-9);

    assert(sylves_grid_get_polygon(gp, c, verts, 6) == 6);
    assert(sylves_grid_find_cell(gp, center, &found));
    assert(found.x == c.x && found.y == c.y && found.z == 0);

    // AABB sizes for both orientations (match Sylves ComputeCellSize)
    SylvesAabb aabb;
    SylvesGrid* gf2 = sylves_hex_grid_create(SYLVES_HEX_ORIENTATION_FLAT_TOP, 2.0);
    assert(sylves_grid_get_cell_aabb(gf2, (SylvesCell){0,0,0}, &aabb) == SYLVES_SUCCESS);
    // Flat: width = s, height = s*sqrt(3)/2
    assert(fabs((aabb.max.x - aabb.min.x) - 2.0) < 1e-9);
    assert(fabs((aabb.max.y - aabb.min.y) - (2.0*sqrt(3.0)/2.0)) < 1e-9);
    sylves_grid_destroy(gf2);

    SylvesGrid* gp2 = sylves_hex_grid_create(SYLVES_HEX_ORIENTATION_POINTY_TOP, 3.0);
    assert(sylves_grid_get_cell_aabb(gp2, (SylvesCell){0,0,0}, &aabb) == SYLVES_SUCCESS);
    // Pointy: width = s*sqrt(3)/2, height = s
    assert(fabs((aabb.max.y - aabb.min.y) - 3.0) < 1e-9);
    assert(fabs((aabb.max.x - aabb.min.x) - (3.0*sqrt(3.0)/2.0)) < 1e-9);
    sylves_grid_destroy(gp2);

    sylves_grid_destroy(gp);
    printf("  hex spatial: PASSED\n");
}

static void test_hex_get_cells_in_aabb_and_bounds() {
    printf("Testing hex cells-in-aabb and bounds...\n");
    SylvesGrid* gb = sylves_hex_grid_create_bounded(SYLVES_HEX_ORIENTATION_FLAT_TOP, 1.0, -2, -2, 2, 1);
    // Query a box around origin
    SylvesVector3 min = sylves_vector3_create(-1.0, -1.0, 0.0);
    SylvesVector3 max = sylves_vector3_create( 1.0,  1.0, 0.0);
    SylvesCell cells[128];
    int n = sylves_grid_get_cells_in_aabb(gb, min, max, cells, 128);
    assert(n >= 1);
    // All returned cells must be in grid and their AABBs intersect
    for (int i=0;i<n;i++) {
        assert(sylves_grid_is_cell_in_grid(gb, cells[i]));
        SylvesAabb ca; sylves_grid_get_cell_aabb(gb, cells[i], &ca);
        assert(!(ca.max.x < min.x || ca.min.x > max.x || ca.max.y < min.y || ca.min.y > max.y));
    }
    sylves_grid_destroy(gb);

    // Bounds integration with hex-specific bound
    SylvesBound* hb = sylves_bound_create_hex_parallelogram(0,0,1,1);
    SylvesGrid* gh = sylves_hex_grid_create(SYLVES_HEX_ORIENTATION_POINTY_TOP, 1.0);
    SylvesGrid* ghb = sylves_grid_bound_by(gh, hb);
    assert(ghb && sylves_grid_is_finite(ghb));
    // Only (q,r) in [0..1]x[0..1] should be in
    for (int q=-1;q<=2;q++) for (int r=-1;r<=2;r++) {
        bool in = sylves_grid_is_cell_in_grid(ghb, (SylvesCell){q,r,0});
        bool expect = (q>=0 && q<=1 && r>=0 && r<=1);
        assert(in == expect);
    }
    sylves_bound_destroy(hb);
    sylves_grid_destroy(ghb);
    sylves_grid_destroy(gh);
    printf("  hex cells-in-aabb/bounds: PASSED\n");
}

int main(void) {
    test_hex_conversions();
    test_hex_creation_and_properties();
    test_hex_spatial();
    test_hex_get_cells_in_aabb_and_bounds();
    return 0;
}

