/**
 * @file hex_grid.c
 * @brief Hex grid core implementation (orientation, creation, conversions)
 */

#include "sylves/hex_grid.h"
#include "sylves/vector.h"
#include "sylves/cell.h"
#include "sylves/cell_type.h"
#include "sylves/triangle_grid.h"
#include "sylves/hex_rotation.h"
#include "grid_internal.h"
#include "square_grid_internal.h" /* reuse patterns */
#include "sylves/bounds.h"
#include <stdlib.h>
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Internal data */
typedef struct {
    SylvesHexOrientation orient;
    double cell_size_x;
    double cell_size_y;
    int min_q, min_r, max_q, max_r;
    int is_bounded;
} HexGridData;

/* Forward decls */
static void hex_destroy(SylvesGrid* grid);
static bool hex_is_2d(const SylvesGrid* grid);
static bool hex_is_3d(const SylvesGrid* grid);
static bool hex_is_planar(const SylvesGrid* grid);
static bool hex_is_repeating(const SylvesGrid* grid);
static bool hex_is_orientable(const SylvesGrid* grid);
static bool hex_is_finite(const SylvesGrid* grid);
static int  hex_get_coordinate_dimension(const SylvesGrid* grid);
static bool hex_is_cell_in_grid(const SylvesGrid* grid, SylvesCell cell);
static const SylvesCellType* hex_get_cell_type(const SylvesGrid* grid, SylvesCell cell);

static int hex_get_cell_dirs(const SylvesGrid* grid, SylvesCell cell, SylvesCellDir* dirs, size_t max_dirs);
static bool hex_try_move(const SylvesGrid* grid, SylvesCell cell, SylvesCellDir dir,
                         SylvesCell* dest, SylvesCellDir* inverse_dir, SylvesConnection* connection);
static int hex_get_cell_corners(const SylvesGrid* grid, SylvesCell cell,
                                SylvesCellCorner* corners, size_t max_corners);
static SylvesVector3 hex_get_cell_center(const SylvesGrid* grid, SylvesCell cell);
static SylvesVector3 hex_get_cell_corner_pos(const SylvesGrid* grid, SylvesCell cell,
                                             SylvesCellCorner corner);
static SylvesError hex_get_cell_aabb(const SylvesGrid* grid, SylvesCell cell, SylvesAabb* aabb);
static int hex_get_polygon(const SylvesGrid* grid, SylvesCell cell,
                           SylvesVector3* vertices, size_t max_vertices);
static bool hex_find_cell(const SylvesGrid* grid, SylvesVector3 position, SylvesCell* cell);

static int hex_raycast(const SylvesGrid* grid, SylvesVector3 origin, SylvesVector3 direction,
                        double max_distance, SylvesRaycastInfo* hits, size_t max_hits);

static const SylvesGridVTable HEX_VT = {
    .destroy = hex_destroy,
    .is_2d = hex_is_2d,
    .is_3d = hex_is_3d,
    .is_planar = hex_is_planar,
    .is_repeating = hex_is_repeating,
    .is_orientable = hex_is_orientable,
    .is_finite = hex_is_finite,
    .get_coordinate_dimension = hex_get_coordinate_dimension,
    .is_cell_in_grid = hex_is_cell_in_grid,
    .get_cell_type = hex_get_cell_type,
    .try_move = hex_try_move,
    .get_cell_dirs = hex_get_cell_dirs,
    .get_cell_corners = hex_get_cell_corners,
    .get_cell_center = hex_get_cell_center,
    .get_polygon = hex_get_polygon,
    .get_cell_corner_pos = hex_get_cell_corner_pos,
    .get_cell_aabb = hex_get_cell_aabb,
    .find_cell = hex_find_cell,
    .raycast = hex_raycast,
    /* index ops provided via helpers for bounded grids */
};

