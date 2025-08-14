/**
 * @file prism_grid.c
 * @brief Implementation of Prism grids - 3D extensions of 2D grids
 */

#include "sylves/prism_grid.h"
#include "sylves/types.h"
#include "sylves/errors.h"
#include "sylves/vector.h"
#include "sylves/cell.h"
#include "sylves/cell_type.h"
#include "internal/grid_internal.h"
#include "sylves/bounds.h"
#include "sylves/hex_prism_cell_type.h"
#include "sylves/triangle_prism_cell_type.h"
#include "sylves/cube_cell_type.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Prism grid data structure */
typedef struct {
    double cell_size;
    double layer_height;
    int min_layer, max_layer;
    bool is_bounded;
    SylvesGridType base_type;  /* Type of the 2D base grid */
    bool flat_topped;          /* For hex grids */
    /* Bounds for the base grid */
    int min_x, min_y;
    int max_x, max_y;
} PrismGridData;

/* Forward declarations */
static void prism_destroy(SylvesGrid* grid);
static bool prism_is_2d(const SylvesGrid* grid);
static bool prism_is_3d(const SylvesGrid* grid);
static bool prism_is_planar(const SylvesGrid* grid);
static bool prism_is_repeating(const SylvesGrid* grid);
static bool prism_is_orientable(const SylvesGrid* grid);
static bool prism_is_finite(const SylvesGrid* grid);
static int prism_get_coordinate_dimension(const SylvesGrid* grid);
static SylvesVector3 prism_get_cell_center(const SylvesGrid* grid, SylvesCell cell);
static bool prism_is_cell_in_grid(const SylvesGrid* grid, SylvesCell cell);
static const SylvesCellType* prism_get_cell_type(const SylvesGrid* grid, SylvesCell cell);
static bool prism_try_move(const SylvesGrid* grid, SylvesCell cell, SylvesCellDir dir,
                          SylvesCell* dest, SylvesCellDir* inverse_dir, SylvesConnection* connection);
static int prism_get_cell_dirs(const SylvesGrid* grid, SylvesCell cell,
                              SylvesCellDir* dirs, size_t max_dirs);
static int prism_get_cell_corners(const SylvesGrid* grid, SylvesCell cell,
                                 SylvesCellCorner* corners, size_t max_corners);
static SylvesVector3 prism_get_cell_corner_pos(const SylvesGrid* grid, SylvesCell cell,
                                               SylvesCellCorner corner);
static SylvesError prism_get_mesh_data(const SylvesGrid* grid, SylvesCell cell,
                                       SylvesMeshData** mesh_data);
static bool prism_find_cell(const SylvesGrid* grid, SylvesVector3 position, SylvesCell* cell);
static SylvesError prism_get_cell_aabb(const SylvesGrid* grid, SylvesCell cell, SylvesAabb* aabb);

/* VTable for prism grids */
static const SylvesGridVTable prism_vtable = {
    .destroy = prism_destroy,
    .is_2d = prism_is_2d,
    .is_3d = prism_is_3d,
    .is_planar = prism_is_planar,
    .is_repeating = prism_is_repeating,
    .is_orientable = prism_is_orientable,
    .is_finite = prism_is_finite,
    .get_coordinate_dimension = prism_get_coordinate_dimension,
    .is_cell_in_grid = prism_is_cell_in_grid,
    .get_cell_type = prism_get_cell_type,
    .try_move = prism_try_move,
    .get_cell_dirs = prism_get_cell_dirs,
    .get_cell_corners = prism_get_cell_corners,
    .get_cell_center = prism_get_cell_center,
    .get_cell_corner_pos = prism_get_cell_corner_pos,
    .get_polygon = NULL,  /* Prism grids are 3D, no 2D polygons */
    .get_cell_aabb = prism_get_cell_aabb,
    .find_cell = prism_find_cell,
    .raycast = NULL,  /* TODO: Implement if needed */
    .get_index_count = NULL,  /* TODO: Implement if needed */
    .get_index = NULL,  /* TODO: Implement if needed */
    .get_cell_by_index = NULL  /* TODO: Implement if needed */
};

