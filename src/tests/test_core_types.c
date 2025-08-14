#include "test_framework.h"
#include "sylves/vector.h"
#include "sylves/matrix.h"
#include "sylves/cell.h"
#include "sylves/geometry.h"
#include <math.h>
#include <stdlib.h>

/* Core Type Tests */

static void test_vector_operations(void) {
    /* Test vector addition */
    SylvesVector3 v1 = {1.0, 2.0, 3.0};
    SylvesVector3 v2 = {4.0, -1.0, 6.0};
    SylvesVector3 result = sylves_vector_add(&v1, &v2);
    TEST_ASSERT(result.x == 5.0);
    TEST_ASSERT(result.y == 1.0);
    TEST_ASSERT(result.z == 9.0);

    /* Test cross product */
    result = sylves_vector_cross(&v1, &v2);
    TEST_ASSERT(result.x == 15.0);
    TEST_ASSERT(result.y == -6.0);
    TEST_ASSERT(result.z == -9.0);

    /* Test dot product */
    double dot = sylves_vector_dot(&v1, &v2);
    TEST_ASSERT(dot == 16.0);
}

static void test_matrix_operations(void) {
    /* Test matrix multiplication */
    SylvesMatrix4x4 m1 = sylves_matrix_identity();
    SylvesMatrix4x4 m2 = sylves_matrix_from_rotation_z(M_PI / 2);
    SylvesMatrix4x4 result = sylves_matrix_multiply(&m1, &m2);
    TEST_ASSERT(fabs(result.m[0][0]) < 1e-6);
    TEST_ASSERT(fabs(result.m[0][1] + 1.0) < 1e-6);
    TEST_ASSERT(fabs(result.m[1][0] - 1.0) < 1e-6);
    TEST_ASSERT(fabs(result.m[1][1]) < 1e-6);

    /* Test translation */
    SylvesVector3 translation = {2.0, 3.0, 4.0};
    m1 = sylves_matrix_from_translation(&translation);
    SylvesVector3 transformed = sylves_matrix_transform(&m1, &translation);
    TEST_ASSERT(transformed.x == 4.0);
    TEST_ASSERT(transformed.y == 6.0);
    TEST_ASSERT(transformed.z == 8.0);
}

static void test_cell_operations(void) {
    /* Basic cell addition */
    SylvesCell cell1 = {1, 2, 3};
    SylvesVector3 offset = {1.0, 1.0, 1.0};
    SylvesCell result = sylves_cell_offset(&cell1, &offset);
    TEST_ASSERT(result.x == 2);
    TEST_ASSERT(result.y == 3);
    TEST_ASSERT(result.z == 4);
}

static void test_geometric_primitives(void) {
    /* Test AABB creation and containment */
    SylvesAabb aabb = {
        .min = {0.0, 0.0, 0.0},
        .max = {1.0, 1.0, 1.0}
    };
    SylvesVector3 point = {0.5, 0.5, 0.5};
    bool contains = sylves_aabb_contains_point(&aabb, &point);
    TEST_ASSERT(contains);

    point.x = 1.5;
    contains = sylves_aabb_contains_point(&aabb, &point);
    TEST_ASSERT(!contains);
}

void test_core_types_main(void) {
    TEST_RUN(test_vector_operations);
    TEST_RUN(test_matrix_operations);
    TEST_RUN(test_cell_operations);
    TEST_RUN(test_geometric_primitives);
}
