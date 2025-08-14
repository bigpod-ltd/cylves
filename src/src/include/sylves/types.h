/**
 * @file types.h
 * @brief Basic type definitions for the Sylves library
 */

#ifndef SYLVES_TYPES_H
#define SYLVES_TYPES_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>


/* Forward declarations */
typedef struct SylvesGrid SylvesGrid;
typedef struct SylvesCellType SylvesCellType;
typedef struct SylvesBound SylvesBound;
typedef struct SylvesMesh SylvesMesh;
typedef struct SylvesDeformation SylvesDeformation;

/**
 * @brief Represents a single cell in a grid
 * 
 * Cells are identified by integer coordinates.
 * The meaning of the coordinates depends on the grid type.
 */
typedef struct {
    int x;  /**< X coordinate */
    int y;  /**< Y coordinate */
    int z;  /**< Z coordinate (0 for 2D grids) */
} SylvesCell;

/**
 * @brief 3D vector with double precision
 */
typedef struct {
    double x;  /**< X component */
    double y;  /**< Y component */
    double z;  /**< Z component */
} SylvesVector3;

/**
 * @brief 3D integer vector
 */
typedef struct {
    int x;  /**< X component */
    int y;  /**< Y component */
    int z;  /**< Z component */
} SylvesVector3Int;

/**
 * @brief 2D vector with double precision
 */
typedef struct {
    double x;  /**< X component */
    double y;  /**< Y component */
} SylvesVector2;

/**
 * @brief 4D vector with double precision
 */
typedef struct {
    double x;  /**< X component */
    double y;  /**< Y component */
    double z;  /**< Z component */
    double w;  /**< W component */
} SylvesVector4;

/**
 * @brief 4x4 transformation matrix
 * 
 * Stored in column-major order for compatibility with OpenGL.
 * Access element (row, col) as m[col * 4 + row].
 */
typedef struct {
    double m[16];  /**< Matrix elements in column-major order */
} SylvesMatrix4x4;

/**
 * @brief Axis-aligned bounding box
 */
typedef struct {
    SylvesVector3 min;  /**< Minimum corner */
    SylvesVector3 max;  /**< Maximum corner */
} SylvesAabb;

/**
 * @brief Transform with position, rotation, and scale
 */
typedef struct {
    SylvesVector3 position;     /**< Translation */
    SylvesMatrix4x4 rotation;   /**< Rotation matrix */
    SylvesVector3 scale;        /**< Scale factors */
} SylvesTRS;

/**
 * @brief Represents a direction from a cell (edge in 2D, face in 3D)
 * 
 * The actual values depend on the cell type.
 * Use cell type functions to work with directions.
 */
typedef int SylvesCellDir;

/**
 * @brief Represents a corner of a cell
 * 
 * The actual values depend on the cell type.
 * Use cell type functions to work with corners.
 */
typedef int SylvesCellCorner;

/**
 * @brief Represents a rotation/reflection of a cell
 * 
 * The actual values depend on the cell type.
 * Includes both rotations and reflections.
 */
typedef int SylvesCellRotation;

/**
 * @brief Describes how cell-local space relates between adjacent cells
 */
typedef struct {
    SylvesCellRotation rotation;  /**< Rotation between cells */
    bool is_mirror;               /**< Whether connection involves reflection */
} SylvesConnection;

/**
 * @brief Result of a raycast operation
 */
typedef struct {
    SylvesCell cell;        /**< Cell that was hit */
    double distance;        /**< Distance to hit point */
    SylvesVector3 point;    /**< Hit point in world space */
    SylvesCellDir face;     /**< Face that was hit (3D only) */
} SylvesRaycastInfo;

/**
 * @brief Face info for mesh grid
 */
typedef struct {
    int* vertices;      /**< Vertex indices for this face */
    int vertex_count;   /**< Number of vertices in face (3 for triangle, 4 for quad, etc) */
    int* neighbors;     /**< Neighbor face indices (-1 for boundary) */
} SylvesMeshFace;

/**
 * @brief Mesh data for a cell or grid
 */
typedef struct {
    SylvesVector3* vertices;    /**< Vertex positions */
    size_t vertex_count;        /**< Number of vertices */
    SylvesMeshFace* faces;      /**< Face data */
    size_t face_count;          /**< Number of faces */
    SylvesVector3* normals;     /**< Vertex normals (optional) */
    SylvesVector2* uvs;         /**< Texture coordinates (optional) */
} SylvesMeshData;

/**
 * @brief Represents a symmetry operation on a grid
 */
typedef struct {
    SylvesCellRotation rotation;    /**< Rotation component */
    SylvesVector3Int translation;   /**< Translation component */
} SylvesGridSymmetry;

/**
 * @brief Options for path finding
 */
typedef struct {
    double max_distance;        /**< Maximum path distance (0 = unlimited) */
    size_t max_steps;          /**< Maximum number of steps (0 = unlimited) */
    bool allow_diagonal;       /**< Allow diagonal movement */
    void* weight_function;     /**< Custom weight function (optional) */
    void* user_data;           /**< User data for weight function */
} SylvesPathOptions;


#endif /* SYLVES_TYPES_H */
