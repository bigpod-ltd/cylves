/**
 * @file bounds.c
 * @brief Basic bound implementations (rectangle and cube) with vtables
 */

#include "sylves/bounds.h"
#include "internal/bound_internal.h"
#include <stdlib.h>

/* Forward declaration for hex bound vtable used before its definition */
static const SylvesBoundVTable HEXB_VT;

/* Rectangle bound (2D grid region) */
typedef struct {
    int min_x, min_y;
    int max_x, max_y;
} RectBoundData;

static bool rect_contains(const SylvesBound* b, SylvesCell c) {
    const RectBoundData* d = (const RectBoundData*)b->data;
    if (c.z != 0) return false;
    return c.x >= d->min_x && c.x <= d->max_x && c.y >= d->min_y && c.y <= d->max_y;
}

static void rect_destroy(SylvesBound* b) {
    if (!b) return;
    free(b->data);
    free(b);
}

static const char* rect_name(const SylvesBound* b) { (void)b; return "rectangle"; }

static int rect_get_cells(const SylvesBound* b, SylvesCell* cells, size_t max_cells) {
    const RectBoundData* d = (const RectBoundData*)b->data;
    size_t count = 0;
    for (int y = d->min_y; y <= d->max_y && count < max_cells; y++) {
        for (int x = d->min_x; x <= d->max_x && count < max_cells; x++) {
            if (cells) cells[count] = (SylvesCell){x,y,0};
            count++;
        }
    }
    return (int)count;
}
static int rect_get_rect(const SylvesBound* b, int* min_x, int* min_y, int* max_x, int* max_y) {
    if (!b || !b->data) return -1;
    const RectBoundData* d = (const RectBoundData*)b->data;
    if (min_x) *min_x = d->min_x; if (min_y) *min_y = d->min_y;
    if (max_x) *max_x = d->max_x; if (max_y) *max_y = d->max_y;
    return 0;
}
static int rect_get_cube(const SylvesBound* b, int* min_x, int* min_y, int* min_z,
                         int* max_x, int* max_y, int* max_z) {
    (void)b; (void)min_x; (void)min_y; (void)min_z; (void)max_x; (void)max_y; (void)max_z; return -1;
}

/* Rectangle bound operations */
static SylvesBound* rect_intersect(const SylvesBound* a, const SylvesBound* b) {
    /* Both must be rectangles for simple implementation */
    if (a->type != SYLVES_BOUND_TYPE_RECT || b->type != SYLVES_BOUND_TYPE_RECT) {
        return NULL;
    }
    
    int a_minx, a_miny, a_maxx, a_maxy;
    int b_minx, b_miny, b_maxx, b_maxy;
    rect_get_rect(a, &a_minx, &a_miny, &a_maxx, &a_maxy);
    rect_get_rect(b, &b_minx, &b_miny, &b_maxx, &b_maxy);
    
    int minx = a_minx > b_minx ? a_minx : b_minx;
    int miny = a_miny > b_miny ? a_miny : b_miny;
    int maxx = a_maxx < b_maxx ? a_maxx : b_maxx;
    int maxy = a_maxy < b_maxy ? a_maxy : b_maxy;
    
    if (minx > maxx || miny > maxy) {
        /* Empty intersection */
        return sylves_bound_create_rectangle(1, 1, 0, 0);
    }
    
    return sylves_bound_create_rectangle(minx, miny, maxx, maxy);
}

static SylvesBound* rect_union(const SylvesBound* a, const SylvesBound* b) {
    if (a->type != SYLVES_BOUND_TYPE_RECT || b->type != SYLVES_BOUND_TYPE_RECT) {
        return NULL;
    }
    
    int a_minx, a_miny, a_maxx, a_maxy;
    int b_minx, b_miny, b_maxx, b_maxy;
    rect_get_rect(a, &a_minx, &a_miny, &a_maxx, &a_maxy);
    rect_get_rect(b, &b_minx, &b_miny, &b_maxx, &b_maxy);
    
    int minx = a_minx < b_minx ? a_minx : b_minx;
    int miny = a_miny < b_miny ? a_miny : b_miny;
    int maxx = a_maxx > b_maxx ? a_maxx : b_maxx;
    int maxy = a_maxy > b_maxy ? a_maxy : b_maxy;
    
    return sylves_bound_create_rectangle(minx, miny, maxx, maxy);
}

static int rect_get_cell_count(const SylvesBound* b) {
    const RectBoundData* d = (const RectBoundData*)b->data;
    int width = d->max_x - d->min_x + 1;
    int height = d->max_y - d->min_y + 1;
    if (width <= 0 || height <= 0) return 0;
    return width * height;
}

static SylvesBound* rect_clone(const SylvesBound* b) {
    const RectBoundData* d = (const RectBoundData*)b->data;
    return sylves_bound_create_rectangle(d->min_x, d->min_y, d->max_x, d->max_y);
}

static bool rect_is_empty(const SylvesBound* b) {
    const RectBoundData* d = (const RectBoundData*)b->data;
    return d->min_x > d->max_x || d->min_y > d->max_y;
}

