/**
 * @file registry.h
 * @brief Factory and registry helpers for consistent implementations
 */

#ifndef SYLVES_REGISTRY_H
#define SYLVES_REGISTRY_H

#include "types.h"
#include "grid.h"


/* Grid factory helpers */

/**
 * @brief Factory function type for creating grids
 */
typedef SylvesGrid* (*SylvesGridFactory)(void* config);

/**
 * @brief Structure for describing a grid implementation
 */
typedef struct {
    const char* name;           /**< Display name (e.g., "square", "hex") */
    SylvesGridType type;        /**< Enum type value */
    SylvesGridFactory factory;  /**< Factory function */
    bool is_2d;                 /**< Quick 2D check */
    bool is_3d;                 /**< Quick 3D check */
} SylvesGridDescriptor;

/* Cell type factory helpers */

/**
 * @brief Factory function type for creating cell types
 */
typedef SylvesCellType* (*SylvesCellTypeFactory)(void* config);

/**
 * @brief Structure for describing a cell type implementation
 */
typedef struct {
    const char* name;                   /**< Display name (e.g., "square", "hex") */
    int dimension;                      /**< 2D or 3D */
    SylvesCellTypeFactory factory;      /**< Factory function */
} SylvesCellTypeDescriptor;

/* Bound factory helpers */

/**
 * @brief Factory function type for creating bounds
 */
typedef SylvesBound* (*SylvesBoundFactory)(void* config);

/**
 * @brief Structure for describing a bound implementation
 */
typedef struct {
    const char* name;                   /**< Display name (e.g., "rectangle", "cube") */
    SylvesBoundFactory factory;         /**< Factory function */
} SylvesBoundDescriptor;

/* Registry API */

/**
 * @brief Register a new grid implementation
 * @param desc Descriptor for the grid
 * @return 0 on success, negative on error
 */
int sylves_registry_add_grid(const SylvesGridDescriptor* desc);

/**
 * @brief Get a grid descriptor by name
 * @param name Name of the grid type
 * @return Descriptor, or NULL if not found
 */
const SylvesGridDescriptor* sylves_registry_get_grid_desc(const char* name);

/**
 * @brief Register a new cell type implementation
 */
int sylves_registry_add_cell_type(const SylvesCellTypeDescriptor* desc);

/**
 * @brief Get a cell type descriptor by name
 */
const SylvesCellTypeDescriptor* sylves_registry_get_cell_type_desc(const char* name);

/**
 * @brief Register a new bound implementation
 */
int sylves_registry_add_bound(const SylvesBoundDescriptor* desc);

/**
 * @brief Get a bound descriptor by name
 */
const SylvesBoundDescriptor* sylves_registry_get_bound_desc(const char* name);

/**
 * @brief Initialize the registry with built-in types
 * @return 0 on success
 */
int sylves_registry_init(void);

/**
 * @brief Clean up the registry
 */
void sylves_registry_cleanup(void);


#endif /* SYLVES_REGISTRY_H */