// Implementation for hex prism grid creation
SylvesGrid* sylves_hex_prism_grid_create(bool flat_topped, double cell_size, double layer_height) {
    // Allocate and initialize the grid
    SylvesGrid* grid = malloc(sizeof(SylvesGrid));
    if (!grid) return NULL;
    
    grid->vtable = &prism_vtable;
    grid->type = SYLVES_GRID_TYPE_CUSTOM;  /* Prism grids are custom type */

    PrismGridData* data = (PrismGridData*)calloc(1, sizeof(PrismGridData));
    if (!data) {
        free(grid);
        return NULL;
    }

    data->cell_size = cell_size;
    data->layer_height = layer_height;
    data->is_bounded = false;
    data->base_type = SYLVES_GRID_TYPE_HEX;
    data->flat_topped = flat_topped;
    grid->data = data;

    return grid;
}

// Implementation for bounded hex prism grid creation
SylvesGrid* sylves_hex_prism_grid_create_bounded(bool flat_topped, double cell_size, double layer_height,
                                                  int min_q, int min_r, int max_q, int max_r,
                                                  int min_layer, int max_layer) {
    SylvesGrid* grid = sylves_hex_prism_grid_create(flat_topped, cell_size, layer_height);
    if (!grid) return NULL;
    
    PrismGridData* data = (PrismGridData*)grid->data;
    data->min_layer = min_layer;
    data->max_layer = max_layer;
    data->is_bounded = true;
    data->min_x = min_q;
    data->min_y = min_r;
    data->max_x = max_q;
    data->max_y = max_r;

    /* TODO: Create proper bound implementation */
    grid->bound = NULL;

    return grid;
}

// Implementation for triangle prism grid creation
SylvesGrid* sylves_triangle_prism_grid_create(double cell_size, double layer_height) {
    SylvesGrid* grid = malloc(sizeof(SylvesGrid));
    if (!grid) return NULL;
    
    grid->vtable = &prism_vtable;
    grid->type = SYLVES_GRID_TYPE_CUSTOM;  /* Prism grids are custom type */

    PrismGridData* data = (PrismGridData*)calloc(1, sizeof(PrismGridData));
    if (!data) {
        free(grid);
        return NULL;
    }

    data->cell_size = cell_size;
    data->layer_height = layer_height;
    data->is_bounded = false;
    data->base_type = SYLVES_GRID_TYPE_TRIANGLE;
    grid->data = data;

    return grid;
}

// Implementation for bounded triangle prism grid creation
SylvesGrid* sylves_triangle_prism_grid_create_bounded(double cell_size, double layer_height,
                                                       int min_x, int min_y, int min_z,
                                                       int max_x, int max_y, int max_z,
                                                       int min_layer, int max_layer) {
    SylvesGrid* grid = sylves_triangle_prism_grid_create(cell_size, layer_height);
    if (!grid) return NULL;
    
    PrismGridData* data = (PrismGridData*)grid->data;
    data->min_layer = min_layer;
    data->max_layer = max_layer;
    data->is_bounded = true;
    data->min_x = min_x;
    data->min_y = min_y;
    data->max_x = max_x;
    data->max_y = max_y;

    /* TODO: Create proper bound implementation */
    grid->bound = NULL;

    return grid;
}