static int rect_get_aabb(const SylvesBound* b, float* min, float* max) {
    const RectBoundData* d = (const RectBoundData*)b->data;
    if (min) {
        min[0] = (float)d->min_x;
        min[1] = (float)d->min_y;
        min[2] = 0.0f;
    }
    if (max) {
        max[0] = (float)(d->max_x + 1);
        max[1] = (float)(d->max_y + 1);
        max[2] = 1.0f;
    }
    return 0;
}

static const SylvesBoundVTable RECT_VT = {
    .contains = rect_contains,
    .destroy = rect_destroy,
    .name = rect_name,
    .get_cells = rect_get_cells,
    .get_rect = rect_get_rect,
    .get_cube = rect_get_cube,
    .intersect = rect_intersect,
    .union_bounds = rect_union,
    .get_cell_count = rect_get_cell_count,
    .clone = rect_clone,
    .is_empty = rect_is_empty,
    .get_aabb = rect_get_aabb
};

/* Cube bound (3D grid region) */
typedef struct {
    int min_x, min_y, min_z;
    int max_x, max_y, max_z;
} CubeBoundData;

static bool cube_contains(const SylvesBound* b, SylvesCell c) {
    const CubeBoundData* d = (const CubeBoundData*)b->data;
    return c.x >= d->min_x && c.x <= d->max_x &&
           c.y >= d->min_y && c.y <= d->max_y &&
           c.z >= d->min_z && c.z <= d->max_z;
}

static void cube_destroy(SylvesBound* b) { rect_destroy(b); }
static const char* cube_name(const SylvesBound* b) { (void)b; return "cube"; }

static int cube_get_cells(const SylvesBound* b, SylvesCell* cells, size_t max_cells) {
    const CubeBoundData* d = (const CubeBoundData*)b->data;
    size_t count = 0;
    for (int z = d->min_z; z <= d->max_z && count < max_cells; z++) {
        for (int y = d->min_y; y <= d->max_y && count < max_cells; y++) {
            for (int x = d->min_x; x <= d->max_x && count < max_cells; x++) {
                if (cells) cells[count] = (SylvesCell){x,y,z};
                count++;
            }
        }
    }
    return (int)count;
}
static int cube_get_rect(const SylvesBound* b, int* min_x, int* min_y, int* max_x, int* max_y) {
    (void)b; (void)min_x; (void)min_y; (void)max_x; (void)max_y; return -1;
}
static int cube_get_cube(const SylvesBound* b, int* min_x, int* min_y, int* min_z,
                         int* max_x, int* max_y, int* max_z) {
    if (!b || !b->data) return -1;
    const CubeBoundData* d = (const CubeBoundData*)b->data;
    if (min_x) *min_x = d->min_x; if (min_y) *min_y = d->min_y; if (min_z) *min_z = d->min_z;
    if (max_x) *max_x = d->max_x; if (max_y) *max_y = d->max_y; if (max_z) *max_z = d->max_z;
    return 0;
}

/* Cube bound operations */
static SylvesBound* cube_intersect(const SylvesBound* a, const SylvesBound* b) {
    if (a->type != SYLVES_BOUND_TYPE_CUBE || b->type != SYLVES_BOUND_TYPE_CUBE) {
        return NULL;
    }
    
    int a_minx, a_miny, a_minz, a_maxx, a_maxy, a_maxz;
    int b_minx, b_miny, b_minz, b_maxx, b_maxy, b_maxz;
    cube_get_cube(a, &a_minx, &a_miny, &a_minz, &a_maxx, &a_maxy, &a_maxz);
    cube_get_cube(b, &b_minx, &b_miny, &b_minz, &b_maxx, &b_maxy, &b_maxz);
    
    int minx = a_minx > b_minx ? a_minx : b_minx;
    int miny = a_miny > b_miny ? a_miny : b_miny;
    int minz = a_minz > b_minz ? a_minz : b_minz;
    int maxx = a_maxx < b_maxx ? a_maxx : b_maxx;
    int maxy = a_maxy < b_maxy ? a_maxy : b_maxy;
    int maxz = a_maxz < b_maxz ? a_maxz : b_maxz;
    
    if (minx > maxx || miny > maxy || minz > maxz) {
        /* Empty intersection */
        return sylves_bound_create_cube(1, 1, 1, 0, 0, 0);
    }
    
    return sylves_bound_create_cube(minx, miny, minz, maxx, maxy, maxz);
}

static SylvesBound* cube_union(const SylvesBound* a, const SylvesBound* b) {
    if (a->type != SYLVES_BOUND_TYPE_CUBE || b->type != SYLVES_BOUND_TYPE_CUBE) {
        return NULL;
    }
    
    int a_minx, a_miny, a_minz, a_maxx, a_maxy, a_maxz;
    int b_minx, b_miny, b_minz, b_maxx, b_maxy, b_maxz;
    cube_get_cube(a, &a_minx, &a_miny, &a_minz, &a_maxx, &a_maxy, &a_maxz);
    cube_get_cube(b, &b_minx, &b_miny, &b_minz, &b_maxx, &b_maxy, &b_maxz);
    
    int minx = a_minx < b_minx ? a_minx : b_minx;
    int miny = a_miny < b_miny ? a_miny : b_miny;
    int minz = a_minz < b_minz ? a_minz : b_minz;
    int maxx = a_maxx > b_maxx ? a_maxx : b_maxx;
    int maxy = a_maxy > b_maxy ? a_maxy : b_maxy;
    int maxz = a_maxz > b_maxz ? a_maxz : b_maxz;
    
    return sylves_bound_create_cube(minx, miny, minz, maxx, maxy, maxz);
}

