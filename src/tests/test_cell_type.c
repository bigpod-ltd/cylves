#include <sylves/sylves.h>
#include <sylves/cell_type.h>
#include <sylves/vector.h>
#include <sylves/matrix.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>

static const double EPS = 1e-6;

static void assert_vec_eq(SylvesVector3 a, SylvesVector3 b, double eps) {
    assert(fabs(a.x-b.x) < eps && fabs(a.y-b.y) < eps && fabs(a.z-b.z) < eps);
}

static void test_square_cell_type() {
    printf("Testing Square cell type...\n");
    SylvesCellType* ct = sylves_square_cell_type_create();
    assert(sylves_cell_type_get_dimension(ct) == 2);
    assert(sylves_cell_type_get_dir_count(ct) == 4);
    assert(sylves_cell_type_get_corner_count(ct) == 4);

    // Invert dirs
    SylvesCellDir inv;
    assert(sylves_cell_type_invert_dir(ct, 0, &inv) && inv == 2);
    assert(sylves_cell_type_invert_dir(ct, 1, &inv) && inv == 3);

    // Rotations
    assert(sylves_cell_type_multiply_rotations(ct, 1, 1) == 2);
    assert(sylves_cell_type_invert_rotation(ct, 1) == 3);

    // Rotate dir
    assert(sylves_cell_type_rotate_dir(ct, 0, 1) == 1);

    // Corner positions
    assert_vec_eq(sylves_cell_type_get_corner_position(ct, 0), sylves_vector3_create(0.5,-0.5,0), EPS);
    assert_vec_eq(sylves_cell_type_get_corner_position(ct, 2), sylves_vector3_create(-0.5,0.5,0), EPS);

    sylves_cell_type_destroy(ct);
    printf("  Square: PASSED\n");
}

static void test_hex_cell_type() {
    printf("Testing Hex cell type...\n");
    SylvesCellType* ft = sylves_hex_cell_type_create(true);
    SylvesCellType* pt = sylves_hex_cell_type_create(false);

    assert(sylves_cell_type_get_dir_count(ft) == 6);
    assert(sylves_cell_type_get_corner_count(pt) == 6);

    // Multiply and invert
    assert(sylves_cell_type_multiply_rotations(ft, 2, ~0) == ~2);
    assert(sylves_cell_type_invert_rotation(ft, 2) == 4);

    // Try get rotation
    SylvesCellRotation r;
    SylvesConnection conn = { .rotation = 0, .is_mirror = false };
    assert(sylves_cell_type_try_get_rotation(ft, 1, 4, &conn, &r));

    // Corner positions exist
    SylvesVector3 p0 = sylves_cell_type_get_corner_position(ft, 0);
    SylvesVector3 p3 = sylves_cell_type_get_corner_position(ft, 3);
    assert(fabs(sqrt(p0.x*p0.x+p0.y*p0.y) - sqrt(p3.x*p3.x+p3.y*p3.y)) < 1e-6);

    sylves_cell_type_destroy(ft);
    sylves_cell_type_destroy(pt);
    printf("  Hex: PASSED\n");
}

static void test_triangle_cell_type() {
    printf("Testing Triangle cell type...\n");
    SylvesCellType* ft = sylves_triangle_cell_type_create(true);
    SylvesCellType* fs = sylves_triangle_cell_type_create(false);

    assert(sylves_cell_type_get_dir_count(ft) == 6);
    assert(sylves_cell_type_get_corner_count(fs) == 6);

    // Invert
    SylvesCellDir inv;
    assert(sylves_cell_type_invert_dir(ft, 0, &inv) && inv == 3);

    // Rotate corner
    assert(sylves_cell_type_rotate_corner(ft, 0, 1) == 1);

    // Corner positions up vs down different
    SylvesVector3 up0 = sylves_cell_type_get_corner_position(ft, 0);
    SylvesVector3 down0 = sylves_cell_type_get_corner_position(ft, 3);
    assert(fabs(up0.y - (-down0.y)) < 1e-6 || fabs(up0.x - (-down0.x)) < 1e-6);

    sylves_cell_type_destroy(ft);
    sylves_cell_type_destroy(fs);
    printf("  Triangle: PASSED\n");
}

static void test_cube_cell_type() {
    printf("Testing Cube cell type...\n");
    SylvesCellType* ct = sylves_cube_cell_type_create();

    assert(sylves_cell_type_get_dimension(ct) == 3);
    assert(sylves_cell_type_get_dir_count(ct) == 6);
    assert(sylves_cell_type_get_corner_count(ct) == 8);

    // Invert
    SylvesCellDir inv;
    assert(sylves_cell_type_invert_dir(ct, 1, &inv) && inv == 0);

    // Connection sense approximations
    SylvesCellDir out;
    SylvesConnection con;
    sylves_cell_type_get_connection(ct, 1, ~2, &out, &con); // reflect X flips Right->Left
    assert(out == 0 && con.is_mirror == true && con.rotation == 2);

    // Corner positions on unit cube
    SylvesVector3 c0 = sylves_cell_type_get_corner_position(ct, 0);
    SylvesVector3 c7 = sylves_cell_type_get_corner_position(ct, 7);
    assert(fabs(c0.x + 0.5) < EPS && fabs(c7.x - 0.5) < EPS);

    sylves_cell_type_destroy(ct);
    printf("  Cube: PASSED\n");
}

int main() {
    printf("Testing cell types...\n");
    test_square_cell_type();
    test_hex_cell_type();
    test_triangle_cell_type();
    test_cube_cell_type();
    printf("All cell type tests passed.\n");
    return 0;
}
