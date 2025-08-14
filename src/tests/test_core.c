#include <sylves/sylves.h>
#include <sylves/vector.h>
#include <sylves/matrix.h>
#include <sylves/quaternion.h>
#include <sylves/aabb.h>
#include <sylves/trs.h>
#include <sylves/memory.h>
#include <sylves/connection.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static const double EPS = 1e-6;

static void test_errors() {
    printf("Testing errors...\n");
    assert(strcmp(sylves_error_string(SYLVES_SUCCESS), "Success") == 0);
    assert(strcmp(sylves_error_string(SYLVES_ERROR_OUT_OF_MEMORY), "Memory allocation failed") == 0);
    printf("  errors: PASSED\n");
}

static void test_vector_math() {
    printf("Testing vector math...\n");
    SylvesVector3 a = sylves_vector3_create(1,2,3);
    SylvesVector3 b = sylves_vector3_create(4,5,6);
    SylvesVector3 c = sylves_vector3_add(a,b);
    assert(fabs(c.x-5) < EPS && fabs(c.y-7) < EPS && fabs(c.z-9) < EPS);
    SylvesVector3 d = sylves_vector3_cross(a,b);
    assert(fabs(d.x + 3) < EPS && fabs(d.y - 6) < EPS && fabs(d.z + -3) < EPS);
    assert(fabs(sylves_vector3_dot(a,b) - 32.0) < EPS);
    SylvesVector3 n = sylves_vector3_normalize(sylves_vector3_create(3,0,4));
    assert(fabs(n.x - 0.6) < EPS && fabs(n.z - 0.8) < EPS);
    printf("  vector: PASSED\n");
}

static void test_matrix_math() {
    printf("Testing matrix math...\n");
    SylvesMatrix4x4 I = sylves_matrix4x4_identity();
    SylvesVector3 p = sylves_vector3_create(1,2,3);
    SylvesVector3 p2 = sylves_matrix4x4_multiply_point(&I, p);
    assert(sylves_vector3_approx_equal(p,p2,EPS));

    SylvesMatrix4x4 T = sylves_matrix4x4_translation(sylves_vector3_create(10,0,-5));
    SylvesVector3 tp = sylves_matrix4x4_multiply_point(&T, p);
    assert(fabs(tp.x-11) < EPS && fabs(tp.y-2) < EPS && fabs(tp.z-2) < EPS);

    SylvesMatrix4x4 Rz = sylves_matrix4x4_rotation_z(M_PI/2);
    SylvesVector3 vx = sylves_vector3_unit_x();
    SylvesVector3 ry = sylves_matrix4x4_multiply_vector(&Rz, vx);
    assert(fabs(ry.x) < 1e-6 && fabs(ry.y - 1.0) < 1e-6);

    SylvesMatrix4x4 A = sylves_matrix4x4_multiply(&T, &Rz);
    SylvesMatrix4x4 invA;
    assert(sylves_matrix4x4_invert(&A, &invA));
    SylvesMatrix4x4 shouldBeI = sylves_matrix4x4_multiply(&A, &invA);
    for (int i=0;i<16;i++) {
        double expected = (i%5==0) ? 1.0 : 0.0;
        assert(fabs(shouldBeI.m[i]-expected) < 1e-5);
    }
    printf("  matrix: PASSED\n");
}

static void test_quaternion_math() {
    printf("Testing quaternion math...\n");
    SylvesQuaternion q = sylves_quaternion_from_axis_angle(sylves_vector3_unit_z(), M_PI/2);
    SylvesVector3 vx = sylves_vector3_unit_x();
    SylvesVector3 vy = sylves_quaternion_rotate_vector(q, vx);
    assert(fabs(vy.x) < 1e-6 && fabs(vy.y - 1.0) < 1e-6);

    SylvesQuaternion qa = sylves_quaternion_from_euler(0, 0, 0);
    SylvesQuaternion qb = sylves_quaternion_from_euler(0, 0, M_PI);
    SylvesQuaternion qm = sylves_quaternion_slerp(qa, qb, 0.5);
    SylvesVector3 v = sylves_quaternion_rotate_vector(qm, vx);
    // halfway to 180deg around Z should rotate ~90deg
    assert(fabs(v.x) < 1e-6 && fabs(v.y - 1.0) < 1e-6);
    printf("  quaternion: PASSED\n");
}