/* Public API */
SylvesGrid* sylves_hex_grid_create(SylvesHexOrientation orient, double cell_size) {
    if (cell_size <= 0.0) return NULL;
    SylvesGrid* g = (SylvesGrid*)calloc(1, sizeof(SylvesGrid));
    if (!g) return NULL;
    HexGridData* d = (HexGridData*)calloc(1, sizeof(HexGridData));
    if (!d) { free(g); return NULL; }
    d->orient = orient; d->is_bounded = 0;
    /* Match Sylves: ComputeCellSize(s):
       PointyTopped: (Sqrt3/2*s, 1*s); FlatTopped: (1*s, Sqrt3/2*s) */
    if (orient == SYLVES_HEX_ORIENTATION_POINTY_TOP) {
        d->cell_size_x = cell_size * (sqrt(3.0) / 2.0);
        d->cell_size_y = cell_size * 1.0;
    } else {
        d->cell_size_x = cell_size * 1.0;
        d->cell_size_y = cell_size * (sqrt(3.0) / 2.0);
    }
    g->vtable = &HEX_VT; g->type = SYLVES_GRID_TYPE_HEX; g->bound = NULL; g->data = d;
    return g;
}

/* ---------------- Helper functions for grid.c integrations (bounded hex) --------------- */
int sylves_hex_grid_enumerate_cells(const SylvesGrid* grid, SylvesCell* cells, size_t max_cells) {
    if (!grid) return SYLVES_ERROR_NULL_POINTER;
    const HexGridData* d = (const HexGridData*)grid->data;
    if (!d->is_bounded) return SYLVES_ERROR_INFINITE_GRID;
    int min_q = d->min_q, max_q = d->max_q, min_r = d->min_r, max_r = d->max_r;
    int total = (max_q - min_q + 1) * (max_r - min_r + 1);
    if (!cells) return total;
    int count = 0;
    for (int q = min_q; q <= max_q; q++) {
        for (int r = min_r; r <= max_r; r++) {
            if ((size_t)count >= max_cells) return count;
            cells[count++] = (SylvesCell){ q, r, 0 };
        }
    }
    return count;
}

int sylves_hex_grid_cell_count(const SylvesGrid* grid) {
    if (!grid) return SYLVES_ERROR_NULL_POINTER;
    const HexGridData* d = (const HexGridData*)grid->data;
    if (!d->is_bounded) return SYLVES_ERROR_INFINITE_GRID;
    return (d->max_q - d->min_q + 1) * (d->max_r - d->min_r + 1);
}

/* Conservative selection of cells overlapping an AABB */
int sylves_hex_grid_get_cells_in_aabb(const SylvesGrid* grid, SylvesVector3 min, SylvesVector3 max,
                                      SylvesCell* cells, size_t max_cells) {
    /* TODO: Wire to TriangleGrid subdivision and dedupe per Sylves */
    if (!grid) return SYLVES_ERROR_NULL_POINTER;
    const HexGridData* d = (const HexGridData*)grid->data;
    double sx = d->cell_size_x;
    double sy = d->cell_size_y;
    /* Convert AABB corners to fractional axial using approximate inverse */
    SylvesCell tmp;
    double qf_min, rf_min, qf_max, rf_max;
    {
        /* use find_cell inversion formulas without rounding */
        double px = min.x, py = min.y;
        if (d->orient == SYLVES_HEX_ORIENTATION_FLAT_TOP) {
            qf_min = px / (0.75 * sx);
            rf_min = (py / sy) - 0.5 * qf_min;
        } else {
            rf_min = py / (0.75 * sy);
            qf_min = (px / sx) - 0.5 * rf_min;
        }
    }
    {
        double px = max.x, py = max.y;
        if (d->orient == SYLVES_HEX_ORIENTATION_FLAT_TOP) {
            qf_max = px / (0.75 * sx);
            rf_max = (py / sy) - 0.5 * qf_max;
        } else {
            rf_max = py / (0.75 * sy);
            qf_max = (px / sx) - 0.5 * rf_max;
        }
    }
    int qmin = (int)floor(fmin(qf_min, qf_max)) - 2;
    int qmax = (int)ceil (fmax(qf_min, qf_max)) + 2;
    int rmin = (int)floor(fmin(rf_min, rf_max)) - 2;
    int rmax = (int)ceil (fmax(rf_min, rf_max)) + 2;

    size_t count = 0;
    for (int q = qmin; q <= qmax; q++) {
        for (int r = rmin; r <= rmax; r++) {
            SylvesCell c = { q, r, 0 };
            if (!hex_is_cell_in_grid(grid, c)) continue;
            SylvesAabb aabb;
            if (hex_get_cell_aabb(grid, c, &aabb) == SYLVES_SUCCESS) {
                if (!(aabb.max.x < min.x || aabb.min.x > max.x || aabb.max.y < min.y || aabb.min.y > max.y)) {
                    if (count < max_cells) cells[count] = c;
                    count++;
                }
            }
        }
    }
    return (int)fmin((double)count, (double)max_cells);
}

