/**
 * @file delaunay.c
 * @brief Delaunay triangulation implementation following Sylves
 */

#include "sylves/delaunay.h"
#include "sylves/memory.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <limits.h>

/* Constants matching Sylves */
static const double EPSILON = 2.220446049250313e-16; /* 2^-52 */
static const int EDGE_STACK_SIZE = 512;

/* Forward declarations */
static int hash_key(const SylvesDelaunay* d, double x, double y);
static double pseudo_angle(double dx, double dy);
static void quicksort(int* ids, const double* dists, int left, int right);
static void swap_int(int* arr, int i, int j);
static double dist(float ax, float ay, float bx, float by);
static int add_triangle(SylvesDelaunay* d, int i0, int i1, int i2, int a, int b, int c);
static void link_halfedge(SylvesDelaunay* d, int a, int b);
static int legalize(SylvesDelaunay* d, int a, int* edge_stack);

SylvesDelaunay* sylves_delaunay_create(
    const SylvesVector2* points,
    size_t num_points,
    SylvesError* error_out
) {
    if (!points || num_points < 3) {
        if (error_out) *error_out = SYLVES_ERROR_INVALID_ARGUMENT;
        return NULL;
    }

    SylvesDelaunay* d = calloc(1, sizeof(SylvesDelaunay));
    if (!d) {
        if (error_out) *error_out = SYLVES_ERROR_OUT_OF_MEMORY;
        return NULL;
    }

    d->points = points;
    d->num_points = num_points;

    /* Allocate flattened coordinate array */
    d->coords = malloc(num_points * 2 * sizeof(float));
    if (!d->coords) {
        sylves_delaunay_destroy(d);
        if (error_out) *error_out = SYLVES_ERROR_OUT_OF_MEMORY;
        return NULL;
    }

    /* Copy points to flattened array */
    for (size_t i = 0; i < num_points; i++) {
        d->coords[2 * i] = (float)points[i].x;
        d->coords[2 * i + 1] = (float)points[i].y;
    }

    /* Allocate triangulation arrays */
    size_t max_triangles = num_points > 2 ? 2 * num_points - 5 : 0;
    d->triangles_capacity = max_triangles * 3;
    d->triangles = malloc(d->triangles_capacity * sizeof(int));
    d->halfedges = malloc(d->triangles_capacity * sizeof(int));
    
    if (!d->triangles || !d->halfedges) {
        sylves_delaunay_destroy(d);
        if (error_out) *error_out = SYLVES_ERROR_OUT_OF_MEMORY;
        return NULL;
    }

    /* Initialize halfedges to -1 */
    memset(d->halfedges, -1, d->triangles_capacity * sizeof(int));

    /* Allocate hull arrays */
    d->hash_size = (int)ceil(sqrt((double)num_points));
    d->hull_prev = malloc(num_points * sizeof(int));
    d->hull_next = malloc(num_points * sizeof(int));
    d->hull_tri = malloc(num_points * sizeof(int));
    d->hull_hash = malloc(d->hash_size * sizeof(int));

    if (!d->hull_prev || !d->hull_next || !d->hull_tri || !d->hull_hash) {
        sylves_delaunay_destroy(d);
        if (error_out) *error_out = SYLVES_ERROR_OUT_OF_MEMORY;
        return NULL;
    }

    /* Initialize hull hash to -1 */
    memset(d->hull_hash, -1, d->hash_size * sizeof(int));

    /* Allocate working arrays */
    int* ids = malloc(num_points * sizeof(int));
    double* dists = malloc(num_points * sizeof(double));
    
    if (!ids || !dists) {
        free(ids);
        free(dists);
        sylves_delaunay_destroy(d);
        if (error_out) *error_out = SYLVES_ERROR_OUT_OF_MEMORY;
        return NULL;
    }

    /* Find bounding box */
    float min_x = FLT_MAX, min_y = FLT_MAX;
    float max_x = -FLT_MAX, max_y = -FLT_MAX;
    
    for (size_t i = 0; i < num_points; i++) {
        float x = d->coords[2 * i];
        float y = d->coords[2 * i + 1];
        if (x < min_x) min_x = x;
        if (y < min_y) min_y = y;
        if (x > max_x) max_x = x;
        if (y > max_y) max_y = y;
        ids[i] = (int)i;
    }

    float cx = (min_x + max_x) / 2;
    float cy = (min_y + max_y) / 2;

    /* Find seed point closest to center */
    double min_dist = DBL_MAX;
    int i0 = 0, i1 = 0, i2 = 0;

    for (size_t i = 0; i < num_points; i++) {
        double d_sq = dist(cx, cy, d->coords[2 * i], d->coords[2 * i + 1]);
        if (d_sq < min_dist) {
            i0 = (int)i;
            min_dist = d_sq;
        }
    }

    float i0x = d->coords[2 * i0];
    float i0y = d->coords[2 * i0 + 1];

    /* Find point closest to seed */
    min_dist = DBL_MAX;
    for (size_t i = 0; i < num_points; i++) {
        if ((int)i == i0) continue;
        double d_sq = dist(i0x, i0y, d->coords[2 * i], d->coords[2 * i + 1]);
        if (d_sq < min_dist && d_sq > 0) {
            i1 = (int)i;
            min_dist = d_sq;
        }
    }

    float i1x = d->coords[2 * i1];
    float i1y = d->coords[2 * i1 + 1];

    /* Find third point forming smallest circumcircle */
    double min_radius = DBL_MAX;
    for (size_t i = 0; i < num_points; i++) {
        if ((int)i == i0 || (int)i == i1) continue;
        double r = sylves_circumradius(i0x, i0y, i1x, i1y, 
                                      d->coords[2 * i], d->coords[2 * i + 1]);
        if (r < min_radius) {
            i2 = (int)i;
            min_radius = r;
        }
    }

    float i2x = d->coords[2 * i2];
    float i2y = d->coords[2 * i2 + 1];

    /* Handle collinear case */
    if (min_radius == DBL_MAX) {
        /* All points are collinear */
        for (size_t i = 0; i < num_points; i++) {
            dists[i] = (d->coords[2 * i] - d->coords[0]) != 0 ? 
                       (d->coords[2 * i] - d->coords[0]) :
                       (d->coords[2 * i + 1] - d->coords[1]);
        }
        quicksort(ids, dists, 0, (int)num_points - 1);
        
        /* Build hull */
        int* hull = malloc(num_points * sizeof(int));
        int j = 0;
        double d0 = -DBL_MAX;
        
        for (size_t i = 0; i < num_points; i++) {
            int id = ids[i];
            if (dists[id] > d0) {
                hull[j++] = id;
                d0 = dists[id];
            }
        }
        
        d->hull = realloc(hull, j * sizeof(int));
        d->hull_size = j;
        d->num_triangles = 0;
        
        free(ids);
        free(dists);
        
        if (error_out) *error_out = SYLVES_SUCCESS;
        return d;
    }

    /* Orient seed triangle */
    if (sylves_orient2d(i0x, i0y, i1x, i1y, i2x, i2y)) {
        int i = i1;
        float x = i1x, y = i1y;
        i1 = i2;
        i1x = i2x;
        i1y = i2y;
        i2 = i;
        i2x = x;
        i2y = y;
    }

    /* Compute circumcenter */
    sylves_circumcenter(i0x, i0y, i1x, i1y, i2x, i2y, &d->cx, &d->cy);

    /* Sort points by distance from circumcenter */
    for (size_t i = 0; i < num_points; i++) {
        dists[i] = dist(d->coords[2 * i], d->coords[2 * i + 1], d->cx, d->cy);
    }
    quicksort(ids, dists, 0, (int)num_points - 1);

    /* Initialize hull with seed triangle */
    d->hull_start = i0;
    d->hull_size = 3;

    d->hull_next[i0] = d->hull_prev[i2] = i1;
    d->hull_next[i1] = d->hull_prev[i0] = i2;
    d->hull_next[i2] = d->hull_prev[i1] = i0;

    d->hull_tri[i0] = 0;
    d->hull_tri[i1] = 1;
    d->hull_tri[i2] = 2;

    d->hull_hash[hash_key(d, i0x, i0y)] = i0;
    d->hull_hash[hash_key(d, i1x, i1y)] = i1;
    d->hull_hash[hash_key(d, i2x, i2y)] = i2;

    /* Add initial triangle */
    d->num_triangles = 0;
    add_triangle(d, i0, i1, i2, -1, -1, -1);

    /* Allocate edge stack for legalization */
    int* edge_stack = malloc(EDGE_STACK_SIZE * sizeof(int));
    if (!edge_stack) {
        free(ids);
        free(dists);
        sylves_delaunay_destroy(d);
        if (error_out) *error_out = SYLVES_ERROR_OUT_OF_MEMORY;
        return NULL;
    }

    /* Add remaining points */
    double xp = 0, yp = 0;
    
    for (size_t k = 0; k < num_points; k++) {
        int i = ids[k];
        double x = d->coords[2 * i];
        double y = d->coords[2 * i + 1];

        /* Skip duplicate points */
        if (k > 0 && fabs(x - xp) <= EPSILON && fabs(y - yp) <= EPSILON) continue;
        xp = x;
        yp = y;

        /* Skip seed triangle points */
        if (i == i0 || i == i1 || i == i2) continue;

        /* Find visible edge on convex hull */
        int start = 0;
        int key = hash_key(d, x, y);
        for (int j = 0; j < d->hash_size; j++) {
            start = d->hull_hash[(key + j) % d->hash_size];
            if (start != -1 && start != d->hull_next[start]) break;
        }

        start = d->hull_prev[start];
        int e = start;
        int q = d->hull_next[e];

        while (!sylves_orient2d(x, y, d->coords[2 * e], d->coords[2 * e + 1],
                               d->coords[2 * q], d->coords[2 * q + 1])) {
            e = q;
            if (e == start) {
                e = INT_MAX;
                break;
            }
            q = d->hull_next[e];
        }

        if (e == INT_MAX) continue; /* Skip near-duplicate point */

        /* Add first triangle from point */
        int t = add_triangle(d, e, i, d->hull_next[e], -1, -1, d->hull_tri[e]);

        /* Legalize new triangle */
        d->hull_tri[i] = legalize(d, t + 2, edge_stack);
        d->hull_tri[e] = t;
        d->hull_size++;

        /* Walk forward through hull, adding more triangles */
        int next = d->hull_next[e];
        q = d->hull_next[next];

        while (sylves_orient2d(x, y, d->coords[2 * next], d->coords[2 * next + 1],
                              d->coords[2 * q], d->coords[2 * q + 1])) {
            t = add_triangle(d, next, i, q, d->hull_tri[i], -1, d->hull_tri[next]);
            d->hull_tri[i] = legalize(d, t + 2, edge_stack);
            d->hull_next[next] = next; /* Mark as removed */
            d->hull_size--;
            next = q;
            q = d->hull_next[next];
        }

        /* Walk backward, adding more triangles */
        if (e == start) {
            q = d->hull_prev[e];

            while (sylves_orient2d(x, y, d->coords[2 * q], d->coords[2 * q + 1],
                                  d->coords[2 * e], d->coords[2 * e + 1])) {
                t = add_triangle(d, q, i, e, -1, d->hull_tri[e], d->hull_tri[q]);
                legalize(d, t + 2, edge_stack);
                d->hull_tri[q] = t;
                d->hull_next[e] = e; /* Mark as removed */
                d->hull_size--;
                e = q;
                q = d->hull_prev[e];
            }
        }

        /* Update hull indices */
        d->hull_start = d->hull_prev[i] = e;
        d->hull_next[e] = d->hull_prev[next] = i;
        d->hull_next[i] = next;

        /* Save new edges in hash */
        d->hull_hash[hash_key(d, x, y)] = i;
        d->hull_hash[hash_key(d, d->coords[2 * e], d->coords[2 * e + 1])] = e;
    }

    /* Build final hull array */
    d->hull = malloc(d->hull_size * sizeof(int));
    if (!d->hull) {
        free(ids);
        free(dists);
        free(edge_stack);
        sylves_delaunay_destroy(d);
        if (error_out) *error_out = SYLVES_ERROR_OUT_OF_MEMORY;
        return NULL;
    }

    int s = d->hull_start;
    for (size_t i = 0; i < d->hull_size; i++) {
        d->hull[i] = s;
        s = d->hull_next[s];
    }

    /* Clean up temporary arrays */
    free(d->hull_prev);
    free(d->hull_next);
    free(d->hull_tri);
    free(d->hull_hash);
    d->hull_prev = d->hull_next = d->hull_tri = d->hull_hash = NULL;

    /* Trim triangle arrays */
    d->triangles = realloc(d->triangles, d->num_triangles * 3 * sizeof(int));
    d->halfedges = realloc(d->halfedges, d->num_triangles * 3 * sizeof(int));

    free(ids);
    free(dists);
    free(edge_stack);

    if (error_out) *error_out = SYLVES_SUCCESS;
    return d;
}