static int cube_get_cell_count(const SylvesBound* b) {
    const CubeBoundData* d = (const CubeBoundData*)b->data;
    int width = d->max_x - d->min_x + 1;
    int height = d->max_y - d->min_y + 1;
    int depth = d->max_z - d->min_z + 1;
    if (width <= 0 || height <= 0 || depth <= 0) return 0;
    return width * height * depth;
}

static SylvesBound* cube_clone(const SylvesBound* b) {
    const CubeBoundData* d = (const CubeBoundData*)b->data;
    return sylves_bound_create_cube(d->min_x, d->min_y, d->min_z, d->max_x, d->max_y, d->max_z);
}

static bool cube_is_empty(const SylvesBound* b) {
    const CubeBoundData* d = (const CubeBoundData*)b->data;
    return d->min_x > d->max_x || d->min_y > d->max_y || d->min_z > d->max_z;
}

static int cube_get_aabb(const SylvesBound* b, float* min, float* max) {
    const CubeBoundData* d = (const CubeBoundData*)b->data;
    if (min) {
        min[0] = (float)d->min_x;
        min[1] = (float)d->min_y;
        min[2] = (float)d->min_z;
    }
    if (max) {
        max[0] = (float)(d->max_x + 1);
        max[1] = (float)(d->max_y + 1);
        max[2] = (float)(d->max_z + 1);
    }
    return 0;
}

static const SylvesBoundVTable CUBE_VT = {
    .contains = cube_contains,
    .destroy = cube_destroy,
    .name = cube_name,
    .get_cells = cube_get_cells,
    .get_rect = cube_get_rect,
    .get_cube = cube_get_cube,
    .intersect = cube_intersect,
    .union_bounds = cube_union,
    .get_cell_count = cube_get_cell_count,
    .clone = cube_clone,
    .is_empty = cube_is_empty,
    .get_aabb = cube_get_aabb
};

/* Hex cube bound with Min/Mex (exclusive upper bound) */
typedef struct {
    int min_x, min_y, min_z;
    int mex_x, mex_y, mex_z;
} HexBoundData;

static bool hexb_contains(const SylvesBound* b, SylvesCell c) {
    const HexBoundData* d = (const HexBoundData*)b->data;
    /* Accept both axial (z=0) and full cube triples where x+y+z=0 */
    int x = c.x;
    int z = (c.z == 0) ? c.y : c.z;
    int y = (c.z == 0) ? -x - z : c.y;
    return x >= d->min_x && y >= d->min_y && z >= d->min_z &&
           x <  d->mex_x && y <  d->mex_y && z <  d->mex_z;
}
static void hexb_destroy(SylvesBound* b) { rect_destroy(b); }
static const char* hexb_name(const SylvesBound* b) { (void)b; return "hex_parallelogram"; }
static int hexb_get_cells(const SylvesBound* b, SylvesCell* cells, size_t max_cells) {
    const HexBoundData* d = (const HexBoundData*)b->data;
    size_t count = 0;
    for (int x = d->min_x; x < d->mex_x && count < max_cells; x++) {
        for (int y = d->min_y; y < d->mex_y && count < max_cells; y++) {
            int z = -x - y;
            if (z < d->min_z || z >= d->mex_z) continue;
            if (cells) cells[count] = (SylvesCell){x, y, z};
            count++;
        }
    }
    return (int)count;
}
static int hexb_get_rect(const SylvesBound* b, int* min_x, int* min_y, int* max_x, int* max_y) {
    const HexBoundData* d = (const HexBoundData*)b->data;
    /* Project cube Min/Mex to axial q=x, r=z, return inclusive max */
    if (min_x) *min_x = d->min_x; if (min_y) *min_y = d->min_z;
    if (max_x) *max_x = d->mex_x - 1; if (max_y) *max_y = d->mex_z - 1;
    return 0;
}
static int hexb_get_cube(const SylvesBound* b, int* min_x, int* min_y, int* min_z,
                         int* max_x, int* max_y, int* max_z) {
    const HexBoundData* d = (const HexBoundData*)b->data;
    if (min_x) *min_x = d->min_x; if (min_y) *min_y = d->min_y; if (min_z) *min_z = d->min_z;
    if (max_x) *max_x = d->mex_x - 1; if (max_y) *max_y = d->mex_y - 1; if (max_z) *max_z = d->mex_z - 1;
    return 0;
}