// Implementation for square prism grid creation
SylvesGrid* sylves_square_prism_grid_create(double cell_size, double layer_height) {
    SylvesGrid* grid = malloc(sizeof(SylvesGrid));
    if (!grid) return NULL;
    
    grid->vtable = &prism_vtable;
    grid->type = SYLVES_GRID_TYPE_CUSTOM;  /* Prism grids are custom type */

    PrismGridData* data = (PrismGridData*)calloc(1, sizeof(PrismGridData));
    if (!data) {
        free(grid);
        return NULL;
    }

    data->cell_size = cell_size;
    data->layer_height = layer_height;
    data->is_bounded = false;
    data->base_type = SYLVES_GRID_TYPE_SQUARE;
    grid->data = data;

    return grid;
}

// Implementation for bounded square prism grid creation
SylvesGrid* sylves_square_prism_grid_create_bounded(double cell_size, double layer_height,
                                                     int min_x, int min_y, int max_x, int max_y,
                                                     int min_layer, int max_layer) {
    SylvesGrid* grid = sylves_square_prism_grid_create(cell_size, layer_height);
    if (!grid) return NULL;
    
    PrismGridData* data = (PrismGridData*)grid->data;
    data->min_layer = min_layer;
    data->max_layer = max_layer;
    data->is_bounded = true;
    data->min_x = min_x;
    data->min_y = min_y;
    data->max_x = max_x;
    data->max_y = max_y;

    /* TODO: Create proper bound implementation */
    grid->bound = NULL;

    return grid;
}

/* VTable function implementations */

static void prism_destroy(SylvesGrid* grid) {
    if (grid) {
        if (grid->bound) {
            sylves_bound_destroy((SylvesBound*)grid->bound);
        }
        free(grid->data);
        free(grid);
    }
}

static bool prism_is_2d(const SylvesGrid* grid) {
    return false;
}

static bool prism_is_3d(const SylvesGrid* grid) {
    return true;
}

static bool prism_is_planar(const SylvesGrid* grid) {
    return false;  /* Prism grids are 3D, not planar */
}

static bool prism_is_repeating(const SylvesGrid* grid) {
    return true;  /* Prism grids have a repeating pattern */
}

static bool prism_is_orientable(const SylvesGrid* grid) {
    return true;  /* Prism grids are orientable */
}

static bool prism_is_finite(const SylvesGrid* grid) {
    PrismGridData* data = (PrismGridData*)grid->data;
    return data->is_bounded;
}

static int prism_get_coordinate_dimension(const SylvesGrid* grid) {
    return 3;  /* Prism grids use 3D coordinates */
}

static bool prism_is_cell_in_grid(const SylvesGrid* grid, SylvesCell cell) {
    PrismGridData* data = (PrismGridData*)grid->data;
    
    /* Check layer bounds */
    if (data->is_bounded) {
        if (cell.z < data->min_layer || cell.z > data->max_layer) {
            return false;
        }
        
        /* Check base grid bounds */
        switch (data->base_type) {
            case SYLVES_GRID_TYPE_SQUARE:
                if (cell.x < data->min_x || cell.x > data->max_x ||
                    cell.y < data->min_y || cell.y > data->max_y) {
                    return false;
                }
                break;
                
            case SYLVES_GRID_TYPE_HEX:
                /* For hex grids, use axial coordinates */
                if (cell.x < data->min_x || cell.x > data->max_x ||
                    cell.y < data->min_y || cell.y > data->max_y) {
                    return false;
                }
                break;
                
            case SYLVES_GRID_TYPE_TRIANGLE:
                /* For triangle grids, check triangle coordinate constraint */
                /* TODO: Add proper triangle bounds checking */
                if (cell.x < data->min_x || cell.x > data->max_x ||
                    cell.y < data->min_y || cell.y > data->max_y) {
                    return false;
                }
                break;
                
            default:
                return false;
        }
    }
    
    return true;
}