void sylves_delaunay_destroy(SylvesDelaunay* delaunay) {
    if (!delaunay) return;

    free(delaunay->coords);
    free(delaunay->triangles);
    free(delaunay->halfedges);
    free(delaunay->hull);
    free(delaunay->hull_prev);
    free(delaunay->hull_next);
    free(delaunay->hull_tri);
    free(delaunay->hull_hash);
    free(delaunay);
}

bool sylves_delaunay_get_triangle(
    const SylvesDelaunay* delaunay,
    int triangle_index,
    SylvesDelaunayTriangle* triangle_out
) {
    if (!delaunay || !triangle_out || triangle_index < 0 || 
        triangle_index >= (int)delaunay->num_triangles) {
        return false;
    }

    int t = triangle_index * 3;
    triangle_out->index = triangle_index;
    triangle_out->p0 = delaunay->triangles[t];
    triangle_out->p1 = delaunay->triangles[t + 1];
    triangle_out->p2 = delaunay->triangles[t + 2];
    
    return true;
}

bool sylves_delaunay_get_triangles(
    const SylvesDelaunay* delaunay,
    SylvesDelaunayTriangle* triangles_out,
    size_t* num_triangles_out
) {
    if (!delaunay || !triangles_out || !num_triangles_out) {
        return false;
    }

    for (size_t i = 0; i < delaunay->num_triangles; i++) {
        sylves_delaunay_get_triangle(delaunay, (int)i, &triangles_out[i]);
    }

    *num_triangles_out = delaunay->num_triangles;
    return true;
}