/* Hex bound operations */
static SylvesBound* hexb_intersect(const SylvesBound* a, const SylvesBound* b) {
    if (a->type != SYLVES_BOUND_TYPE_HEX || b->type != SYLVES_BOUND_TYPE_HEX) {
        return NULL;
    }
    
    const HexBoundData* data_a = (const HexBoundData*)a->data;
    const HexBoundData* data_b = (const HexBoundData*)b->data;
    
    int minx = data_a->min_x > data_b->min_x ? data_a->min_x : data_b->min_x;
    int miny = data_a->min_y > data_b->min_y ? data_a->min_y : data_b->min_y;
    int minz = data_a->min_z > data_b->min_z ? data_a->min_z : data_b->min_z;
    int mexx = data_a->mex_x < data_b->mex_x ? data_a->mex_x : data_b->mex_x;
    int mexy = data_a->mex_y < data_b->mex_y ? data_a->mex_y : data_b->mex_y;
    int mexz = data_a->mex_z < data_b->mex_z ? data_a->mex_z : data_b->mex_z;
    
    if (minx >= mexx || miny >= mexy || minz >= mexz) {
        /* Empty intersection */
        return sylves_bound_create_hex_parallelogram(1, 1, 0, 0);
    }
    
    /* Create new hex bound with intersected Min/Mex */
    SylvesBound* result = (SylvesBound*)calloc(1, sizeof(SylvesBound));
    if (!result) return NULL;
    
    HexBoundData* d = (HexBoundData*)calloc(1, sizeof(HexBoundData));
    if (!d) { free(result); return NULL; }
    
    d->min_x = minx; d->min_y = miny; d->min_z = minz;
    d->mex_x = mexx; d->mex_y = mexy; d->mex_z = mexz;
    
    result->vtable = &HEXB_VT;
    result->data = d;
    result->type = SYLVES_BOUND_TYPE_HEX;
    return result;
}

static SylvesBound* hexb_union(const SylvesBound* a, const SylvesBound* b) {
    if (a->type != SYLVES_BOUND_TYPE_HEX || b->type != SYLVES_BOUND_TYPE_HEX) {
        return NULL;
    }
    
    const HexBoundData* data_a = (const HexBoundData*)a->data;
    const HexBoundData* data_b = (const HexBoundData*)b->data;
    
    int minx = data_a->min_x < data_b->min_x ? data_a->min_x : data_b->min_x;
    int miny = data_a->min_y < data_b->min_y ? data_a->min_y : data_b->min_y;
    int minz = data_a->min_z < data_b->min_z ? data_a->min_z : data_b->min_z;
    int mexx = data_a->mex_x > data_b->mex_x ? data_a->mex_x : data_b->mex_x;
    int mexy = data_a->mex_y > data_b->mex_y ? data_a->mex_y : data_b->mex_y;
    int mexz = data_a->mex_z > data_b->mex_z ? data_a->mex_z : data_b->mex_z;
    
    /* Create new hex bound with union Min/Mex */
    SylvesBound* result = (SylvesBound*)calloc(1, sizeof(SylvesBound));
    if (!result) return NULL;
    
    HexBoundData* d = (HexBoundData*)calloc(1, sizeof(HexBoundData));
    if (!d) { free(result); return NULL; }
    
    d->min_x = minx; d->min_y = miny; d->min_z = minz;
    d->mex_x = mexx; d->mex_y = mexy; d->mex_z = mexz;
    
    result->vtable = &HEXB_VT;
    result->data = d;
    result->type = SYLVES_BOUND_TYPE_HEX;
    return result;
}

static int hexb_get_cell_count(const SylvesBound* b) {
    const HexBoundData* d = (const HexBoundData*)b->data;
    /* Count cells where x+y+z=0 within bounds */
    int count = 0;
    for (int x = d->min_x; x < d->mex_x; x++) {
        for (int y = d->min_y; y < d->mex_y; y++) {
            int z = -x - y;
            if (z >= d->min_z && z < d->mex_z) {
                count++;
            }
        }
    }
    return count;
}

static SylvesBound* hexb_clone(const SylvesBound* b) {
    const HexBoundData* d = (const HexBoundData*)b->data;
    /* Clone with same Min/Mex */
    SylvesBound* result = (SylvesBound*)calloc(1, sizeof(SylvesBound));
    if (!result) return NULL;
    
    HexBoundData* new_d = (HexBoundData*)calloc(1, sizeof(HexBoundData));
    if (!new_d) { free(result); return NULL; }
    
    *new_d = *d;
    result->vtable = &HEXB_VT;
    result->data = new_d;
    result->type = SYLVES_BOUND_TYPE_HEX;
    return result;
}

static bool hexb_is_empty(const SylvesBound* b) {
    const HexBoundData* d = (const HexBoundData*)b->data;
    return d->min_x >= d->mex_x || d->min_y >= d->mex_y || d->min_z >= d->mex_z;
}

static int hexb_get_aabb(const SylvesBound* b, float* min, float* max) {
    /* Hex bounds don't map directly to AABB - return approximate bounds */
    int min_x, min_y, max_x, max_y;
    hexb_get_rect(b, &min_x, &min_y, &max_x, &max_y);
    
    if (min) {
        min[0] = (float)min_x * 0.866025f; /* sqrt(3)/2 */
        min[1] = (float)min_y * 0.75f;
        min[2] = 0.0f;
    }
    if (max) {
        max[0] = (float)(max_x + 1) * 0.866025f;
        max[1] = (float)(max_y + 1) * 0.75f;
        max[2] = 1.0f;
    }
    return 0;
}