static const SylvesCellType* prism_get_cell_type(const SylvesGrid* grid, SylvesCell cell) {
    if (!prism_is_cell_in_grid(grid, cell)) {
        return NULL;
    }
    
    PrismGridData* data = (PrismGridData*)grid->data;
    
    /* Return appropriate prism cell type based on base grid type */
    switch (data->base_type) {
        case SYLVES_GRID_TYPE_SQUARE:
            return sylves_cube_cell_type_get();
            
        case SYLVES_GRID_TYPE_HEX:
            return sylves_hex_prism_cell_type_get(data->flat_topped);
            
        case SYLVES_GRID_TYPE_TRIANGLE:
            return sylves_triangle_prism_cell_type_get(data->flat_topped);
            
        default:
            return NULL;
    }
}

static SylvesVector3 prism_get_cell_center(const SylvesGrid* grid, SylvesCell cell) {
    PrismGridData* data = (PrismGridData*)grid->data;
    SylvesVector3 center = {0, 0, 0};
    
    /* Calculate center based on base grid type */
    switch (data->base_type) {
        case SYLVES_GRID_TYPE_HEX:
            /* Hex grid center calculation */
            if (data->flat_topped) {
                /* Flat-topped hex: x = 3/4 * size * q, y = sqrt(3)/2 * size * (r + q/2) */
                center.x = 0.75 * data->cell_size * cell.x;
                center.y = 0.8660254037844386 * data->cell_size * (cell.y + 0.5 * cell.x);
            } else {
                /* Pointy-topped hex: x = sqrt(3)/2 * size * (q + r/2), y = 3/4 * size * r */
                center.x = 0.8660254037844386 * data->cell_size * (cell.x + 0.5 * cell.y);
                center.y = 0.75 * data->cell_size * cell.y;
            }
            break;
            
        case SYLVES_GRID_TYPE_TRIANGLE:
            {
                /* Triangle grid center calculation */
                /* For triangle grids, we use the FTri coordinate system */
                /* Check if triangle is upward or downward pointing */
                double sqrt3_2 = 0.8660254037844386;
                int is_upward = ((cell.x + cell.y + cell.z) == 1);
                
                if (is_upward) {
                    center.x = data->cell_size * (0.5 * cell.x + cell.y);
                    center.y = data->cell_size * sqrt3_2 * cell.x / 3.0;
                } else {
                    center.x = data->cell_size * (0.5 * cell.x + cell.y);
                    center.y = data->cell_size * sqrt3_2 * (cell.x + 2.0) / 3.0;
                }
            }
            break;
            
        case SYLVES_GRID_TYPE_SQUARE:
            /* Square grid center calculation */
            center.x = (cell.x + 0.5) * data->cell_size;
            center.y = (cell.y + 0.5) * data->cell_size;
            break;
            
        default:
            break;
    }
    
    /* Add vertical offset for the layer */
    center.z = (cell.z + 0.5) * data->layer_height;
    
    return center;
}