static void test_aabb() {
    printf("Testing AABB...\n");
    SylvesVector3 pts[3] = {
        sylves_vector3_create(0,0,0),
        sylves_vector3_create(1,2,3),
        sylves_vector3_create(-1, 3, -2)
    };
    SylvesAabb a = sylves_aabb_create_from_points(pts, 3);
    assert(a.min.x == -1 && a.min.y == 0 && a.min.z == -2);
    assert(a.max.x == 1 && a.max.y == 3 && a.max.z == 3);
    assert(sylves_aabb_contains_point(a, sylves_vector3_create(0,1,0)));
    assert(!sylves_aabb_contains_point(a, sylves_vector3_create(2,0,0)));
    SylvesAabb b = sylves_aabb_expand(a, 1.0);
    assert(sylves_aabb_intersects(a,b));

    SylvesMatrix4x4 T = sylves_matrix4x4_translation(sylves_vector3_create(10,0,0));
    SylvesAabb at = sylves_aabb_transform(a, &T);
    assert(fabs(at.min.x - (a.min.x+10)) < EPS);
    printf("  aabb: PASSED\n");
}

static void test_trs() {
    printf("Testing TRS...\n");
    SylvesQuaternion rq = sylves_quaternion_from_axis_angle(sylves_vector3_unit_z(), M_PI/2);
    SylvesTRS trs = sylves_trs_create(sylves_vector3_create(1,0,0), rq, sylves_vector3_create(2,2,2));
    SylvesVector3 p = sylves_vector3_unit_x();
    SylvesVector3 tp = sylves_trs_transform_point(trs, p);
    // scale: (2,0,0), rotate 90deg around Z => (0,2,0), translate + (1,0,0) => (1,2,0)
    assert(fabs(tp.x-1) < EPS && fabs(tp.y-2) < EPS);

    SylvesTRS inv = sylves_trs_inverse(trs);
    SylvesVector3 back = sylves_trs_transform_point(inv, tp);
    assert(sylves_vector3_approx_equal(back, p, 1e-5));

    SylvesTRS id = sylves_trs_identity();
    SylvesVector3 same = sylves_trs_transform_point(id, p);
    assert(sylves_vector3_approx_equal(same, p, EPS));
    printf("  trs: PASSED\n");
}

static void test_memory() {
    printf("Testing memory...\n");
    int* arr = SYLVES_NEW_ARRAY(int, 4);
    assert(arr);
    for (int i=0;i<4;i++) arr[i] = i*i;
    int* arr2 = (int*)sylves_memdup(arr, sizeof(int)*4);
    assert(arr2);
    for (int i=0;i<4;i++) assert(arr2[i]==arr[i]);
    SYLVES_DELETE(arr);
    SYLVES_DELETE(arr2);
    printf("  memory: PASSED\n");
}

static void test_connection() {
    printf("Testing connection...\n");
    SylvesConnection id = sylves_connection_identity();
    assert(sylves_connection_is_identity(id));
    SylvesConnection r1 = sylves_connection_create(1, false);
    SylvesConnection r2 = sylves_connection_create(2, false);
    SylvesConnection c = sylves_connection_compose(r1, r2);
    assert(c.rotation == 3 && c.is_mirror == false);
    SylvesConnection inv = sylves_connection_invert(r1);
    SylvesConnection combined = sylves_connection_compose(r1, inv);
    assert(sylves_connection_is_identity(combined));
    printf("  connection: PASSED\n");
}

int main() {
    printf("Running core tests...\n");
    test_errors();
    test_vector_math();
    test_matrix_math();
    test_quaternion_math();
    test_aabb();
    test_trs();
    test_memory();
    test_connection();
    printf("All core tests passed.\n");
    return 0;
}

