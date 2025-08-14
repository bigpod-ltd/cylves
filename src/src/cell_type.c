#include "sylves/cell_type.h"
#include "sylves/vector.h"
#include "sylves/matrix.h"
#include "internal/cell_type_internal.h"
#include "stdlib.h"
#include "string.h"
#include "math.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef enum {
    CTK_SQUARE,
    CTK_HEX_FT,
    CTK_HEX_PT,
    CTK_TRI_FT,
    CTK_TRI_FS,
    CTK_CUBE,
} CellTypeKind;

typedef struct {
    CellTypeKind kind;
} CellTypeData;

static SylvesCellType* ct_create(CellTypeKind kind, const char* name_unused);

/* Internal helpers for NGon-like rotations */
static inline int rot_is_reflection(SylvesCellRotation r) { return r < 0; }
static inline int rot_value(SylvesCellRotation r) { return r < 0 ? ~r : r; }

static SylvesCellRotation ngon_multiply(int n, SylvesCellRotation a, SylvesCellRotation b) {
    int ia = (int)a;
    int ib = (int)b;
    if (ia >= 0) {
        if (ib >= 0) {
            return (SylvesCellRotation)((ia + ib) % n);
        } else {
            return (SylvesCellRotation)~((n + ia + ~ib) % n);
        }
    } else {
        if (ib >= 0) {
            return (SylvesCellRotation)~((n + ~ia - ib) % n);
        } else {
            return (SylvesCellRotation)((n + ~ia - ~ib) % n);
        }
    }
}

static SylvesCellRotation ngon_invert(int n, SylvesCellRotation a) {
    if ((int)a < 0) return a;
    return (SylvesCellRotation)((n - (int)a) % n);
}

static SylvesCellDir ngon_rotate_dir(int n, SylvesCellDir d, SylvesCellRotation r) {
    if ((int)r >= 0) return (SylvesCellDir)(((int)d + (int)r) % n);
    return (SylvesCellDir)((n - (int)d + ~(int)r) % n);
}

static SylvesCellCorner ngon_rotate_corner(int n, SylvesCellCorner c, SylvesCellRotation r) {
    if ((int)r >= 0) return (SylvesCellCorner)(((int)c + (int)r) % n);
    return (SylvesCellCorner)((1 + n - (int)c + ~(int)r) % n);
}

static int ngon_dir_count(CellTypeKind kind) {
    switch (kind) {
        case CTK_SQUARE: return 4;
        case CTK_HEX_FT:
        case CTK_HEX_PT: return 6;
        case CTK_TRI_FT:
        case CTK_TRI_FS: return 6; /* up/down triangles as separate dirs */
        case CTK_CUBE: return 6;
    }
    return 0;
}

static int ngon_corner_count(CellTypeKind kind) {
    switch (kind) {
        case CTK_SQUARE: return 4;
        case CTK_HEX_FT:
        case CTK_HEX_PT: return 6;
        case CTK_TRI_FT:
        case CTK_TRI_FS: return 6; /* up/down triangles corners */
        case CTK_CUBE: return 8;
    }
    return 0;
}

static int ct_get_dimension(const SylvesCellType* ct) {
    const CellTypeData* d = (const CellTypeData*)ct->data;
    return d->kind == CTK_CUBE ? 3 : 2;
}

static int ct_get_dir_count(const SylvesCellType* ct) {
    const CellTypeData* d = (const CellTypeData*)ct->data;
    return ngon_dir_count(d->kind);
}

static int ct_get_corner_count(const SylvesCellType* ct) {
    const CellTypeData* d = (const CellTypeData*)ct->data;
    return ngon_corner_count(d->kind);
}

