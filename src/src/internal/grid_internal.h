/**
 * @file grid_internal.h
 * @brief Internal grid structure and vtable
 */

#ifndef GRID_INTERNAL_H
#define GRID_INTERNAL_H

#include "sylves/types.h"
#include "sylves/errors.h"
#include "sylves/grid.h"

/* Virtual function table for grid operations */
typedef struct {
    /* Destructor */
    void (*destroy)(SylvesGrid* grid);
    
    /* Properties */
    bool (*is_2d)(const SylvesGrid* grid);
    bool (*is_3d)(const SylvesGrid* grid);
    bool (*is_planar)(const SylvesGrid* grid);
    bool (*is_repeating)(const SylvesGrid* grid);
    bool (*is_orientable)(const SylvesGrid* grid);
    bool (*is_finite)(const SylvesGrid* grid);
    int (*get_coordinate_dimension)(const SylvesGrid* grid);
    
    /* Cell operations */
    bool (*is_cell_in_grid)(const SylvesGrid* grid, SylvesCell cell);
    const SylvesCellType* (*get_cell_type)(const SylvesGrid* grid, SylvesCell cell);
    
    /* Topology */
    bool (*try_move)(const SylvesGrid* grid, SylvesCell cell, SylvesCellDir dir,
                     SylvesCell* dest, SylvesCellDir* inverse_dir, SylvesConnection* connection);
    int (*get_cell_dirs)(const SylvesGrid* grid, SylvesCell cell,
                        SylvesCellDir* dirs, size_t max_dirs);
    int (*get_cell_corners)(const SylvesGrid* grid, SylvesCell cell,
                           SylvesCellCorner* corners, size_t max_corners);
    
    /* Position */
    SylvesVector3 (*get_cell_center)(const SylvesGrid* grid, SylvesCell cell);
    SylvesVector3 (*get_cell_corner_pos)(const SylvesGrid* grid, SylvesCell cell,
                                         SylvesCellCorner corner);
    
    /* Shape */
    int (*get_polygon)(const SylvesGrid* grid, SylvesCell cell,
                      SylvesVector3* vertices, size_t max_vertices);
    SylvesError (*get_cell_aabb)(const SylvesGrid* grid, SylvesCell cell, SylvesAabb* aabb);
    
    /* Queries */
    bool (*find_cell)(const SylvesGrid* grid, SylvesVector3 position, SylvesCell* cell);
    /* Raycast */
    int (*raycast)(const SylvesGrid* grid, SylvesVector3 origin, SylvesVector3 direction,
                   double max_distance, SylvesRaycastInfo* hits, size_t max_hits);
    
    /* Index */
    int (*get_index_count)(const SylvesGrid* grid);
    int (*get_index)(const SylvesGrid* grid, SylvesCell cell);
    SylvesError (*get_cell_by_index)(const SylvesGrid* grid, int index, SylvesCell* cell);
} SylvesGridVTable;

/* Base grid structure */
struct SylvesGrid {
    const SylvesGridVTable* vtable;
    SylvesGridType type;
    const SylvesBound* bound;
    void* data;  /* Grid-specific data */
};

/* Helper macros for vtable calls */
#define GRID_VTABLE(grid) ((grid)->vtable)
#define GRID_CALL(grid, method, ...) \
    (GRID_VTABLE(grid)->method ? GRID_VTABLE(grid)->method((grid), ##__VA_ARGS__) : 0)

#endif /* GRID_INTERNAL_H */