SylvesGrid* sylves_hex_grid_bound_by(const SylvesGrid* grid, const SylvesBound* bound) {
    if (!grid || !bound) return NULL;
    const HexGridData* d = (const HexGridData*)grid->data;
    int min_x, min_y, max_x, max_y;
if (sylves_bound_get_type(bound) != SYLVES_BOUND_TYPE_RECT && sylves_bound_get_type(bound) != SYLVES_BOUND_TYPE_HEX) return NULL;
    if (sylves_bound_get_rect(bound, &min_x, &min_y, &max_x, &max_y) != 0) return NULL;
    int bminq = min_x, bminr = min_y, bmaxq = max_x, bmaxr = max_y;
    if (d->is_bounded) {
        /* intersect */
        bminq = (bminq > d->min_q) ? bminq : d->min_q;
        bminr = (bminr > d->min_r) ? bminr : d->min_r;
        bmaxq = (bmaxq < d->max_q) ? bmaxq : d->max_q;
        bmaxr = (bmaxr < d->max_r) ? bmaxr : d->max_r;
    }
    double s_scalar = (d->orient == SYLVES_HEX_ORIENTATION_POINTY_TOP) ? d->cell_size_y : d->cell_size_x;
    return sylves_hex_grid_create_bounded(d->orient, s_scalar, bminq, bminr, bmaxq, bmaxr);
}

SylvesGrid* sylves_hex_grid_unbounded_clone(const SylvesGrid* grid) {
    if (!grid) return NULL;
    const HexGridData* d = (const HexGridData*)grid->data;
    double s_scalar = (d->orient == SYLVES_HEX_ORIENTATION_POINTY_TOP) ? d->cell_size_y : d->cell_size_x;
    return sylves_hex_grid_create(d->orient, s_scalar);
}

SylvesGrid* sylves_hex_grid_create_bounded(SylvesHexOrientation orient, double cell_size,
                                           int min_q, int min_r, int max_q, int max_r) {
    SylvesGrid* g = sylves_hex_grid_create(orient, cell_size);
    if (!g) return NULL;
HexGridData* d = (HexGridData*)g->data;
    d->min_q = min_q; d->min_r = min_r; d->max_q = max_q; d->max_r = max_r; d->is_bounded = 1;
    g->bound = sylves_bound_create_hex_parallelogram(min_q, min_r, max_q, max_r);
    return g;
}

/* Coordinate conversions */
void sylves_hex_axial_to_cube(int q, int r, int* x, int* y, int* z) {
    /* axial q,r -> cube x = q, z = r, y = -x - z */
    if (x) *x = q;
    if (z) *z = r;
    if (y) *y = -q - r;
}

void sylves_hex_cube_to_axial(int x, int y, int z, int* q, int* r) {
    (void)y; /* x + y + z = 0; y is redundant */
    if (q) *q = x;
    if (r) *r = z;
}

void sylves_hex_axial_to_offset_evenq(int q, int r, int* col, int* row) {
    /* Even-q vertical layout */
    int c = q;
    int ro = r + (q + (q&1)) / 2;
    if (col) *col = c;
    if (row) *row = ro;
}

void sylves_hex_offset_evenq_to_axial(int col, int row, int* q, int* r) {
    int rq = col;
    int rr = row - (col + (col&1)) / 2;
    if (q) *q = rq; if (r) *r = rr;
}

/* Vtable impl minimal */
static void hex_destroy(SylvesGrid* grid) {
    if (!grid) return;
    if (grid->bound) {
        /* bounds may be added later */
    }
    free(grid->data);
    free(grid);
}