static const SylvesBoundVTable HEXB_VT = {
    .contains = hexb_contains,
    .destroy = hexb_destroy,
    .name = hexb_name,
    .get_cells = hexb_get_cells,
    .get_rect = hexb_get_rect,
    .get_cube = hexb_get_cube,
    .intersect = hexb_intersect,
    .union_bounds = hexb_union,
    .get_cell_count = hexb_get_cell_count,
    .clone = hexb_clone,
    .is_empty = hexb_is_empty,
    .get_aabb = hexb_get_aabb
};

/* Public API */
SylvesBound* sylves_bound_create_rectangle(int min_x, int min_y, int max_x, int max_y) {
    SylvesBound* b = (SylvesBound*)calloc(1, sizeof(SylvesBound));
    if (!b) return NULL;
    RectBoundData* d = (RectBoundData*)calloc(1, sizeof(RectBoundData));
    if (!d) { free(b); return NULL; }
    d->min_x = min_x; d->min_y = min_y; d->max_x = max_x; d->max_y = max_y;
    b->vtable = &RECT_VT; b->data = d; b->type = SYLVES_BOUND_TYPE_RECT;
    return b;
}

SylvesBound* sylves_bound_create_cube(int min_x, int min_y, int min_z,
                                      int max_x, int max_y, int max_z) {
    SylvesBound* b = (SylvesBound*)calloc(1, sizeof(SylvesBound));
    if (!b) return NULL;
    CubeBoundData* d = (CubeBoundData*)calloc(1, sizeof(CubeBoundData));
    if (!d) { free(b); return NULL; }
    d->min_x = min_x; d->min_y = min_y; d->min_z = min_z;
    d->max_x = max_x; d->max_y = max_y; d->max_z = max_z;
    b->vtable = &CUBE_VT; b->data = d; b->type = SYLVES_BOUND_TYPE_CUBE;
    return b;
}

SylvesBound* sylves_bound_create_hex_parallelogram(int min_q, int min_r, int max_q, int max_r) {
    SylvesBound* b = (SylvesBound*)calloc(1, sizeof(SylvesBound));
    if (!b) return NULL;
    HexBoundData* d = (HexBoundData*)calloc(1, sizeof(HexBoundData));
    if (!d) { free(b); return NULL; }
    /* Convert axial inclusive [min..max] to cube Min/Mex (exclusive upper bound) */
    d->min_x = min_q; d->min_z = min_r; d->min_y = -d->min_x - d->min_z;
    d->mex_x = max_q + 1; d->mex_z = max_r + 1; d->mex_y = -d->mex_x - d->mex_z;
    b->vtable = &HEXB_VT; b->data = d; b->type = SYLVES_BOUND_TYPE_HEX;
    return b;
}

/* Triangle bound */
typedef struct {
    int min_x, min_y, min_z;
    int max_x, max_y, max_z;
} TriangleBoundData;

static bool triangle_bound_contains(const SylvesBound* b, SylvesCell c) {
    const TriangleBoundData* d = (const TriangleBoundData*)b->data;
    /* Cell must be within bounds and satisfy triangle constraint (x+y+z == 1 or 2) */
    int sum = c.x + c.y + c.z;
    if (sum != 1 && sum != 2) return false;
    return c.x >= d->min_x && c.x <= d->max_x &&
           c.y >= d->min_y && c.y <= d->max_y &&
           c.z >= d->min_z && c.z <= d->max_z;
}

static void triangle_bound_destroy(SylvesBound* b) {
    if (!b) return;
    free(b->data);
    free(b);
}

static const char* triangle_bound_name(const SylvesBound* b) {
    (void)b;
    return "triangle_parallelogram";
}

static int triangle_bound_get_cells(const SylvesBound* b, SylvesCell* cells, size_t max_cells) {
    const TriangleBoundData* d = (const TriangleBoundData*)b->data;
    size_t count = 0;
    
    /* Enumerate all cells in the bounding box that satisfy triangle constraint */
    for (int x = d->min_x; x <= d->max_x && count < max_cells; x++) {
        for (int y = d->min_y; y <= d->max_y && count < max_cells; y++) {
            for (int z = d->min_z; z <= d->max_z && count < max_cells; z++) {
                int sum = x + y + z;
                if (sum == 1 || sum == 2) {
                    if (cells) cells[count] = (SylvesCell){x, y, z};
                    count++;
                }
            }
        }
    }
    return (int)count;
}

static int triangle_bound_get_rect(const SylvesBound* b, int* min_x, int* min_y, int* max_x, int* max_y) {
    /* Triangle bounds don't map cleanly to 2D rectangles */
    (void)b; (void)min_x; (void)min_y; (void)max_x; (void)max_y;
    return -1;
}