static SylvesVector3 ct_get_corner_pos(const SylvesCellType* ct, SylvesCellCorner c) {
    const CellTypeData* d = (const CellTypeData*)ct->data;
    switch (d->kind) {
        case CTK_SQUARE: {
            switch (c % 4) {
                case 0: return sylves_vector3_create(0.5, -0.5, 0.0);
                case 1: return sylves_vector3_create(0.5, 0.5, 0.0);
                case 2: return sylves_vector3_create(-0.5, 0.5, 0.0);
                default: return sylves_vector3_create(-0.5, -0.5, 0.0);
            }
        }
        case CTK_HEX_FT:
        case CTK_HEX_PT: {
            int n = 6;
            double inr = 0.5;
            double angle0 = (d->kind == CTK_HEX_FT) ? 0.0 : M_PI/6.0;
            int i = ((int)c % n + n) % n;
            double ang = (-0.5 + i) * (2.0*M_PI/n) + angle0;
            double circum = inr / cos(M_PI/n);
            return sylves_vector3_create(cos(ang)*circum, sin(ang)*circum, 0.0);
        }
        case CTK_TRI_FT:
        case CTK_TRI_FS: {
            double inr = 0.5;
            double R = inr / cos(M_PI/3.0);
            int i = ((int)c % 3 + 3) % 3;
            int up = ((int)c / 3) % 2 == 0;
            double base = (d->kind == CTK_TRI_FT) ? 0.0 : M_PI/2.0;
            double rot = up ? 0.0 : M_PI;
            double ang = base + rot - M_PI/2.0 + i * 2.0*M_PI/3.0;
            return sylves_vector3_create(cos(ang)*R, sin(ang)*R, 0.0);
        }
        case CTK_CUBE: {
            int idx = ((int)c % 8 + 8) % 8;
            int sx = (idx & 1) ? 1 : -1;
            int sy = (idx & 2) ? 1 : -1;
            int sz = (idx & 4) ? 1 : -1;
            return sylves_vector3_create(0.5*sx, 0.5*sy, 0.5*sz);
        }
    }
    return sylves_vector3_zero();
}

static const char* ct_name(const SylvesCellType* ct) {
    const CellTypeData* d = (const CellTypeData*)ct->data;
    switch (d->kind) {
        case CTK_SQUARE: return "Square";
        case CTK_HEX_FT: return "Hex(FlatTopped)";
        case CTK_HEX_PT: return "Hex(PointyTopped)";
        case CTK_TRI_FT: return "Triangle(FlatTopped)";
        case CTK_TRI_FS: return "Triangle(FlatSides)";
        case CTK_CUBE: return "Cube";
    }
    return "Unknown";
}

static void ct_destroy(SylvesCellType* ct) {
    if (!ct) return;
    if (ct->data) free(ct->data);
    free(ct);
}

static const SylvesCellTypeVTable CT_VTABLE = {
    .get_dimension = ct_get_dimension,
    .get_dir_count = ct_get_dir_count,
    .get_corner_count = ct_get_corner_count,
    .get_corner_pos = ct_get_corner_pos,
    .name = ct_name,
    .destroy = ct_destroy,
};

static SylvesCellType* ct_create(CellTypeKind kind, const char* name_unused) {
    (void)name_unused;
    SylvesCellType* ct = (SylvesCellType*)calloc(1, sizeof(SylvesCellType));
    if (!ct) return NULL;
    CellTypeData* data = (CellTypeData*)calloc(1, sizeof(CellTypeData));
    if (!data) { free(ct); return NULL; }
    data->kind = kind;
    ct->vtable = &CT_VTABLE;
    ct->data = data;
    return ct;
}

/* Public API wrappers */

/* Factories */
SylvesCellType* sylves_square_cell_type_create(void) { return ct_create(CTK_SQUARE, "Square"); }
SylvesCellType* sylves_hex_cell_type_create(bool is_flat_topped) { return ct_create(is_flat_topped ? CTK_HEX_FT : CTK_HEX_PT, "Hex"); }
SylvesCellType* sylves_triangle_cell_type_create(bool is_flat_topped) { return ct_create(is_flat_topped ? CTK_TRI_FT : CTK_TRI_FS, "Triangle"); }
SylvesCellType* sylves_cube_cell_type_create(void) { return ct_create(CTK_CUBE, "Cube"); }

int sylves_cell_type_get_dir_count(const SylvesCellType* cell_type) {
    if (!cell_type || !cell_type->vtable || !cell_type->vtable->get_dir_count) return 0;
    return cell_type->vtable->get_dir_count(cell_type);
}

int sylves_cell_type_get_corner_count(const SylvesCellType* cell_type) {
    if (!cell_type || !cell_type->vtable || !cell_type->vtable->get_corner_count) return 0;
    return cell_type->vtable->get_corner_count(cell_type);
}

int sylves_cell_type_get_dimension(const SylvesCellType* cell_type) {
    return cell_type && cell_type->vtable && cell_type->vtable->get_dimension
        ? cell_type->vtable->get_dimension(cell_type) : 0;
}

int sylves_cell_type_get_dirs(const SylvesCellType* cell_type, SylvesCellDir* dirs, size_t max_dirs) {
    if (!cell_type) return 0;
    int n = sylves_cell_type_get_dir_count(cell_type);
    int count = (int)((size_t)n < max_dirs ? n : max_dirs);
    if (dirs) { for (int i=0;i<count;i++) dirs[i] = i; }
    return count;
}