static bool hex_is_2d(const SylvesGrid* grid) { (void)grid; return true; }
static bool hex_is_3d(const SylvesGrid* grid) { (void)grid; return false; }
static bool hex_is_planar(const SylvesGrid* grid) { (void)grid; return true; }
static bool hex_is_repeating(const SylvesGrid* grid) { (void)grid; return true; }
static bool hex_is_orientable(const SylvesGrid* grid) { (void)grid; return true; }
static bool hex_is_finite(const SylvesGrid* grid) {
const HexGridData* d = (const HexGridData*)grid->data; return d->is_bounded != 0;
}
static int  hex_get_coordinate_dimension(const SylvesGrid* grid) { (void)grid; return 2; }

static bool hex_is_cell_in_grid(const SylvesGrid* grid, SylvesCell cell) {
    /* interpret cell.x,cell.y,cell.z as axial q,r,(unused) in 2D; z must be 0 */
    (void)grid; if (cell.z != 0) return false;
const HexGridData* d = (const HexGridData*)grid->data;
    if (!d->is_bounded) return true;
    int q = cell.x, r = cell.y;
    return q >= d->min_q && q <= d->max_q && r >= d->min_r && r <= d->max_r;
}

static const SylvesCellType* hex_get_cell_type(const SylvesGrid* grid, SylvesCell cell) {
    (void)grid; (void)cell;
    /* Return a basic hex cell type; using flat-topped vs pointy-topped symmetry may differ later */
    static SylvesCellType* hex_ct = NULL;
    if (!hex_ct) hex_ct = sylves_hex_cell_type_create(true);
    return hex_ct;
}

/* Neighbor deltas in axial coordinates (q, r): E, NE, NW, W, SW, SE */
static const int HEX_DQ[6] = {+1, +1, 0, -1, -1, 0};
static const int HEX_DR[6] = { 0, -1,-1,  0, +1, +1};

static int hex_get_cell_dirs(const SylvesGrid* grid, SylvesCell cell, SylvesCellDir* dirs, size_t max_dirs) {
    (void)grid;
    if (cell.z != 0) return SYLVES_ERROR_CELL_NOT_IN_GRID;
    int count = 0;
    for (int i=0;i<6 && (size_t)count<max_dirs;i++) {
        if (dirs) dirs[count] = i;
        count++;
    }
    return count;
}

static bool hex_try_move(const SylvesGrid* grid, SylvesCell cell, SylvesCellDir dir,
                         SylvesCell* dest, SylvesCellDir* inverse_dir, SylvesConnection* connection) {
    if (!hex_is_cell_in_grid(grid, cell)) return false;
    int di = dir % 6; if (di < 0) di += 6;
    SylvesCell next = cell;
    next.x += HEX_DQ[di];
    next.y += HEX_DR[di];
    if (!hex_is_cell_in_grid(grid, next)) return false;
    if (dest) *dest = next;
    if (inverse_dir) *inverse_dir = (di + 3) % 6;
    if (connection) { connection->rotation = 0; connection->is_mirror = false; }
    return true;
}

/* Raycast for hex grid: temporary conservative approach walking AABB-overlapped cells.
   TODO: Replace with TriangleGrid-backed raycast with exact Sylves logic. */
static int hex_raycast(const SylvesGrid* grid, SylvesVector3 origin, SylvesVector3 direction,
                       double max_distance, SylvesRaycastInfo* hits, size_t max_hits) {
    /* Fallback: step to the first boundary and then across cells using simple DDA-like increments in axial space is complex;
       For now, return a single starting cell if inside grid. */
    (void)max_distance;
    SylvesCell start;
    size_t count = 0;
    if (hex_find_cell(grid, origin, &start)) {
        if (hits && max_hits > 0) {
            hits[0].cell = start;
            hits[0].point = origin;
            hits[0].distance = 0.0;
            hits[0].face = 0;
        }
        count = 1;
    }
    return (int)count;
}

static int hex_get_cell_corners(const SylvesGrid* grid, SylvesCell cell,
                                SylvesCellCorner* corners, size_t max_corners) {
    (void)grid; (void)cell;
    int count = (max_corners < 6) ? (int)max_corners : 6;
    if (corners) { for (int i=0;i<count;i++) corners[i] = i; }
    return count;
}

