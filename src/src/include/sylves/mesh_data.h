/**
 * @file mesh_data.h
 * @brief Comprehensive mesh data structures and operations
 */

#ifndef SYLVES_MESH_DATA_H
#define SYLVES_MESH_DATA_H

#include "types.h"
#include "errors.h"
#include "vector.h"
#include "matrix.h"

/**
 * @brief Mesh topology types
 */
typedef enum {
    SYLVES_MESH_TOPOLOGY_TRIANGLES = 0,  /**< Triangle list */
    SYLVES_MESH_TOPOLOGY_QUADS = 2,      /**< Quad list */
    SYLVES_MESH_TOPOLOGY_NGON = -1       /**< N-gon with inverted last index */
} SylvesMeshTopology;

/**
 * @brief Submesh data
 */
typedef struct {
    int* indices;               /**< Index array */
    size_t index_count;         /**< Number of indices */
    SylvesMeshTopology topology;/**< Topology type */
} SylvesSubmesh;

/**
 * @brief Enhanced mesh data structure
 */
typedef struct {
    /* Core geometry */
    SylvesVector3* vertices;    /**< Vertex positions */
    size_t vertex_count;        /**< Number of vertices */
    
    /* Submeshes */
    SylvesSubmesh* submeshes;   /**< Submesh array */
    size_t submesh_count;       /**< Number of submeshes */
    
    /* Optional attributes */
    SylvesVector3* normals;     /**< Vertex normals */
    SylvesVector2* uvs;         /**< Texture coordinates */
    SylvesVector4* tangents;    /**< Tangent vectors (xyz) with handedness (w) */
    
    /* Topology information (computed on demand) */
    void* edge_data;            /**< Edge connectivity data */
    void* face_data;            /**< Face adjacency data */
} SylvesMeshDataEx;

/**
 * @brief Edge information
 */
typedef struct {
    int v0, v1;                 /**< Vertex indices */
    int f0, f1;                 /**< Adjacent face indices (-1 if boundary) */
    int e0, e1;                 /**< Edge index within each face */
} SylvesMeshEdge;

/**
 * @brief Face iterator
 */
typedef struct {
    const int* indices;         /**< Current index pointer */
    size_t remaining;           /**< Remaining indices */
    SylvesMeshTopology topology;/**< Current topology */
    int face_vertices[32];      /**< Current face vertices */
    int vertex_count;           /**< Vertices in current face */
} SylvesFaceIterator;

/* Creation and destruction */
SylvesMeshDataEx* sylves_mesh_data_ex_create(size_t vertex_count, size_t submesh_count);
void sylves_mesh_data_ex_destroy(SylvesMeshDataEx* mesh);
SylvesMeshDataEx* sylves_mesh_data_ex_clone(const SylvesMeshDataEx* mesh);

/* Submesh management */
SylvesError sylves_mesh_data_ex_set_submesh(
    SylvesMeshDataEx* mesh,
    size_t submesh_index,
    const int* indices,
    size_t index_count,
    SylvesMeshTopology topology);

/* Attribute management */
SylvesError sylves_mesh_data_ex_allocate_normals(SylvesMeshDataEx* mesh);
SylvesError sylves_mesh_data_ex_allocate_uvs(SylvesMeshDataEx* mesh);
SylvesError sylves_mesh_data_ex_allocate_tangents(SylvesMeshDataEx* mesh);

/* Normal calculation */
void sylves_mesh_data_ex_recalculate_normals(SylvesMeshDataEx* mesh);
void sylves_mesh_data_ex_recalculate_tangents(SylvesMeshDataEx* mesh);

/* Topology operations */
SylvesMeshDataEx* sylves_mesh_data_ex_triangulate(const SylvesMeshDataEx* mesh);
SylvesMeshDataEx* sylves_mesh_data_ex_invert_winding(const SylvesMeshDataEx* mesh);

/* Transformation */
SylvesMeshDataEx* sylves_mesh_data_ex_transform(
    const SylvesMeshDataEx* mesh,
    const SylvesMatrix4x4* transform);

/* Face iteration */
void sylves_face_iterator_init(
    SylvesFaceIterator* iter,
    const SylvesMeshDataEx* mesh,
    size_t submesh);

bool sylves_face_iterator_next(SylvesFaceIterator* iter);

/* Validation */
bool sylves_mesh_data_ex_validate(const SylvesMeshDataEx* mesh);
bool sylves_mesh_data_ex_is_manifold(const SylvesMeshDataEx* mesh);
bool sylves_mesh_data_ex_is_closed(const SylvesMeshDataEx* mesh);
bool sylves_mesh_data_ex_has_consistent_winding(const SylvesMeshDataEx* mesh);

/* Edge topology */
SylvesError sylves_mesh_data_ex_build_edges(SylvesMeshDataEx* mesh);
size_t sylves_mesh_data_ex_get_edge_count(const SylvesMeshDataEx* mesh);
const SylvesMeshEdge* sylves_mesh_data_ex_get_edges(const SylvesMeshDataEx* mesh);

/* Mesh statistics */
void sylves_mesh_data_ex_get_bounds(
    const SylvesMeshDataEx* mesh,
    SylvesVector3* min,
    SylvesVector3* max);

double sylves_mesh_data_ex_get_surface_area(const SylvesMeshDataEx* mesh);
double sylves_mesh_data_ex_get_volume(const SylvesMeshDataEx* mesh);

/* Conversion from simple mesh data */
SylvesMeshDataEx* sylves_mesh_data_to_ex(const SylvesMeshData* simple);
SylvesMeshData* sylves_mesh_data_ex_to_simple(const SylvesMeshDataEx* ex);

#endif /* SYLVES_MESH_DATA_H */