static bool prism_try_move(const SylvesGrid* grid, SylvesCell cell, SylvesCellDir dir,
                          SylvesCell* dest, SylvesCellDir* inverse_dir, SylvesConnection* connection) {
    if (!prism_is_cell_in_grid(grid, cell)) {
        return false;
    }
    
    PrismGridData* data = (PrismGridData*)grid->data;
    
    /* Prism grids have base grid directions (4 for square, 6 for hex, 3 for triangle)
     * plus 2 vertical directions (up and down) */
    int base_dirs = 0;
    switch (data->base_type) {
        case SYLVES_GRID_TYPE_SQUARE:
            base_dirs = 4;
            break;
        case SYLVES_GRID_TYPE_HEX:
            base_dirs = 6;
            break;
        case SYLVES_GRID_TYPE_TRIANGLE:
            base_dirs = 3;
            break;
        default:
            return false;
    }
    
    *dest = cell;
    
    if (dir < base_dirs) {
        /* Horizontal movement in base grid */
        switch (data->base_type) {
            case SYLVES_GRID_TYPE_SQUARE:
                /* Square directions: 0=Right, 1=Up, 2=Left, 3=Down */
                switch (dir) {
                    case 0: dest->x++; break;
                    case 1: dest->y++; break;
                    case 2: dest->x--; break;
                    case 3: dest->y--; break;
                }
                *inverse_dir = (dir + 2) % 4;
                break;
                
            case SYLVES_GRID_TYPE_HEX:
                /* Hex directions in axial coordinates */
                if (data->flat_topped) {
                    /* Flat-topped: 0=Right, 1=UpRight, 2=UpLeft, 3=Left, 4=DownLeft, 5=DownRight */
                    switch (dir) {
                        case 0: dest->x++; break;
                        case 1: dest->y++; break;
                        case 2: dest->x--; dest->y++; break;
                        case 3: dest->x--; break;
                        case 4: dest->y--; break;
                        case 5: dest->x++; dest->y--; break;
                    }
                } else {
                    /* Pointy-topped directions */
                    switch (dir) {
                        case 0: dest->x++; dest->y--; break;
                        case 1: dest->x++; break;
                        case 2: dest->y++; break;
                        case 3: dest->x--; dest->y++; break;
                        case 4: dest->x--; break;
                        case 5: dest->y--; break;
                    }
                }
                *inverse_dir = (dir + 3) % 6;
                break;
                
            case SYLVES_GRID_TYPE_TRIANGLE:
                /* Triangle directions: proper navigation based on triangle orientation */
                {
                    /* Determine if triangle is up/down pointing */
                    int is_upward = ((cell.x + cell.y + cell.z) == 1);
                    if (is_upward) {
                        /* Up-pointing triangle has dirs: 0, 2, 4 */
                        switch (dir) {
                            case 0: dest->x++; break;           /* Right */
                            case 1: dest->y++; break;           /* UpLeft */  
                            case 2: dest->y--; break;           /* DownRight */
                        }
                        /* Inverse directions for up-pointing triangles */
                        *inverse_dir = (dir == 0) ? 0 : (dir == 1) ? 2 : 1;
                    } else {
                        /* Down-pointing triangle has dirs: 1, 3, 5 */
                        switch (dir) {
                            case 0: dest->z--; break;           /* UpRight */
                            case 1: dest->x--; break;           /* Left */
                            case 2: dest->z++; break;           /* DownLeft */
                        }
                        /* Inverse directions for down-pointing triangles */
                        *inverse_dir = (dir == 0) ? 2 : (dir == 1) ? 1 : 0;
                    }
                }
                break;
        }
    } else if (dir == base_dirs) {
        /* Up direction */
        dest->z++;
        *inverse_dir = base_dirs + 1;  /* Down */
    } else if (dir == base_dirs + 1) {
        /* Down direction */
        dest->z--;
        *inverse_dir = base_dirs;  /* Up */
    } else {
        return false;
    }
    
    /* Check if destination is in grid */
    if (!prism_is_cell_in_grid(grid, *dest)) {
        return false;
    }
    
    /* Set connection (no rotation/reflection for prism movements) */
    if (connection) {
        connection->is_mirror = false;
        connection->rotation = 0;
    }
    
    return true;
}

static int prism_get_cell_dirs(const SylvesGrid* grid, SylvesCell cell,
                              SylvesCellDir* dirs, size_t max_dirs) {
    if (!prism_is_cell_in_grid(grid, cell)) {
        return SYLVES_ERROR_INVALID_CELL;
    }
    
    PrismGridData* data = (PrismGridData*)grid->data;
    
    /* Get base direction count */
    int base_dirs = 0;
    switch (data->base_type) {
        case SYLVES_GRID_TYPE_SQUARE:
            base_dirs = 4;
            break;
        case SYLVES_GRID_TYPE_HEX:
            base_dirs = 6;
            break;
        case SYLVES_GRID_TYPE_TRIANGLE:
            base_dirs = 3;
            break;
        default:
            return -1;  /* Invalid grid type */
    }
    
    int total_dirs = base_dirs + 2;  /* Base dirs + up/down */
    
    if (!dirs) {
        return total_dirs;
    }
    
    int count = 0;
    for (int i = 0; i < total_dirs && count < (int)max_dirs; i++) {
        dirs[count++] = i;
    }
    
    return count;
}