static SylvesVector3 hex_get_cell_corner_pos(const SylvesGrid* grid, SylvesCell cell,
                                             SylvesCellCorner corner) {
    const HexGridData* d = (const HexGridData*)grid->data;
    SylvesVector3 c = hex_get_cell_center(grid, cell);
    double rx = d->cell_size_x * 0.5;
    double ry = d->cell_size_y * 0.5;
    /* Use anisotropic scaling of unit hex angles */
    double angle_offset = (d->orient == SYLVES_HEX_ORIENTATION_FLAT_TOP) ? 0.0 : (acos(-1.0)/6.0);
    int idx = ((int)corner) % 6; if (idx < 0) idx += 6;
    double ang = angle_offset + idx * (acos(-1.0)/3.0);
    return sylves_vector3_create(c.x + rx * cos(ang), c.y + ry * sin(ang), 0.0);
}

static SylvesError hex_get_cell_aabb(const SylvesGrid* grid, SylvesCell cell, SylvesAabb* aabb) {
    if (!aabb) return SYLVES_ERROR_NULL_POINTER;
    const HexGridData* d = (const HexGridData*)grid->data;
    SylvesVector3 c = hex_get_cell_center(grid, cell);
    double ex = d->cell_size_x * 0.5;
    double ey = d->cell_size_y * 0.5;
    aabb->min = sylves_vector3_create(c.x - ex, c.y - ey, 0.0);
    aabb->max = sylves_vector3_create(c.x + ex, c.y + ey, 0.0);
    return SYLVES_SUCCESS;
}

/* Spatial: center and polygon */
static SylvesVector3 hex_get_cell_center(const SylvesGrid* grid, SylvesCell cell) {
    const HexGridData* d = (const HexGridData*)grid->data;
    double sx = d->cell_size_x;
    double sy = d->cell_size_y;
    int x = cell.x; /* cube x */
    int z = cell.y; /* cube z (axial r) */
    int y = -x - z; /* cube y */
    double wx, wy;
    if (d->orient == SYLVES_HEX_ORIENTATION_FLAT_TOP) {
        wx = (0.5 * x - 0.25 * y - 0.25 * z) * sx;
        wy = (0.5 * y - 0.5  * z) * sy;
    } else {
        wx = (0.5 * x - 0.5  * z) * sx;
        wy = (0.5 * y - 0.25 * x - 0.25 * z) * sy;
    }
    return sylves_vector3_create(wx, wy, 0.0);
}

static int hex_get_polygon(const SylvesGrid* grid, SylvesCell cell,
                           SylvesVector3* vertices, size_t max_vertices) {
    if (cell.z != 0) return SYLVES_ERROR_CELL_NOT_IN_GRID;
    const HexGridData* d = (const HexGridData*)grid->data;
    if (d->is_bounded && !hex_is_cell_in_grid(grid, cell)) return SYLVES_ERROR_CELL_NOT_IN_GRID;
    if (max_vertices < 6) return SYLVES_ERROR_BUFFER_TOO_SMALL;

    SylvesVector3 c = hex_get_cell_center(grid, cell);
    double rx = d->cell_size_x * 0.5;
    double ry = d->cell_size_y * 0.5;
    // Angles: flat-top start at 0, step 60 deg; pointy-top start at 30 deg
    double angle_offset = (d->orient == SYLVES_HEX_ORIENTATION_FLAT_TOP) ? 0.0 : (acos(-1.0)/6.0);
    for (int i=0;i<6;i++) {
        double ang = angle_offset + i * (acos(-1.0)/3.0);
        double vx = c.x + rx * cos(ang);
        double vy = c.y + ry * sin(ang);
        if (vertices) vertices[i] = sylves_vector3_create(vx, vy, 0.0);
    }
    return 6;
}