static int triangle_bound_get_cube(const SylvesBound* b, int* min_x, int* min_y, int* min_z,
                                   int* max_x, int* max_y, int* max_z) {
    if (!b || !b->data) return -1;
    const TriangleBoundData* d = (const TriangleBoundData*)b->data;
    if (min_x) *min_x = d->min_x; if (min_y) *min_y = d->min_y; if (min_z) *min_z = d->min_z;
    if (max_x) *max_x = d->max_x; if (max_y) *max_y = d->max_y; if (max_z) *max_z = d->max_z;
    return 0;
}

/* Triangle bound operations */
static SylvesBound* triangle_intersect(const SylvesBound* a, const SylvesBound* b) {
    if (a->type != SYLVES_BOUND_TYPE_TRIANGLE || b->type != SYLVES_BOUND_TYPE_TRIANGLE) {
        return NULL;
    }
    
    int a_minx, a_miny, a_minz, a_maxx, a_maxy, a_maxz;
    int b_minx, b_miny, b_minz, b_maxx, b_maxy, b_maxz;
    triangle_bound_get_cube(a, &a_minx, &a_miny, &a_minz, &a_maxx, &a_maxy, &a_maxz);
    triangle_bound_get_cube(b, &b_minx, &b_miny, &b_minz, &b_maxx, &b_maxy, &b_maxz);
    
    int minx = a_minx > b_minx ? a_minx : b_minx;
    int miny = a_miny > b_miny ? a_miny : b_miny;
    int minz = a_minz > b_minz ? a_minz : b_minz;
    int maxx = a_maxx < b_maxx ? a_maxx : b_maxx;
    int maxy = a_maxy < b_maxy ? a_maxy : b_maxy;
    int maxz = a_maxz < b_maxz ? a_maxz : b_maxz;
    
    if (minx > maxx || miny > maxy || minz > maxz) {
        /* Empty intersection */
        return sylves_bound_create_triangle_parallelogram(1, 1, 1, 0, 0, 0);
    }
    
    return sylves_bound_create_triangle_parallelogram(minx, miny, minz, maxx, maxy, maxz);
}

static SylvesBound* triangle_union(const SylvesBound* a, const SylvesBound* b) {
    if (a->type != SYLVES_BOUND_TYPE_TRIANGLE || b->type != SYLVES_BOUND_TYPE_TRIANGLE) {
        return NULL;
    }
    
    int a_minx, a_miny, a_minz, a_maxx, a_maxy, a_maxz;
    int b_minx, b_miny, b_minz, b_maxx, b_maxy, b_maxz;
    triangle_bound_get_cube(a, &a_minx, &a_miny, &a_minz, &a_maxx, &a_maxy, &a_maxz);
    triangle_bound_get_cube(b, &b_minx, &b_miny, &b_minz, &b_maxx, &b_maxy, &b_maxz);
    
    int minx = a_minx < b_minx ? a_minx : b_minx;
    int miny = a_miny < b_miny ? a_miny : b_miny;
    int minz = a_minz < b_minz ? a_minz : b_minz;
    int maxx = a_maxx > b_maxx ? a_maxx : b_maxx;
    int maxy = a_maxy > b_maxy ? a_maxy : b_maxy;
    int maxz = a_maxz > b_maxz ? a_maxz : b_maxz;
    
    return sylves_bound_create_triangle_parallelogram(minx, miny, minz, maxx, maxy, maxz);
}

static int triangle_get_cell_count(const SylvesBound* b) {
    const TriangleBoundData* d = (const TriangleBoundData*)b->data;
    int count = 0;
    
    for (int x = d->min_x; x <= d->max_x; x++) {
        for (int y = d->min_y; y <= d->max_y; y++) {
            for (int z = d->min_z; z <= d->max_z; z++) {
                int sum = x + y + z;
                if (sum == 1 || sum == 2) {
                    count++;
                }
            }
        }
    }
    return count;
}

static SylvesBound* triangle_clone(const SylvesBound* b) {
    const TriangleBoundData* d = (const TriangleBoundData*)b->data;
    return sylves_bound_create_triangle_parallelogram(d->min_x, d->min_y, d->min_z,
                                                     d->max_x, d->max_y, d->max_z);
}

static bool triangle_is_empty(const SylvesBound* b) {
    const TriangleBoundData* d = (const TriangleBoundData*)b->data;
    return d->min_x > d->max_x || d->min_y > d->max_y || d->min_z > d->max_z;
}

static int triangle_get_aabb(const SylvesBound* b, float* min, float* max) {
    /* Triangle bounds don't map directly to AABB - return approximate bounds */
    const TriangleBoundData* d = (const TriangleBoundData*)b->data;
    
    if (min) {
        min[0] = (float)d->min_x * 0.5f;
        min[1] = (float)d->min_y * 0.866025f; /* sqrt(3)/2 */
        min[2] = 0.0f;
    }
    if (max) {
        max[0] = (float)(d->max_x + 1) * 0.5f;
        max[1] = (float)(d->max_y + 1) * 0.866025f;
        max[2] = 1.0f;
    }
    return 0;
}