static SylvesError prism_get_mesh_data(const SylvesGrid* grid, SylvesCell cell,
                                       SylvesMeshData** mesh_data) {
    if (!prism_is_cell_in_grid(grid, cell)) {
        return SYLVES_ERROR_INVALID_CELL;
    }
    
    PrismGridData* data = (PrismGridData*)grid->data;
    
    /* Get base corner count */
    int base_corners = 0;
    switch (data->base_type) {
        case SYLVES_GRID_TYPE_SQUARE:
            base_corners = 4;
            break;
        case SYLVES_GRID_TYPE_HEX:
            base_corners = 6;
            break;
        case SYLVES_GRID_TYPE_TRIANGLE:
            base_corners = 3;
            break;
        default:
            return SYLVES_ERROR_INVALID_ARGUMENT;
    }
    
    /* Allocate mesh data */
    *mesh_data = (SylvesMeshData*)calloc(1, sizeof(SylvesMeshData));
    if (!*mesh_data) {
        return SYLVES_ERROR_OUT_OF_MEMORY;
    }
    
    /* Prism has base_corners * 2 vertices */
    int vertex_count = base_corners * 2;
    (*mesh_data)->vertices = (SylvesVector3*)malloc(sizeof(SylvesVector3) * vertex_count);
    (*mesh_data)->normals = (SylvesVector3*)malloc(sizeof(SylvesVector3) * vertex_count);
    if (!(*mesh_data)->vertices || !(*mesh_data)->normals) {
        sylves_mesh_data_free(*mesh_data);
        return SYLVES_ERROR_OUT_OF_MEMORY;
    }
    (*mesh_data)->vertex_count = vertex_count;
    
    /* Get all corner positions */
    for (int i = 0; i < vertex_count; i++) {
        (*mesh_data)->vertices[i] = prism_get_cell_corner_pos(grid, cell, i);
    }
    
    /* TODO: Properly implement face-based mesh generation instead of indices */
    /* For now, just create an empty face array */
    int face_count = 2 + base_corners;
    (*mesh_data)->faces = (SylvesMeshFace*)calloc(face_count, sizeof(SylvesMeshFace));
    if (!(*mesh_data)->faces) {
        sylves_mesh_data_free(*mesh_data);
        return SYLVES_ERROR_OUT_OF_MEMORY;
    }
    (*mesh_data)->face_count = face_count;
    
    /* Calculate normals */
    /* Initialize all normals to zero */
    for (int i = 0; i < vertex_count; i++) {
        (*mesh_data)->normals[i] = (SylvesVector3){0, 0, 0};
    }
    
    /* TODO: Calculate normals based on faces instead of indices */
    
    /* Normalize vertex normals */
    for (int i = 0; i < vertex_count; i++) {
        (*mesh_data)->normals[i] = sylves_vector3_normalize((*mesh_data)->normals[i]);
    }
    
    return SYLVES_SUCCESS;
}