/* Spatial query: position to axial rounding */
static bool hex_find_cell(const SylvesGrid* grid, SylvesVector3 position, SylvesCell* cell) {
    const HexGridData* d = (const HexGridData*)grid->data;
    double sx = d->cell_size_x;
    double sy = d->cell_size_y;
    double px = position.x, py = position.y;
    double qf, rf;
    if (d->orient == SYLVES_HEX_ORIENTATION_FLAT_TOP) {
        // Invert Sylves center formula for flat-top using cube with z=-x-y
        // xw = (0.5*q - 0.25*r - 0.25*(-q-r)) * sx = 0.75*q * sx
        // yw = (0.5*r - 0.5*(-q-r)) * sy = 0.5*(r+q+r) * sy = 0.5*(q+2r) * sy
        // Approximate inverse:
        double q_est = px / (0.75 * sx);
        double r_est = (py / sy) - 0.5 * q_est;
        qf = q_est; rf = r_est;
    } else {
        // pointy-top:
        // xw = (0.5*q - 0.5*(-q-r)) * sx = 0.5*(q+q+r) * sx = 0.5*(2q+r) * sx
        // yw = (0.5*r - 0.25*q - 0.25*(-q-r)) * sy = 0.5*r -0.25*q +0.25*q+0.25*r = 0.75*r * sy
        double r_est = py / (0.75 * sy);
        double q_est = (px / sx) - 0.5 * r_est;
        qf = q_est; rf = r_est;
    }
    // Convert to cube and round
    double xf = qf;
    double zf = rf;
    double yf = -xf - zf;
    int rx = (int)round(xf);
    int ry = (int)round(yf);
    int rz = (int)round(zf);
    double dx = fabs((double)rx - xf);
    double dy = fabs((double)ry - yf);
    double dz = fabs((double)rz - zf);
    if (dx > dy && dx > dz) {
        rx = -ry - rz;
    } else if (dy > dz) {
        ry = -rx - rz;
    } else {
        rz = -rx - ry;
    }
    int q = rx;
    int r = rz;
    SylvesCell cax = { q, r, 0 };
    if (d->is_bounded && !hex_is_cell_in_grid(grid, cax)) return false;
    if (cell) *cell = cax;
    return true;
}

/* Hex/Triangle grid integration - matching Sylves C# implementation */
void sylves_hex_get_child_triangles(SylvesCell hex_cell, SylvesCell triangles[6]) {
    int x, y, z;
    sylves_hex_axial_to_cube(hex_cell.x, hex_cell.y, &x, &y, &z);
    
    int a = x - y;
    int b = y - z;
    int c = z - x;
    
    triangles[0] = (SylvesCell){a + 1, b, c};
    triangles[1] = (SylvesCell){a + 1, b + 1, c};
    triangles[2] = (SylvesCell){a, b + 1, c};
    triangles[3] = (SylvesCell){a, b + 1, c + 1};
    triangles[4] = (SylvesCell){a, b, c + 1};
    triangles[5] = (SylvesCell){a + 1, b, c + 1};
}

SylvesCell sylves_hex_get_triangle_parent(SylvesCell triangle_cell, SylvesHexOrientation orientation) {
    int x = triangle_cell.x;
    int y = triangle_cell.y;
    int z = triangle_cell.z;
    
    int hex_x, hex_y, hex_z;
    if (orientation == SYLVES_HEX_ORIENTATION_FLAT_TOP) {
        hex_x = round((x - z) / 3.0);
        hex_y = round((y - x) / 3.0);
        hex_z = round((z - y) / 3.0);
    } else {
        hex_x = round((x - y) / 3.0);
        hex_y = round((y - z) / 3.0);
        hex_z = round((z - x) / 3.0);
    }
    
    /* Convert back to axial */
    int q, r;
    sylves_hex_cube_to_axial(hex_x, hex_y, hex_z, &q, &r);
    return (SylvesCell){q, r, 0};
}