bool sylves_delaunay_get_edge(
    const SylvesDelaunay* delaunay,
    int edge_index,
    SylvesVector2* p0_out,
    SylvesVector2* p1_out
) {
    if (!delaunay || !p0_out || !p1_out || edge_index < 0 ||
        edge_index >= (int)(delaunay->num_triangles * 3)) {
        return false;
    }

    int p0 = delaunay->triangles[edge_index];
    int p1 = delaunay->triangles[sylves_delaunay_next_halfedge(edge_index)];

    *p0_out = delaunay->points[p0];
    *p1_out = delaunay->points[p1];

    return true;
}

bool sylves_delaunay_get_triangle_circumcenter(
    const SylvesDelaunay* delaunay,
    int triangle_index,
    SylvesVector2* circumcenter_out
) {
    if (!delaunay || !circumcenter_out || triangle_index < 0 ||
        triangle_index >= (int)delaunay->num_triangles) {
        return false;
    }

    int t = triangle_index * 3;
    int p0 = delaunay->triangles[t];
    int p1 = delaunay->triangles[t + 1];
    int p2 = delaunay->triangles[t + 2];

    sylves_circumcenter(
        delaunay->coords[p0 * 2], delaunay->coords[p0 * 2 + 1],
        delaunay->coords[p1 * 2], delaunay->coords[p1 * 2 + 1],
        delaunay->coords[p2 * 2], delaunay->coords[p2 * 2 + 1],
        &circumcenter_out->x, &circumcenter_out->y
    );

    return true;
}