static SylvesVector3 prism_get_cell_corner_pos(const SylvesGrid* grid, SylvesCell cell,
                                               SylvesCellCorner corner) {
    PrismGridData* data = (PrismGridData*)grid->data;
    SylvesVector3 pos = {0, 0, 0};
    
    /* Get base corner count */
    int base_corners = 0;
    switch (data->base_type) {
        case SYLVES_GRID_TYPE_SQUARE:
            base_corners = 4;
            break;
        case SYLVES_GRID_TYPE_HEX:
            base_corners = 6;
            break;
        case SYLVES_GRID_TYPE_TRIANGLE:
            base_corners = 3;
            break;
        default:
            return pos;
    }
    
    /* Determine if corner is on top or bottom face */
    bool is_top = (corner >= base_corners);
    int base_corner = corner % base_corners;
    
    /* Get base position based on grid type */
    switch (data->base_type) {
        case SYLVES_GRID_TYPE_SQUARE:
            /* Square corners: 0=BottomLeft, 1=BottomRight, 2=TopRight, 3=TopLeft */
            switch (base_corner) {
                case 0: pos.x = cell.x * data->cell_size; pos.y = cell.y * data->cell_size; break;
                case 1: pos.x = (cell.x + 1) * data->cell_size; pos.y = cell.y * data->cell_size; break;
                case 2: pos.x = (cell.x + 1) * data->cell_size; pos.y = (cell.y + 1) * data->cell_size; break;
                case 3: pos.x = cell.x * data->cell_size; pos.y = (cell.y + 1) * data->cell_size; break;
            }
            break;
            
        case SYLVES_GRID_TYPE_HEX:
            /* Hex corners */
            {
                double angle = base_corner * M_PI / 3.0;
                if (data->flat_topped) {
                    angle += M_PI / 6.0;  /* 30 degree offset for flat-topped */
                }
                SylvesVector3 center = prism_get_cell_center(grid, cell);
                pos.x = center.x + data->cell_size * 0.5 * cos(angle);
                pos.y = center.y + data->cell_size * 0.5 * sin(angle);
            }
            break;
            
        case SYLVES_GRID_TYPE_TRIANGLE:
            /* Triangle corners */
            {
                /* Get triangle center first */
                SylvesVector3 center = prism_get_cell_center(grid, cell);
                bool is_upward = ((cell.x + cell.y + cell.z) == 1);
                
                /* Triangle corner offsets from center */
                double sqrt3_6 = 0.28867513459481288; /* sqrt(3)/6 */
                double sqrt3_3 = 0.57735026918962576; /* sqrt(3)/3 */
                
                if (is_upward) {
                    /* Upward pointing triangle */
                    switch (base_corner) {
                        case 0: /* Bottom left */
                            pos.x = center.x - 0.5 * data->cell_size;
                            pos.y = center.y - sqrt3_6 * data->cell_size;
                            break;
                        case 1: /* Bottom right */
                            pos.x = center.x + 0.5 * data->cell_size;
                            pos.y = center.y - sqrt3_6 * data->cell_size;
                            break;
                        case 2: /* Top */
                            pos.x = center.x;
                            pos.y = center.y + sqrt3_3 * data->cell_size;
                            break;
                    }
                } else {
                    /* Downward pointing triangle */
                    switch (base_corner) {
                        case 0: /* Top left */
                            pos.x = center.x - 0.5 * data->cell_size;
                            pos.y = center.y + sqrt3_6 * data->cell_size;
                            break;
                        case 1: /* Top right */
                            pos.x = center.x + 0.5 * data->cell_size;
                            pos.y = center.y + sqrt3_6 * data->cell_size;
                            break;
                        case 2: /* Bottom */
                            pos.x = center.x;
                            pos.y = center.y - sqrt3_3 * data->cell_size;
                            break;
                    }
                }
            }
            break;
    }
    
    /* Set Z coordinate based on top/bottom */
    pos.z = is_top ? (cell.z + 1) * data->layer_height : cell.z * data->layer_height;
    
    return pos;
}