/* TryMoveByOffset implementation using HexRotation */
bool sylves_hex_try_move_by_offset(const SylvesGrid* grid, SylvesCell start_cell, 
                                   SylvesVector3Int start_offset, SylvesVector3Int dest_offset,
                                   SylvesCellRotation start_rotation, 
                                   SylvesCell* dest_cell, SylvesCellRotation* dest_rotation) {
    if (!hex_is_cell_in_grid(grid, start_cell)) return false;
    
    *dest_rotation = start_rotation;
    
    /* Convert cell to cube coordinates */
    int x, y, z;
    sylves_hex_axial_to_cube(start_cell.x, start_cell.y, &x, &y, &z);
    SylvesVector3Int cube_start = {x, y, z};
    
    /* Apply rotation to offset */
    SylvesVector3Int offset_diff = {
        dest_offset.x - start_offset.x,
        dest_offset.y - start_offset.y,
        dest_offset.z - start_offset.z
    };
    
    SylvesHexRotation hex_rot = sylves_hex_rotation_from_int(start_rotation);
    SylvesVector3Int rotated_offset = sylves_hex_rotation_multiply(hex_rot, offset_diff);
    
    /* Add to start position */
    SylvesVector3Int cube_dest = {
        cube_start.x + rotated_offset.x,
        cube_start.y + rotated_offset.y,
        cube_start.z + rotated_offset.z
    };
    
    /* Convert back to axial */
    int q, r;
    sylves_hex_cube_to_axial(cube_dest.x, cube_dest.y, cube_dest.z, &q, &r);
    *dest_cell = (SylvesCell){q, r, 0};
    
    return hex_is_cell_in_grid(grid, *dest_cell);
}

/* Symmetry helpers */
bool sylves_hex_grid_try_move_by_offset(const SylvesGrid* grid, SylvesCell cell, 
                                         SylvesVector3Int offset, SylvesCell* dest) {
    if (!grid || !dest || grid->type != SYLVES_GRID_TYPE_HEX) return false;
    if (!hex_is_cell_in_grid(grid, cell)) return false;
    
    /* Convert cell to cube coordinates */
    int x, y, z;
    sylves_hex_axial_to_cube(cell.x, cell.y, &x, &y, &z);
    
    /* Add offset */
    x += offset.x;
    y += offset.y;
    z += offset.z;
    
    /* Convert back to axial */
    int q, r;
    sylves_hex_cube_to_axial(x, y, z, &q, &r);
    *dest = (SylvesCell){q, r, 0};
    
    return hex_is_cell_in_grid(grid, *dest);
}

SylvesCellDir sylves_hex_grid_parallel_transport(const SylvesGrid* grid, SylvesCell from_cell, 
                                                 SylvesCell to_cell, SylvesCellDir dir) {
    if (!grid || grid->type != SYLVES_GRID_TYPE_HEX) return dir;
    
    /* In a regular hex grid, parallel transport preserves direction */
    /* This is because hex grids have no curvature */
    return dir;
}

bool sylves_hex_grid_try_apply_symmetry(const SylvesGrid* grid, 
                                        SylvesGridSymmetry symmetry, SylvesCell cell,
                                        SylvesCell* dest, SylvesHexRotation* rotation) {
    if (!grid || !dest || !rotation || grid->type != SYLVES_GRID_TYPE_HEX) return false;
    if (!hex_is_cell_in_grid(grid, cell)) return false;
    
    /* Apply rotation */
    *rotation = sylves_hex_rotation_from_int(symmetry.rotation);
    
    /* Convert cell to cube coordinates */
    int x, y, z;
    sylves_hex_axial_to_cube(cell.x, cell.y, &x, &y, &z);
    SylvesVector3Int cube_pos = {x, y, z};
    
    /* Apply rotation */
    SylvesVector3Int rotated = sylves_hex_rotation_multiply(*rotation, cube_pos);
    
    /* Apply translation */
    rotated.x += symmetry.translation.x;
    rotated.y += symmetry.translation.y;
    rotated.z += symmetry.translation.z;
    
    /* Convert back to axial */
    int q, r;
    sylves_hex_cube_to_axial(rotated.x, rotated.y, rotated.z, &q, &r);
    *dest = (SylvesCell){q, r, 0};
    
    return hex_is_cell_in_grid(grid, *dest);
}


SylvesCell sylves_triangle_get_hex_parent(SylvesCell tri_cell) {
    int x = tri_cell.x;
    int y = tri_cell.y;
    int z = tri_cell.z;
    
    /* Use the formula from Sylves to find parent hex */
    int hex_x = (int)round((x - z) / 3.0);
    int hex_y = (int)round((y - x) / 3.0);
    int hex_z = (int)round((z - y) / 3.0);
    
    /* Convert back to axial */
    int q, r;
    sylves_hex_cube_to_axial(hex_x, hex_y, hex_z, &q, &r);
    return (SylvesCell){q, r, 0};
}