static const SylvesBoundVTable TRIANGLE_BOUND_VT = {
    .contains = triangle_bound_contains,
    .destroy = triangle_bound_destroy,
    .name = triangle_bound_name,
    .get_cells = triangle_bound_get_cells,
    .get_rect = triangle_bound_get_rect,
    .get_cube = triangle_bound_get_cube,
    .intersect = triangle_intersect,
    .union_bounds = triangle_union,
    .get_cell_count = triangle_get_cell_count,
    .clone = triangle_clone,
    .is_empty = triangle_is_empty,
    .get_aabb = triangle_get_aabb
};

void sylves_bound_destroy(SylvesBound* bound) {
    if (!bound) return;
    if (bound->vtable && bound->vtable->destroy) bound->vtable->destroy(bound);
}

bool sylves_bound_contains(const SylvesBound* bound, SylvesCell cell) {
    return sylves_bound_call_contains(bound, cell);
}

SylvesBoundType sylves_bound_get_type(const SylvesBound* bound) {
    if (!bound) return SYLVES_BOUND_TYPE_UNKNOWN;
    return (SylvesBoundType)bound->type;
}

int sylves_bound_get_cells(const SylvesBound* bound, SylvesCell* cells, size_t max_cells) {
    if (!bound || !bound->vtable || !bound->vtable->get_cells) return -1;
    return bound->vtable->get_cells(bound, cells, max_cells);
}

int sylves_bound_get_rect(const SylvesBound* bound, int* min_x, int* min_y, int* max_x, int* max_y) {
    if (!bound || !bound->vtable || !bound->vtable->get_rect) return -1;
    return bound->vtable->get_rect(bound, min_x, min_y, max_x, max_y);
}

int sylves_bound_get_cube(const SylvesBound* bound, int* min_x, int* min_y, int* min_z,
                          int* max_x, int* max_y, int* max_z) {
    if (!bound || !bound->vtable || !bound->vtable->get_cube) return -1;
    return bound->vtable->get_cube(bound, min_x, min_y, min_z, max_x, max_y, max_z);
}

int sylves_hex_bound_get_min_mex(const SylvesBound* bound,
                                 int* min_x, int* min_y, int* min_z,
                                 int* mex_x, int* mex_y, int* mex_z) {
    if (!bound || bound->type != SYLVES_BOUND_TYPE_HEX) return -1;
    const HexBoundData* d = (const HexBoundData*)bound->data;
    if (min_x) *min_x = d->min_x; if (min_y) *min_y = d->min_y; if (min_z) *min_z = d->min_z;
    if (mex_x) *mex_x = d->mex_x; if (mex_y) *mex_y = d->mex_y; if (mex_z) *mex_z = d->mex_z;
    return 0;
}

/* Generic intersect/union for rectangle-like 2D bounds (RECT/HEX) */
SylvesBound* sylves_bound_intersect(const SylvesBound* a, const SylvesBound* b) {
    if (!a || !b) return NULL;
    int a_minx, a_miny, a_maxx, a_maxy;
    int b_minx, b_miny, b_maxx, b_maxy;
    if (sylves_bound_get_rect(a, &a_minx, &a_miny, &a_maxx, &a_maxy) != 0) return NULL;
    if (sylves_bound_get_rect(b, &b_minx, &b_miny, &b_maxx, &b_maxy) != 0) return NULL;
    int minx = (a_minx > b_minx ? a_minx : b_minx);
    int miny = (a_miny > b_miny ? a_miny : b_miny);
    int maxx = (a_maxx < b_maxx ? a_maxx : b_maxx);
    int maxy = (a_maxy < b_maxy ? a_maxy : b_maxy);
    if (minx > maxx || miny > maxy) {
        /* Empty intersection: return empty rectangle */
        return sylves_bound_create_rectangle(1,1,0,0);
    }
    /* Prefer HEX if both are HEX */
    if (sylves_bound_get_type(a) == SYLVES_BOUND_TYPE_HEX && sylves_bound_get_type(b) == SYLVES_BOUND_TYPE_HEX) {
        return sylves_bound_create_hex_parallelogram(minx, miny, maxx, maxy);
    }
    return sylves_bound_create_rectangle(minx, miny, maxx, maxy);
}

SylvesBound* sylves_bound_union(const SylvesBound* a, const SylvesBound* b) {
    if (!a || !b) return NULL;
    int a_minx, a_miny, a_maxx, a_maxy;
    int b_minx, b_miny, b_maxx, b_maxy;
    if (sylves_bound_get_rect(a, &a_minx, &a_miny, &a_maxx, &a_maxy) != 0) return NULL;
    if (sylves_bound_get_rect(b, &b_minx, &b_miny, &b_maxx, &b_maxy) != 0) return NULL;
    int minx = (a_minx < b_minx ? a_minx : b_minx);
    int miny = (a_miny < b_miny ? a_miny : b_miny);
    int maxx = (a_maxx > b_maxx ? a_maxx : b_maxx);
    int maxy = (a_maxy > b_maxy ? a_maxy : b_maxy);
    /* Preserve HEX if both HEX else default to RECT */
    if (sylves_bound_get_type(a) == SYLVES_BOUND_TYPE_HEX && sylves_bound_get_type(b) == SYLVES_BOUND_TYPE_HEX) {
        return sylves_bound_create_hex_parallelogram(minx, miny, maxx, maxy);
    }
    return sylves_bound_create_rectangle(minx, miny, maxx, maxy);
}