static SylvesError prism_get_cell_aabb(const SylvesGrid* grid, SylvesCell cell, SylvesAabb* aabb) {
    if (!prism_is_cell_in_grid(grid, cell)) {
        return SYLVES_ERROR_INVALID_CELL;
    }
    
    PrismGridData* data = (PrismGridData*)grid->data;
    (void)data; /* Currently unused */
    
    /* Initialize with first corner */
    SylvesVector3 corner = prism_get_cell_corner_pos(grid, cell, 0);
    aabb->min = corner;
    aabb->max = corner;
    
    /* Expand to include all corners */
    int num_corners = prism_get_cell_corners(grid, cell, NULL, 0);
    for (int i = 1; i < num_corners; i++) {
        corner = prism_get_cell_corner_pos(grid, cell, i);
        if (corner.x < aabb->min.x) aabb->min.x = corner.x;
        if (corner.y < aabb->min.y) aabb->min.y = corner.y;
        if (corner.z < aabb->min.z) aabb->min.z = corner.z;
        if (corner.x > aabb->max.x) aabb->max.x = corner.x;
        if (corner.y > aabb->max.y) aabb->max.y = corner.y;
        if (corner.z > aabb->max.z) aabb->max.z = corner.z;
    }
    
    return SYLVES_SUCCESS;
}

static bool prism_find_cell(const SylvesGrid* grid, SylvesVector3 position, SylvesCell* cell) {
    PrismGridData* data = (PrismGridData*)grid->data;
    
    /* Find layer */
    cell->z = (int)floor(position.z / data->layer_height);
    
    /* Find base cell based on grid type */
    switch (data->base_type) {
        case SYLVES_GRID_TYPE_SQUARE:
            cell->x = (int)floor(position.x / data->cell_size);
            cell->y = (int)floor(position.y / data->cell_size);
            break;
            
        case SYLVES_GRID_TYPE_HEX:
            /* Hex grid find cell using proper cube coordinate rounding */
            {
                double sx = data->flat_topped ? data->cell_size : data->cell_size * 0.8660254037844386;
                double sy = data->flat_topped ? data->cell_size * 0.8660254037844386 : data->cell_size;
                double px = position.x, py = position.y;
                double qf, rf;
                
                if (data->flat_topped) {
                    /* Flat-topped hex */
                    double q_est = px / (0.75 * sx);
                    double r_est = (py / sy) - 0.5 * q_est;
                    qf = q_est; rf = r_est;
                } else {
                    /* Pointy-topped hex */
                    double r_est = py / (0.75 * sy);
                    double q_est = (px / sx) - 0.5 * r_est;
                    qf = q_est; rf = r_est;
                }
                
                /* Convert to cube and round */
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
                cell->x = rx;
                cell->y = rz;
            }
            break;
            
        case SYLVES_GRID_TYPE_TRIANGLE:
            /* Triangle grid find cell using proper FTri coordinate system */
            {
                /* Flat-topped triangles (FTri) */
                double x = position.x / data->cell_size;
                double y = position.y / data->cell_size;
                cell->x = (int)ceil(x - 0.5 * y);
                cell->y = (int)floor(y) + 1;
                cell->z = (int)ceil(-x - 0.5 * y);
            }
            break;
            
        default:
            return false;
    }
    
    return prism_is_cell_in_grid(grid, *cell);
}

static int prism_get_cell_corners(const SylvesGrid* grid, SylvesCell cell,
                                 SylvesCellCorner* corners, size_t max_corners) {
    if (!prism_is_cell_in_grid(grid, cell)) {
        return SYLVES_ERROR_INVALID_CELL;
    }
    
    PrismGridData* data = (PrismGridData*)grid->data;
    
    /* Get base corner count */
    int base_corners = 0;
    switch (data->base_type) {
        case SYLVES_GRID_TYPE_SQUARE:
            base_corners = 4;
            break;
        case SYLVES_GRID_TYPE_HEX:
            base_corners = 6;
            break;
        case SYLVES_GRID_TYPE_TRIANGLE:
            base_corners = 3;
            break;
        default:
            return -1;  /* Invalid grid type */
    }
    
    int total_corners = base_corners * 2;  /* Top and bottom faces */
    
    if (!corners) {
        return total_corners;
    }
    
    int count = 0;
    for (int i = 0; i < total_corners && count < (int)max_corners; i++) {
        corners[count++] = i;
    }
    
    return count;
}

