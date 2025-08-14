/**
 * @file grid.h
 * @brief Grid interface and operations
 */

#ifndef SYLVES_GRID_H
#define SYLVES_GRID_H

#include "types.h"
#include "errors.h"


/**
 * @brief Grid type enumeration
 */
typedef enum {
    SYLVES_GRID_TYPE_SQUARE,
    SYLVES_GRID_TYPE_HEX,
    SYLVES_GRID_TYPE_TRIANGLE,
    SYLVES_GRID_TYPE_CUBE,
    SYLVES_GRID_TYPE_MESH,
    SYLVES_GRID_TYPE_MODIFIER,
    SYLVES_GRID_TYPE_CUSTOM
} SylvesGridType;

/* Grid creation and destruction */

/**
 * @brief Destroy a grid and free all associated memory
 * @param grid The grid to destroy
 */
void sylves_grid_destroy(SylvesGrid* grid);

/* Grid properties */

/**
 * @brief Get the type of a grid
 * @param grid The grid
 * @return Grid type
 */
SylvesGridType sylves_grid_get_type(const SylvesGrid* grid);

/**
 * @brief Check if grid uses 2D cell types
 * @param grid The grid
 * @return true if 2D, false otherwise
 */
bool sylves_grid_is_2d(const SylvesGrid* grid);

/**
 * @brief Check if grid uses 3D cell types
 * @param grid The grid
 * @return true if 3D, false otherwise
 */
bool sylves_grid_is_3d(const SylvesGrid* grid);

/**
 * @brief Check if grid is planar (2D cells in XY plane)
 * @param grid The grid
 * @return true if planar, false otherwise
 */
bool sylves_grid_is_planar(const SylvesGrid* grid);

/**
 * @brief Check if grid is repeating (fixed pattern)
 * @param grid The grid
 * @return true if repeating, false otherwise
 */
bool sylves_grid_is_repeating(const SylvesGrid* grid);

/**
 * @brief Check if grid is orientable (no mirroring)
 * @param grid The grid
 * @return true if orientable, false otherwise
 */
bool sylves_grid_is_orientable(const SylvesGrid* grid);

/**
 * @brief Check if grid has finite number of cells
 * @param grid The grid
 * @return true if finite, false otherwise
 */
bool sylves_grid_is_finite(const SylvesGrid* grid);

/**
 * @brief Get coordinate dimension (1, 2, or 3)
 * @param grid The grid
 * @return Coordinate dimension
 */
int sylves_grid_get_coordinate_dimension(const SylvesGrid* grid);

/* Cell operations */

/**
 * @brief Check if a cell is in the grid
 * @param grid The grid
 * @param cell The cell to check
 * @return true if cell is in grid, false otherwise
 */
bool sylves_grid_is_cell_in_grid(const SylvesGrid* grid, SylvesCell cell);

/**
 * @brief Get the cell type for a specific cell
 * @param grid The grid
 * @param cell The cell
 * @return Cell type, or NULL if cell not in grid
 */
const SylvesCellType* sylves_grid_get_cell_type(const SylvesGrid* grid, SylvesCell cell);

/**
 * @brief Get all cells in the grid
 * @param grid The grid
 * @param cells Output array (must be allocated by caller)
 * @param max_cells Maximum number of cells to return
 * @return Number of cells written, or negative error code
 */
int sylves_grid_get_cells(const SylvesGrid* grid, SylvesCell* cells, size_t max_cells);

/**
 * @brief Get the number of cells in the grid
 * @param grid The grid
 * @return Number of cells, or negative error code for infinite grids
 */
int sylves_grid_get_cell_count(const SylvesGrid* grid);

/* Topology */

/**
 * @brief Try to move from one cell to another in a direction
 * @param grid The grid
 * @param cell Starting cell
 * @param dir Direction to move
 * @param dest Output: destination cell
 * @param inverse_dir Output: direction back from dest to cell
 * @param connection Output: connection information
 * @return true if move successful, false otherwise
 */
bool sylves_grid_try_move(const SylvesGrid* grid, SylvesCell cell, SylvesCellDir dir,
                          SylvesCell* dest, SylvesCellDir* inverse_dir, 
                          SylvesConnection* connection);

/**
 * @brief Get all valid directions from a cell
 * @param grid The grid
 * @param cell The cell
 * @param dirs Output array for directions
 * @param max_dirs Maximum number of directions to return
 * @return Number of directions, or negative error code
 */
int sylves_grid_get_cell_dirs(const SylvesGrid* grid, SylvesCell cell,
                              SylvesCellDir* dirs, size_t max_dirs);

/**
 * @brief Get all corners of a cell
 * @param grid The grid
 * @param cell The cell
 * @param corners Output array for corners
 * @param max_corners Maximum number of corners to return
 * @return Number of corners, or negative error code
 */
int sylves_grid_get_cell_corners(const SylvesGrid* grid, SylvesCell cell,
                                 SylvesCellCorner* corners, size_t max_corners);

/**
 * @brief Find a basic path between two cells
 * @param grid The grid
 * @param start Starting cell
 * @param dest Destination cell
 * @param path Output array for path cells
 * @param dirs Output array for directions (optional)
 * @param max_steps Maximum path length
 * @return Path length, or negative error code
 */
int sylves_grid_find_basic_path(const SylvesGrid* grid, SylvesCell start, SylvesCell dest,
                                SylvesCell* path, SylvesCellDir* dirs, size_t max_steps);

/* Position and shape */

/**
 * @brief Get the center position of a cell
 * @param grid The grid
 * @param cell The cell
 * @return Cell center position
 */
