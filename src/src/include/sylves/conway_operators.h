/**
 * @file conway_operators.h
 * @brief Conway operators for mesh manipulation
 */

#ifndef SYLVES_CONWAY_OPERATORS_H
#define SYLVES_CONWAY_OPERATORS_H

#include "sylves/mesh_data.h"
#include "sylves/errors.h"

/**
 * @brief Apply Conway operator "kis" to the mesh
 * 
 * Adds a vertex at the center of each face, connecting to all vertices of the face.
 * 
 * @param mesh The mesh to transform
 * @return New mesh with "kis" operator applied
 */
SylvesMeshDataEx* sylves_conway_kis(const SylvesMeshDataEx* mesh);

/**
 * @brief Apply Conway operator "truncate" to the mesh
 * 
 * Truncates the corners of the mesh, creating a new face at each vertex.
 * 
 * @param mesh The mesh to transform
 * @return New mesh with "truncate" operator applied
 */
SylvesMeshDataEx* sylves_conway_truncate(const SylvesMeshDataEx* mesh);

/**
 * @brief Apply Conway operator "dual" to the mesh
 * 
 * Generates the dual of the mesh.
 * 
 * @param mesh The mesh to transform
 * @return New dual mesh
 */
SylvesMeshDataEx* sylves_conway_dual(const SylvesMeshDataEx* mesh);

#endif /* SYLVES_CONWAY_OPERATORS_H */