bool sylves_delaunay_points_around_triangle(
    const SylvesDelaunay* delaunay,
    int triangle_index,
    int* p0,
    int* p1,
    int* p2
) {
    if (!delaunay || !p0 || !p1 || !p2 || triangle_index < 0 ||
        triangle_index >= (int)delaunay->num_triangles) {
        return false;
    }

    int t = triangle_index * 3;
    *p0 = delaunay->triangles[t];
    *p1 = delaunay->triangles[t + 1];
    *p2 = delaunay->triangles[t + 2];

    return true;
}

/* Geometric predicates */

bool sylves_orient2d(
    double px, double py,
    double qx, double qy,
    double rx, double ry
) {
    return (qy - py) * (rx - qx) - (qx - px) * (ry - qy) < 0;
}

bool sylves_incircle(
    double ax, double ay,
    double bx, double by,
    double cx, double cy,
    double px, double py
) {
    double dx = ax - px;
    double dy = ay - py;
    double ex = bx - px;
    double ey = by - py;
    double fx = cx - px;
    double fy = cy - py;

    double ap = dx * dx + dy * dy;
    double bp = ex * ex + ey * ey;
    double cp = fx * fx + fy * fy;

    return dx * (ey * cp - bp * fy) -
           dy * (ex * cp - bp * fx) +
           ap * (ex * fy - ey * fx) < 0;
}

double sylves_circumradius(
    double ax, double ay,
    double bx, double by,
    double cx, double cy
) {
    double dx = bx - ax;
    double dy = by - ay;
    double ex = cx - ax;
    double ey = cy - ay;
    double bl = dx * dx + dy * dy;
    double cl = ex * ex + ey * ey;
    double d = 0.5 / (dx * ey - dy * ex);
    double x = (ey * bl - dy * cl) * d;
    double y = (dx * cl - ex * bl) * d;
    return x * x + y * y;
}

/* Internal helper functions */

static int hash_key(const SylvesDelaunay* d, double x, double y) {
    double angle = pseudo_angle(x - d->cx, y - d->cy);
    return (int)(floor(angle * d->hash_size) + 0.5) % d->hash_size;
}