SylvesBound* sylves_bound_create_triangle_parallelogram(int min_x, int min_y, int min_z,
                                                        int max_x, int max_y, int max_z) {
    SylvesBound* b = (SylvesBound*)calloc(1, sizeof(SylvesBound));
    if (!b) return NULL;
    TriangleBoundData* d = (TriangleBoundData*)calloc(1, sizeof(TriangleBoundData));
    if (!d) { free(b); return NULL; }
    d->min_x = min_x; d->min_y = min_y; d->min_z = min_z;
    d->max_x = max_x; d->max_y = max_y; d->max_z = max_z;
    b->vtable = &TRIANGLE_BOUND_VT; b->data = d; b->type = SYLVES_BOUND_TYPE_TRIANGLE;
    return b;
}

/* CubeBound specific functions */
SylvesBound* sylves_cube_bound_create(int min_x, int min_y, int min_z,
                                      int max_x, int max_y, int max_z) {
    return sylves_bound_create_cube(min_x, min_y, min_z, max_x, max_y, max_z);
}

int sylves_cube_bound_get_min_x(const SylvesBound* bound) {
    if (!bound || bound->type != SYLVES_BOUND_TYPE_CUBE) return 0;
    const CubeBoundData* d = (const CubeBoundData*)bound->data;
    return d->min_x;
}

int sylves_cube_bound_get_min_y(const SylvesBound* bound) {
    if (!bound || bound->type != SYLVES_BOUND_TYPE_CUBE) return 0;
    const CubeBoundData* d = (const CubeBoundData*)bound->data;
    return d->min_y;
}

int sylves_cube_bound_get_min_z(const SylvesBound* bound) {
    if (!bound || bound->type != SYLVES_BOUND_TYPE_CUBE) return 0;
    const CubeBoundData* d = (const CubeBoundData*)bound->data;
    return d->min_z;
}

int sylves_cube_bound_get_max_x(const SylvesBound* bound) {
    if (!bound || bound->type != SYLVES_BOUND_TYPE_CUBE) return 0;
    const CubeBoundData* d = (const CubeBoundData*)bound->data;
    return d->max_x;
}

int sylves_cube_bound_get_max_y(const SylvesBound* bound) {
    if (!bound || bound->type != SYLVES_BOUND_TYPE_CUBE) return 0;
    const CubeBoundData* d = (const CubeBoundData*)bound->data;
    return d->max_y;
}

int sylves_cube_bound_get_max_z(const SylvesBound* bound) {
    if (!bound || bound->type != SYLVES_BOUND_TYPE_CUBE) return 0;
    const CubeBoundData* d = (const CubeBoundData*)bound->data;
    return d->max_z;
}

/* Additional public API functions for bounds operations */
int sylves_bound_get_cell_count(const SylvesBound* bound) {
    if (!bound || !bound->vtable || !bound->vtable->get_cell_count) return -1;
    return bound->vtable->get_cell_count(bound);
}

SylvesBound* sylves_bound_clone(const SylvesBound* bound) {
    if (!bound || !bound->vtable || !bound->vtable->clone) return NULL;
    return bound->vtable->clone(bound);
}

bool sylves_bound_is_empty(const SylvesBound* bound) {
    if (!bound || !bound->vtable || !bound->vtable->is_empty) return true;
    return bound->vtable->is_empty(bound);
}

int sylves_bound_get_aabb(const SylvesBound* bound, float* min, float* max) {
    if (!bound || !bound->vtable || !bound->vtable->get_aabb) return -1;
    return bound->vtable->get_aabb(bound, min, max);
}

/* Enhanced intersection/union that use vtable dispatch */
SylvesBound* sylves_bound_intersect_ex(const SylvesBound* a, const SylvesBound* b) {
    if (!a || !b) return NULL;
    
    /* Try vtable intersect first */
    if (a->vtable && a->vtable->intersect) {
        SylvesBound* result = a->vtable->intersect(a, b);
        if (result) return result;
    }
    if (b->vtable && b->vtable->intersect) {
        SylvesBound* result = b->vtable->intersect(b, a);
        if (result) return result;
    }
    
    /* Fall back to generic implementation */
    return sylves_bound_intersect(a, b);
}

SylvesBound* sylves_bound_union_ex(const SylvesBound* a, const SylvesBound* b) {
    if (!a || !b) return NULL;
    
    /* Try vtable union first */
    if (a->vtable && a->vtable->union_bounds) {
        SylvesBound* result = a->vtable->union_bounds(a, b);
        if (result) return result;
    }
    if (b->vtable && b->vtable->union_bounds) {
        SylvesBound* result = b->vtable->union_bounds(b, a);
        if (result) return result;
    }
    
    /* Fall back to generic implementation */
    return sylves_bound_union(a, b);
}