int sylves_cell_type_get_corners(const SylvesCellType* cell_type, SylvesCellCorner* corners, size_t max_corners) {
    if (!cell_type) return 0;
    int n = sylves_cell_type_get_corner_count(cell_type);
    int count = (int)((size_t)n < max_corners ? n : max_corners);
    if (corners) { for (int i=0;i<count;i++) corners[i] = i; }
    return count;
}

int sylves_cell_type_get_rotations(const SylvesCellType* cell_type,
                                   SylvesCellRotation* rotations, size_t max_rotations,
                                   bool include_reflections) {
    if (!cell_type) return 0;
    const CellTypeData* d = (const CellTypeData*)cell_type->data;
    if (d->kind == CTK_CUBE) {
        SylvesCellRotation rset[8] = { 0, ~0, ~2, ~1, 1 };
        int k = 5;
        int count = (int)((size_t)k < max_rotations ? k : max_rotations);
        if (rotations) { for (int i=0;i<count;i++) rotations[i] = rset[i]; }
        return count;
    } else {
        int n = (d->kind == CTK_SQUARE) ? 4 : 6;
        int k = include_reflections ? 2*n : n;
        int count = (int)((size_t)k < max_rotations ? k : max_rotations);
        if (rotations) {
            for (int i=0;i<n && i<count;i++) rotations[i] = i;
            if (include_reflections) {
                for (int i=0;i<n && (n+i)<count;i++) rotations[n+i] = ~i;
            }
        }
        return k;
    }
}

bool sylves_cell_type_invert_dir(const SylvesCellType* cell_type,
                                 SylvesCellDir dir, SylvesCellDir* inverse_dir) {
    if (!cell_type) return false;
    const CellTypeData* d = (const CellTypeData*)cell_type->data;
    switch (d->kind) {
        case CTK_SQUARE: if (inverse_dir) *inverse_dir = (dir + 2) % 4; return true;
        case CTK_HEX_FT:
        case CTK_HEX_PT:
        case CTK_TRI_FT:
        case CTK_TRI_FS: if (inverse_dir) *inverse_dir = (dir + 3) % 6; return true;
        case CTK_CUBE:
            if (inverse_dir) {
                switch (dir % 6) {
                    case 0: *inverse_dir = 1; break;
                    case 1: *inverse_dir = 0; break;
                    case 2: *inverse_dir = 3; break;
                    case 3: *inverse_dir = 2; break;
                    case 4: *inverse_dir = 5; break;
                    default: *inverse_dir = 4; break;
                }
            }
            return true;
    }
    return false;
}

SylvesCellDir sylves_cell_type_rotate_dir(const SylvesCellType* cell_type,
                                          SylvesCellDir dir, SylvesCellRotation rotation) {
    const CellTypeData* d = (const CellTypeData*)cell_type->data;
    switch (d->kind) {
        case CTK_SQUARE: return ngon_rotate_dir(4, dir, rotation);
        case CTK_HEX_FT:
        case CTK_HEX_PT:
        case CTK_TRI_FT:
        case CTK_TRI_FS: return ngon_rotate_dir(6, dir, rotation);
        case CTK_CUBE: {
            int r = (int)rotation;
            if (r >= 0) {
                int k = r % 4;
                int d0 = dir;
                for (int i=0;i<k;i++) {
                    if (d0==1) d0=2; else if (d0==2) d0=0; else if (d0==0) d0=3; else if (d0==3) d0=1;
                }
                return d0;
            } else {
                int v = ~r % 4;
                if (v==2) { if (dir==0) return 1; if (dir==1) return 0; }
                return dir;
            }
        }
    }
    return dir;
}

SylvesCellCorner sylves_cell_type_rotate_corner(const SylvesCellType* cell_type,
                                                SylvesCellCorner corner, SylvesCellRotation rotation) {
    const CellTypeData* d = (const CellTypeData*)cell_type->data;
    switch (d->kind) {
        case CTK_SQUARE: return ngon_rotate_corner(4, corner, rotation);
        case CTK_HEX_FT:
        case CTK_HEX_PT:
        case CTK_TRI_FT:
        case CTK_TRI_FS: return ngon_rotate_corner(6, corner, rotation);
        case CTK_CUBE: return corner;
    }
    return corner;
}

SylvesCellRotation sylves_cell_type_multiply_rotations(const SylvesCellType* cell_type,
                                                       SylvesCellRotation a, SylvesCellRotation b) {
    const CellTypeData* d = (const CellTypeData*)cell_type->data;
    switch (d->kind) {
        case CTK_SQUARE: return ngon_multiply(4, a, b);
        case CTK_HEX_FT:
        case CTK_HEX_PT:
        case CTK_TRI_FT:
        case CTK_TRI_FS: return ngon_multiply(6, a, b);
        case CTK_CUBE: return ngon_multiply(4, a, b);
    }
    return 0;
}

