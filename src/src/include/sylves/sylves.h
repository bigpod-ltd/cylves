/**
 * @file sylves.h
 * @brief Main header file for the Sylves C library
 * 
 * This header includes all public APIs of the Sylves library.
 * Include this single header to access all Sylves functionality.
 */

#ifndef SYLVES_H
#define SYLVES_H

#include "export.h"

// Core headers
#include "errors.h"
#include "types.h"
#include "vector.h"
#include "matrix.h"
#include "bounds.h"

// Grid system
#include "cell.h"
#include "cell_type.h"
#include "grid.h"
#include "connection.h"

// Specific grid types
#include "square_grid.h"
#include "hex_grid.h"
#include "cube_grid.h"
#include "triangle_grid.h"

// Mesh and geometry
#include "mesh.h"
#include "polygon.h"
#include "deformation.h"

// Grid modifiers
#include "grid_modifier.h"
#include "transform_modifier.h"
#include "mask_modifier.h"
#include "wrap_modifier.h"
#include "bijection_modifier.h"
#include "relaxation_modifier.h"
#include "nested_modifier.h"
#include "ravel_modifier.h"
#include "planar_prism_modifier.h"

// Algorithms
#include "voronoi.h"
#include "delaunay.h"
#include "pathfinding.h"

// Utilities
#include "utils.h"


#endif /* SYLVES_H */
