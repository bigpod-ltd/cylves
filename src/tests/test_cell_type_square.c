/**
 * @file test_cell_type_square.c
 * @brief Tests for Square CellType operations
 */

#include <sylves/sylves.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>

static int approx(double a, double b) { return fabs(a-b) < 1e-9; }

void test_square_cell_type_basic() {
    printf("Testing Square CellType basics...\n");
    SylvesCellType* ct = sylves_square_cell_type_create();
    assert(ct != NULL);

    // Dir and corner counts
    assert(sylves_cell_type_get_dir_count(ct) == 4);
    assert(sylves_cell_type_get_corner_count(ct) == 4);
    assert(sylves_cell_type_get_dimension(ct) == 2);

    // Rotations (no reflections): 0,1,2,3
    SylvesCellRotation rots[8];
    int rc = sylves_cell_type_get_rotations(ct, rots, 8, false);
    assert(rc == 4);
    for (int i=0;i<4;i++) assert(rots[i] == i);

    // Rotations + reflections: include ~i
    rc = sylves_cell_type_get_rotations(ct, rots, 8, true);
    assert(rc >= 4);

    // Multiply/invert
    assert(sylves_cell_type_multiply_rotations(ct, 1, 1) == 2);
    assert(sylves_cell_type_multiply_rotations(ct, 3, 1) == 0);
    assert(sylves_cell_type_invert_rotation(ct, 0) == 0);
    assert(sylves_cell_type_invert_rotation(ct, 1) == 3);
    assert(sylves_cell_type_invert_rotation(ct, 2) == 2);
    assert(sylves_cell_type_invert_rotation(ct, 3) == 1);

    // rotate_dir: Right(0) with 1->Up(1), 2->Left(2), 3->Down(3) using our conventions
    // Note: In square_grid.h, SYLVES_SQUARE_DIR_RIGHT==0, UP==1, LEFT==2, DOWN==3
    assert(sylves_cell_type_rotate_dir(ct, 0, 0) == 0);
    assert(sylves_cell_type_rotate_dir(ct, 0, 1) == 1);
    assert(sylves_cell_type_rotate_dir(ct, 0, 2) == 2);
    assert(sylves_cell_type_rotate_dir(ct, 0, 3) == 3);

    // invert_dir
    SylvesCellDir inv;
    assert(sylves_cell_type_invert_dir(ct, 0, &inv) && inv == 2);
    assert(sylves_cell_type_invert_dir(ct, 1, &inv) && inv == 3);

    // rotate_corner mapping: BR(0)->TR(1)->TL(2)->BL(3)
    assert(sylves_cell_type_rotate_corner(ct, 0, 0) == 0);
    assert(sylves_cell_type_rotate_corner(ct, 0, 1) == 1);
    assert(sylves_cell_type_rotate_corner(ct, 0, 2) == 2);
    assert(sylves_cell_type_rotate_corner(ct, 0, 3) == 3);

    // Corner positions canonical (centered at origin, side length 1)
    SylvesVector3 br = sylves_cell_type_get_corner_position(ct, 0);
    SylvesVector3 tr = sylves_cell_type_get_corner_position(ct, 1);
    SylvesVector3 tl = sylves_cell_type_get_corner_position(ct, 2);
    SylvesVector3 bl = sylves_cell_type_get_corner_position(ct, 3);
    assert(approx(br.x, 0.5) && approx(br.y, -0.5));
    assert(approx(tr.x, 0.5) && approx(tr.y, 0.5));
    assert(approx(tl.x, -0.5) && approx(tl.y, 0.5));
    assert(approx(bl.x, -0.5) && approx(bl.y, -0.5));

    // try_get_rotation: from Right(0) to Up(1) should be rotation 1
    SylvesCellRotation r;
    SylvesConnection conn = {0};
    assert(sylves_cell_type_try_get_rotation(ct, 0, 1, &conn, &r));
    assert(r == 1);

    // Rotation matrix: 90 degrees around Z for rotation 1
    SylvesMatrix4x4 m = sylves_cell_type_get_rotation_matrix(ct, 1);
    SylvesVector3 vx = sylves_matrix4x4_multiply_vector(&m, sylves_vector3_unit_x());
    assert(approx(vx.x, 0.0) && approx(vx.y, 1.0));

    sylves_cell_type_destroy(ct);
    printf("  Square CellType: PASSED\n");
}

int main(void) {
    test_square_cell_type_basic();
    return 0;
}