static double pseudo_angle(double dx, double dy) {
    double p = dx / (fabs(dx) + fabs(dy));
    return (dy > 0 ? 3 - p : 1 + p) / 4; /* [0..1] */
}

static void quicksort(int* ids, const double* dists, int left, int right) {
    if (right - left <= 20) {
        /* Insertion sort for small arrays */
        for (int i = left + 1; i <= right; i++) {
            int temp = ids[i];
            double temp_dist = dists[temp];
            int j = i - 1;
            while (j >= left && dists[ids[j]] > temp_dist) {
                ids[j + 1] = ids[j];
                j--;
            }
            ids[j + 1] = temp;
        }
    } else {
        int median = (left + right) >> 1;
        int i = left + 1;
        int j = right;
        
        swap_int(ids, median, i);
        if (dists[ids[left]] > dists[ids[right]]) swap_int(ids, left, right);
        if (dists[ids[i]] > dists[ids[right]]) swap_int(ids, i, right);
        if (dists[ids[left]] > dists[ids[i]]) swap_int(ids, left, i);

        int temp = ids[i];
        double temp_dist = dists[temp];
        
        while (1) {
            do i++; while (dists[ids[i]] < temp_dist);
            do j--; while (dists[ids[j]] > temp_dist);
            if (j < i) break;
            swap_int(ids, i, j);
        }
        
        ids[left + 1] = ids[j];
        ids[j] = temp;

        if (right - i + 1 >= j - left) {
            quicksort(ids, dists, i, right);
            quicksort(ids, dists, left, j - 1);
        } else {
            quicksort(ids, dists, left, j - 1);
            quicksort(ids, dists, i, right);
        }
    }
}

static void swap_int(int* arr, int i, int j) {
    int tmp = arr[i];
    arr[i] = arr[j];
    arr[j] = tmp;
}

static double dist(float ax, float ay, float bx, float by) {
    double dx = ax - bx;
    double dy = ay - by;
    return dx * dx + dy * dy;
}

static int add_triangle(SylvesDelaunay* d, int i0, int i1, int i2, 
                       int a, int b, int c) {
    int t = (int)(d->num_triangles * 3);

    d->triangles[t] = i0;
    d->triangles[t + 1] = i1;
    d->triangles[t + 2] = i2;

    link_halfedge(d, t, a);
    link_halfedge(d, t + 1, b);
    link_halfedge(d, t + 2, c);

    d->num_triangles++;
    return t;
}

static void link_halfedge(SylvesDelaunay* d, int a, int b) {
    d->halfedges[a] = b;
    if (b != -1) d->halfedges[b] = a;
}

static int legalize(SylvesDelaunay* d, int a, int* edge_stack) {
    int i = 0;
    int ar;

    /* Process edges using stack instead of recursion */
    while (1) {
        int b = d->halfedges[a];

        /* Check if edge is on convex hull */
        int a0 = a - a % 3;
        ar = a0 + (a + 2) % 3;

        if (b == -1) {
            if (i == 0) break;
            a = edge_stack[--i];
            continue;
        }

        int b0 = b - b % 3;
        int al = a0 + (a + 1) % 3;
        int bl = b0 + (b + 2) % 3;

        int p0 = d->triangles[ar];
        int pr = d->triangles[a];
        int pl = d->triangles[al];
        int p1 = d->triangles[bl];

        bool illegal = sylves_incircle(
            d->coords[2 * p0], d->coords[2 * p0 + 1],
            d->coords[2 * pr], d->coords[2 * pr + 1],
            d->coords[2 * pl], d->coords[2 * pl + 1],
            d->coords[2 * p1], d->coords[2 * p1 + 1]
        );

        if (illegal) {
            d->triangles[a] = p1;
            d->triangles[b] = p0;

            int hbl = d->halfedges[bl];

            /* Handle edge on hull */
            if (hbl == -1) {
                int e = d->hull_start;
                do {
                    if (d->hull_tri[e] == bl) {
                        d->hull_tri[e] = a;
                        break;
                    }
                    e = d->hull_prev[e];
                } while (e != d->hull_start);
            }

            link_halfedge(d, a, hbl);
            link_halfedge(d, b, d->halfedges[ar]);
            link_halfedge(d, ar, bl);

            int br = b0 + (b + 1) % 3;

            /* Push edge onto stack */
            if (i < EDGE_STACK_SIZE) {
                edge_stack[i++] = br;
            }
        } else {
            if (i == 0) break;
            a = edge_stack[--i];
        }
    }

    return ar;
}