SylvesCellRotation sylves_cell_type_invert_rotation(const SylvesCellType* cell_type,
                                                    SylvesCellRotation rotation) {
    const CellTypeData* d = (const CellTypeData*)cell_type->data;
    switch (d->kind) {
        case CTK_SQUARE: return ngon_invert(4, rotation);
        case CTK_HEX_FT:
        case CTK_HEX_PT:
        case CTK_TRI_FT:
        case CTK_TRI_FS: return ngon_invert(6, rotation);
        case CTK_CUBE: return ngon_invert(4, rotation);
    }
    return 0;
}

SylvesCellRotation sylves_cell_type_get_identity_rotation(const SylvesCellType* cell_type) {
    (void)cell_type; return 0;
}

SylvesVector3 sylves_cell_type_get_corner_position(const SylvesCellType* cell_type,
                                                   SylvesCellCorner corner) {
    if (!cell_type || !cell_type->vtable || !cell_type->vtable->get_corner_pos) return sylves_vector3_zero();
    return cell_type->vtable->get_corner_pos(cell_type, corner);
}

SylvesMatrix4x4 sylves_cell_type_get_rotation_matrix(const SylvesCellType* cell_type,
                                                     SylvesCellRotation rotation) {
    const CellTypeData* d = (const CellTypeData*)cell_type->data;
    if (d->kind == CTK_CUBE) {
        int r = (int)rotation;
        SylvesMatrix4x4 m = sylves_matrix4x4_identity();
        if (r >= 0) {
            double ang = (M_PI/2.0) * (r % 4);
            m = sylves_matrix4x4_rotation_z(ang);
        } else {
            int v = ~r % 4;
            if (v==0) {
                SylvesMatrix4x4 s = sylves_matrix4x4_identity();
                s.m[5] = -1.0;
                m = s;
            } else if (v==2) {
                SylvesMatrix4x4 s = sylves_matrix4x4_identity();
                s.m[0] = -1.0;
                m = s;
            }
        }
        return m;
    } else {
        int n = (d->kind == CTK_SQUARE) ? 4 : 6;
        int i = (int)rotation;
        int rot = i < 0 ? ~i : i;
        int is_ref = i < 0;
        SylvesMatrix4x4 m = sylves_matrix4x4_identity();
        if (is_ref) {
            SylvesMatrix4x4 s = sylves_matrix4x4_identity();
            s.m[5] = -1.0;
            m = s;
        }
        double ang = 2.0*M_PI/n * rot;
        SylvesMatrix4x4 rz = sylves_matrix4x4_rotation_z(ang);
        return sylves_matrix4x4_multiply(&rz, &m);
    }
}

void sylves_cell_type_get_connection(const SylvesCellType* cell_type,
                                     SylvesCellDir dir, SylvesCellRotation rotation,
                                     SylvesCellDir* result_dir, SylvesConnection* connection) {
    if (result_dir) *result_dir = sylves_cell_type_rotate_dir(cell_type, dir, rotation);
    if (connection) {
        connection->rotation = rot_value(rotation);
        connection->is_mirror = rot_is_reflection(rotation);
    }
}

bool sylves_cell_type_try_get_rotation(const SylvesCellType* cell_type,
                                       SylvesCellDir from_dir, SylvesCellDir to_dir,
                                       const SylvesConnection* connection,
                                       SylvesCellRotation* rotation) {
    const CellTypeData* d = (const CellTypeData*)cell_type->data;
    if (d->kind == CTK_CUBE) {
        int mir = connection && connection->is_mirror;
        int rot = connection ? (int)connection->rotation : 0;
        if (rotation) *rotation = mir ? ~rot : rot;
        return true;
    } else {
        int n = (d->kind == CTK_SQUARE) ? 4 : 6;
        if (connection && connection->is_mirror) {
            int delta = ((int)to_dir + (int)from_dir) % n + n;
            if (rotation) *rotation = (SylvesCellRotation)~(delta % n);
        } else {
            int delta = ((int)to_dir - (int)from_dir) % n + n;
            if (rotation) *rotation = (SylvesCellRotation)(delta % n);
        }
        return true;
    }
}

const char* sylves_cell_type_get_name(const SylvesCellType* cell_type) {
    if (!cell_type || !cell_type->vtable || !cell_type->vtable->name) return NULL;
    return cell_type->vtable->name(cell_type);
}

void sylves_cell_type_destroy(SylvesCellType* cell_type) {
    if (cell_type && cell_type->vtable && cell_type->vtable->destroy) cell_type->vtable->destroy(cell_type);
}