SylvesVector3 sylves_grid_get_cell_center(const SylvesGrid* grid, SylvesCell cell);

/**
 * @brief Get the position of a cell corner
 * @param grid The grid
 * @param cell The cell
 * @param corner The corner
 * @return Corner position
 */
SylvesVector3 sylves_grid_get_cell_corner(const SylvesGrid* grid, SylvesCell cell,
                                          SylvesCellCorner corner);

/**
 * @brief Get the transform (TRS) for a cell
 * @param grid The grid
 * @param cell The cell
 * @param trs Output: transform
 * @return Error code
 */
SylvesError sylves_grid_get_trs(const SylvesGrid* grid, SylvesCell cell, SylvesTRS* trs);

/**
 * @brief Get polygon vertices for a 2D cell
 * @param grid The grid
 * @param cell The cell
 * @param vertices Output array for vertices
 * @param max_vertices Maximum number of vertices
 * @return Number of vertices, or negative error code
 */
int sylves_grid_get_polygon(const SylvesGrid* grid, SylvesCell cell,
                            SylvesVector3* vertices, size_t max_vertices);

/**
 * @brief Get mesh data for a 3D cell
 * @param grid The grid
 * @param cell The cell
 * @param mesh_data Output: mesh data (caller must free)
 * @return Error code
 */
SylvesError sylves_grid_get_mesh_data(const SylvesGrid* grid, SylvesCell cell,
                                      SylvesMeshData** mesh_data);

/**
 * @brief Free mesh data allocated by get_mesh_data
 * @param mesh_data The mesh data to free
 */
void sylves_mesh_data_free(SylvesMeshData* mesh_data);

/**
 * @brief Get axis-aligned bounding box for a cell
 * @param grid The grid
 * @param cell The cell
 * @param aabb Output: bounding box
 * @return Error code
 */
SylvesError sylves_grid_get_cell_aabb(const SylvesGrid* grid, SylvesCell cell,
                                      SylvesAabb* aabb);

/* Queries */

/**
 * @brief Find the cell containing a position
 * @param grid The grid
 * @param position The position to query
 * @param cell Output: the cell containing the position
 * @return true if cell found, false otherwise
 */
bool sylves_grid_find_cell(const SylvesGrid* grid, SylvesVector3 position,
                           SylvesCell* cell);

/**
 * @brief Find cell from a transformation matrix
 * @param grid The grid
 * @param matrix The transformation matrix
 * @param cell Output: the cell
 * @param rotation Output: the rotation
 * @return true if found, false otherwise
 */
bool sylves_grid_find_cell_from_matrix(const SylvesGrid* grid, const SylvesMatrix4x4* matrix,
                                       SylvesCell* cell, SylvesCellRotation* rotation);

/**
 * @brief Get cells that potentially overlap a bounding box
 * @param grid The grid
 * @param min Minimum corner of box
 * @param max Maximum corner of box
 * @param cells Output array for cells
 * @param max_cells Maximum number of cells to return
 * @return Number of cells, or negative error code
 */
int sylves_grid_get_cells_in_aabb(const SylvesGrid* grid, SylvesVector3 min, SylvesVector3 max,
                                  SylvesCell* cells, size_t max_cells);

/**
 * @brief Cast a ray through the grid
 * @param grid The grid
 * @param origin Ray origin
 * @param direction Ray direction (normalized)
 * @param max_distance Maximum ray distance
 * @param hits Output array for hit information
 * @param max_hits Maximum number of hits to return
 * @return Number of hits, or negative error code
 */
int sylves_grid_raycast(const SylvesGrid* grid, SylvesVector3 origin, SylvesVector3 direction,
                       double max_distance, SylvesRaycastInfo* hits, size_t max_hits);

/* Bounds */

/**
 * @brief Get the current bound of the grid
 * @param grid The grid
 * @return The bound, or NULL if unbounded
 */
const SylvesBound* sylves_grid_get_bound(const SylvesGrid* grid);

/**
 * @brief Create a new grid with a bound applied
 * @param grid The original grid
 * @param bound The bound to apply
 * @return New bounded grid (must be freed), or NULL on error
 */
SylvesGrid* sylves_grid_bound_by(const SylvesGrid* grid, const SylvesBound* bound);

/**
 * @brief Get unbounded version of grid
 * @param grid The grid
 * @return Unbounded grid (must be freed), or NULL on error
 */
SylvesGrid* sylves_grid_unbounded(const SylvesGrid* grid);

/* Grid relationships */

/**
 * @brief Get the dual grid
 * @param grid The grid
 * @return Dual grid (must be freed), or NULL on error
 */
SylvesGrid* sylves_grid_get_dual(const SylvesGrid* grid);

/**
 * @brief Get diagonal grid (with diagonal connections)
 * @param grid The grid
 * @return Diagonal grid (must be freed), or NULL on error
 */
SylvesGrid* sylves_grid_get_diagonal(const SylvesGrid* grid);

/* Index operations */

/**
 * @brief Get maximum index value
 * @param grid The grid
 * @return Maximum index + 1, or negative error code
 */
int sylves_grid_get_index_count(const SylvesGrid* grid);

/**
 * @brief Get index for a cell
 * @param grid The grid
 * @param cell The cell
 * @return Cell index, or negative error code
 */
int sylves_grid_get_index(const SylvesGrid* grid, SylvesCell cell);

/**
 * @brief Get cell from index
 * @param grid The grid
 * @param index The index
 * @param cell Output: the cell
 * @return Error code
 */
SylvesError sylves_grid_get_cell_by_index(const SylvesGrid* grid, int index, SylvesCell* cell);


#endif /* SYLVES_GRID_H */
