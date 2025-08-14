/**
 * @file planar_lazy_mesh_grid.c
 * @brief Implementation of planar lazy mesh grid
 */

#include "sylves/planar_lazy_mesh_grid.h"
#include "sylves/mesh.h"
#include "sylves/grid.h"
#include "sylves/memory.h"
#include "sylves/errors.h"
#include "sylves/cell.h"
#include "sylves/hash.h"
#include "internal/grid_internal.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Hash table entry for cached chunks */
typedef struct ChunkEntry {
    SylvesCell chunk_cell;           /* Chunk coordinates */
    SylvesMeshData* mesh_data;       /* Mesh data for this chunk */
    SylvesGrid* mesh_grid;           /* Mesh grid for this chunk */
    struct ChunkEntry* next;         /* Next entry in hash chain */
} ChunkEntry;

/* Planar lazy mesh grid structure */
typedef struct {
    SylvesGrid base;                 /* Base grid structure */
    
    /* Chunk generation */
    SylvesGetMeshDataFunc get_mesh_data;
    void* user_data;
    
    /* Chunk layout */
    SylvesVector2 stride_x;          /* X stride between chunks */
    SylvesVector2 stride_y;          /* Y stride between chunks */
    SylvesVector2 aabb_min;          /* Min corner of chunk bounding box */
    SylvesVector2 aabb_max;          /* Max corner of chunk bounding box */
    bool translate_mesh_data;        /* Whether to translate mesh coordinates */
    
    /* Options */
    SylvesMeshGridOptions options;
    SylvesCachePolicy cache_policy;
    
    /* Cache */
    ChunkEntry** chunk_cache;        /* Hash table for cached chunks */
    size_t cache_size;               /* Size of hash table */
    size_t cache_count;              /* Number of cached chunks */
    size_t cache_max;                /* Maximum cached chunks (for LRU) */
} PlanarLazyMeshGrid;

/* Forward declarations */
static void planar_lazy_destroy(SylvesGrid* grid);
static bool planar_lazy_is_cell_in_grid(const SylvesGrid* grid, SylvesCell cell);
static SylvesVector3 planar_lazy_get_cell_center(const SylvesGrid* grid, SylvesCell cell);
static int planar_lazy_get_polygon(const SylvesGrid* grid, SylvesCell cell,
                                   SylvesVector3* vertices, size_t max_vertices);
static bool planar_lazy_try_move(const SylvesGrid* grid, SylvesCell cell, SylvesCellDir dir,
                                 SylvesCell* dest, SylvesCellDir* inverse_dir, 
                                 SylvesConnection* connection);

/* VTable */
static const SylvesGridVTable planar_lazy_vtable = {
    .destroy = planar_lazy_destroy,
    .is_2d = NULL,
    .is_3d = NULL,
    .is_planar = NULL,
    .is_repeating = NULL,
    .is_orientable = NULL,
    .is_finite = NULL,
    .get_coordinate_dimension = NULL,
    .is_cell_in_grid = planar_lazy_is_cell_in_grid,
    .get_cell_type = NULL,
    .try_move = planar_lazy_try_move,
    .get_cell_dirs = NULL,
    .get_cell_corners = NULL,
    .get_cell_center = planar_lazy_get_cell_center,
    .get_cell_corner_pos = NULL,
    .get_polygon = planar_lazy_get_polygon,
    .get_cell_aabb = NULL,
    .find_cell = NULL,
    .raycast = NULL,
    .get_index_count = NULL,
    .get_index = NULL,
    .get_cell_by_index = NULL
};

/* Helper: Split a global cell into chunk and local cell within chunk */
static void split_cell(const PlanarLazyMeshGrid* grid, SylvesCell cell,
                      SylvesCell* chunk_cell, SylvesCell* local_cell) {
    /* For simplicity, use a fixed chunk size in cells */
    /* Real implementation would map based on mesh bounds */
    int chunk_size = 10;  /* 10x10 cells per chunk */
    
    if (chunk_cell) {
        chunk_cell->x = cell.x / chunk_size;
        chunk_cell->y = cell.y / chunk_size;
        chunk_cell->z = 0;
    }
    
    if (local_cell) {
        local_cell->x = cell.x % chunk_size;
        local_cell->y = cell.y % chunk_size;
        local_cell->z = cell.z;
    }
}

/* Helper: Combine chunk and local cell into global cell */
static SylvesCell combine_cells(const PlanarLazyMeshGrid* grid,
                                SylvesCell chunk_cell, SylvesCell local_cell) {
    int chunk_size = 10;
    return sylves_cell_create(
        chunk_cell.x * chunk_size + local_cell.x,
        chunk_cell.y * chunk_size + local_cell.y,
        local_cell.z
    );
}

/* Helper: Get or create mesh grid for a chunk */
static SylvesGrid* get_chunk_grid(PlanarLazyMeshGrid* grid, SylvesCell chunk_cell) {
    /* Check cache first */
    if (grid->cache_policy != SYLVES_CACHE_NONE && grid->chunk_cache) {
        /* Simple hash based on chunk coordinates */
        size_t hash = ((size_t)chunk_cell.x * 73856093) ^ 
                     ((size_t)chunk_cell.y * 19349663);
        size_t bucket = hash % grid->cache_size;
        
        ChunkEntry* entry = grid->chunk_cache[bucket];
        while (entry) {
            if (sylves_cell_equals(entry->chunk_cell, chunk_cell)) {
                return entry->mesh_grid;  /* Found in cache */
            }
            entry = entry->next;
        }
    }
    
    /* Generate mesh data for chunk */
    SylvesMeshData* mesh_data = grid->get_mesh_data(
        chunk_cell.x, chunk_cell.y, grid->user_data);
    
    if (!mesh_data) {
        return NULL;  /* Failed to generate mesh */
    }
    
    /* Optionally translate mesh data */
    if (grid->translate_mesh_data) {
        SylvesVector2 offset = {
            chunk_cell.x * grid->stride_x.x + chunk_cell.y * grid->stride_y.x,
            chunk_cell.x * grid->stride_x.y + chunk_cell.y * grid->stride_y.y
        };
        
        for (size_t i = 0; i < mesh_data->vertex_count; i++) {
            mesh_data->vertices[i].x += offset.x;
            mesh_data->vertices[i].y += offset.y;
        }
    }
    
    /* Compute adjacency if requested */
    if (grid->options.compute_adjacency) {
        sylves_mesh_compute_adjacency(mesh_data);
    }
    
    /* Create mesh grid */
    SylvesGrid* mesh_grid = sylves_mesh_grid_create(mesh_data);
    
    /* Cache the result */
    if (grid->cache_policy != SYLVES_CACHE_NONE && grid->chunk_cache && mesh_grid) {
        size_t hash = ((size_t)chunk_cell.x * 73856093) ^ 
                     ((size_t)chunk_cell.y * 19349663);
        size_t bucket = hash % grid->cache_size;
        
        ChunkEntry* entry = sylves_alloc(sizeof(ChunkEntry));
        if (entry) {
            entry->chunk_cell = chunk_cell;
            entry->mesh_data = mesh_data;
            entry->mesh_grid = mesh_grid;
            entry->next = grid->chunk_cache[bucket];
            grid->chunk_cache[bucket] = entry;
            grid->cache_count++;
            
            /* TODO: Implement LRU eviction for SYLVES_CACHE_LRU */
        }
    } else {
        /* Not caching, so clean up mesh data */
        sylves_mesh_data_destroy(mesh_data);
    }
    
    return mesh_grid;
}

/* Implementation of vtable functions */
static void planar_lazy_destroy(SylvesGrid* grid) {
    if (!grid || !grid->data) return;
    
    PlanarLazyMeshGrid* plmg = (PlanarLazyMeshGrid*)grid->data;
    
    /* Clean up cache */
    if (plmg->chunk_cache) {
        for (size_t i = 0; i < plmg->cache_size; i++) {
            ChunkEntry* entry = plmg->chunk_cache[i];
            while (entry) {
                ChunkEntry* next = entry->next;
                if (entry->mesh_grid) {
                    entry->mesh_grid->vtable->destroy(entry->mesh_grid);
                }
                if (entry->mesh_data) {
                    sylves_mesh_data_destroy(entry->mesh_data);
                }
                sylves_free(entry);
                entry = next;
            }
        }
        sylves_free(plmg->chunk_cache);
    }
    
    sylves_free(plmg);
    sylves_free(grid);
}

static bool planar_lazy_is_cell_in_grid(const SylvesGrid* grid, SylvesCell cell) {
    PlanarLazyMeshGrid* plmg = (PlanarLazyMeshGrid*)grid->data;
    
    /* Check if chunk is within bounds if bounded */
    if (grid->bound) {
        /* TODO: Check against bound */
    }
    
    /* Get chunk for this cell */
    SylvesCell chunk_cell, local_cell;
    split_cell(plmg, cell, &chunk_cell, &local_cell);
    
    /* Get or create chunk grid */
    SylvesGrid* chunk_grid = get_chunk_grid(plmg, chunk_cell);
    if (!chunk_grid) {
        return false;
    }
    
    /* Check if local cell is in chunk grid */
    return chunk_grid->vtable->is_cell_in_grid(chunk_grid, local_cell);
}

static SylvesVector3 planar_lazy_get_cell_center(const SylvesGrid* grid, SylvesCell cell) {
    PlanarLazyMeshGrid* plmg = (PlanarLazyMeshGrid*)grid->data;
    
    SylvesCell chunk_cell, local_cell;
    split_cell(plmg, cell, &chunk_cell, &local_cell);
    
    SylvesGrid* chunk_grid = get_chunk_grid(plmg, chunk_cell);
    if (!chunk_grid) {
        return (SylvesVector3){0, 0, 0};
    }
    
    SylvesVector3 local_center = chunk_grid->vtable->get_cell_center(chunk_grid, local_cell);
    
    /* Add chunk offset if not already translated */
    if (!plmg->translate_mesh_data) {
        SylvesVector2 offset = {
            chunk_cell.x * plmg->stride_x.x + chunk_cell.y * plmg->stride_y.x,
            chunk_cell.x * plmg->stride_x.y + chunk_cell.y * plmg->stride_y.y
        };
        local_center.x += offset.x;
        local_center.y += offset.y;
    }
    
    return local_center;
}

static int planar_lazy_get_polygon(const SylvesGrid* grid, SylvesCell cell,
                                   SylvesVector3* vertices, size_t max_vertices) {
    PlanarLazyMeshGrid* plmg = (PlanarLazyMeshGrid*)grid->data;
    
    SylvesCell chunk_cell, local_cell;
    split_cell(plmg, cell, &chunk_cell, &local_cell);
    
    SylvesGrid* chunk_grid = get_chunk_grid(plmg, chunk_cell);
    if (!chunk_grid) {
        return -1;
    }
    
    int count = chunk_grid->vtable->get_polygon(chunk_grid, local_cell, vertices, max_vertices);
    
    /* Add chunk offset if not already translated */
    if (!plmg->translate_mesh_data && vertices && count > 0) {
        SylvesVector2 offset = {
            chunk_cell.x * plmg->stride_x.x + chunk_cell.y * plmg->stride_y.x,
            chunk_cell.x * plmg->stride_x.y + chunk_cell.y * plmg->stride_y.y
        };
        
        for (int i = 0; i < count && i < max_vertices; i++) {
            vertices[i].x += offset.x;
            vertices[i].y += offset.y;
        }
    }
    
    return count;
}

static bool planar_lazy_try_move(const SylvesGrid* grid, SylvesCell cell, SylvesCellDir dir,
                                 SylvesCell* dest, SylvesCellDir* inverse_dir, 
                                 SylvesConnection* connection) {
    PlanarLazyMeshGrid* plmg = (PlanarLazyMeshGrid*)grid->data;
    
    SylvesCell chunk_cell, local_cell;
    split_cell(plmg, cell, &chunk_cell, &local_cell);
    
    SylvesGrid* chunk_grid = get_chunk_grid(plmg, chunk_cell);
    if (!chunk_grid) {
        return false;
    }
    
    /* Try move within chunk first */
    SylvesCell local_dest;
    if (chunk_grid->vtable->try_move(chunk_grid, local_cell, dir, 
                                     &local_dest, inverse_dir, connection)) {
        /* Move succeeded within chunk */
        if (dest) {
            *dest = combine_cells(plmg, chunk_cell, local_dest);
        }
        return true;
    }
    
    /* TODO: Handle moves across chunk boundaries */
    /* This requires checking adjacent chunks and matching edges */
    
    return false;
}

/* Public API implementation */
void sylves_mesh_grid_options_init(SylvesMeshGridOptions* options) {
    if (!options) return;
    
    options->validate_mesh = true;
    options->compute_adjacency = true;
    options->allow_non_manifold = false;
    options->max_vertices_per_face = 0;  /* Unlimited */
}

SylvesGrid* sylves_planar_lazy_mesh_grid_create(
    SylvesGetMeshDataFunc get_mesh_data,
    SylvesVector2 stride_x,
    SylvesVector2 stride_y,
    SylvesVector2 aabb_min,
    SylvesVector2 aabb_max,
    bool translate_mesh_data,
    const SylvesMeshGridOptions* options,
    const SylvesBound* bound,
    SylvesCachePolicy cache_policy,
    void* user_data) {
    
    if (!get_mesh_data) {
        return NULL;
    }
    
    PlanarLazyMeshGrid* plmg = sylves_alloc(sizeof(PlanarLazyMeshGrid));
    if (!plmg) {
        return NULL;
    }
    
    /* Initialize fields */
    plmg->get_mesh_data = get_mesh_data;
    plmg->user_data = user_data;
    plmg->stride_x = stride_x;
    plmg->stride_y = stride_y;
    plmg->aabb_min = aabb_min;
    plmg->aabb_max = aabb_max;
    plmg->translate_mesh_data = translate_mesh_data;
    plmg->cache_policy = cache_policy;
    
    /* Copy or init options */
    if (options) {
        plmg->options = *options;
    } else {
        sylves_mesh_grid_options_init(&plmg->options);
    }
    
    /* Initialize cache */
    if (cache_policy != SYLVES_CACHE_NONE) {
        plmg->cache_size = 256;  /* Fixed size hash table */
        plmg->chunk_cache = sylves_alloc(sizeof(ChunkEntry*) * plmg->cache_size);
        if (plmg->chunk_cache) {
            memset(plmg->chunk_cache, 0, sizeof(ChunkEntry*) * plmg->cache_size);
        }
        plmg->cache_count = 0;
        plmg->cache_max = (cache_policy == SYLVES_CACHE_LRU) ? 100 : SIZE_MAX;
    }
    
    /* Create grid */
    SylvesGrid* grid = sylves_alloc(sizeof(SylvesGrid));
    if (!grid) {
        sylves_free(plmg->chunk_cache);
        sylves_free(plmg);
        return NULL;
    }
    
    grid->vtable = &planar_lazy_vtable;
    grid->type = SYLVES_GRID_TYPE_MESH;  /* Could add PLANAR_LAZY type */
    grid->bound = bound;
    grid->data = plmg;
    
    plmg->base = *grid;
    
    return grid;
}

SylvesGrid* sylves_planar_lazy_mesh_grid_create_square(
    SylvesGetMeshDataFunc get_mesh_data,
    double chunk_size,
    double margin,
    bool translate_mesh_data,
    const SylvesMeshGridOptions* options,
    const SylvesBound* bound,
    SylvesCachePolicy cache_policy,
    void* user_data) {
    
    SylvesVector2 stride_x = {chunk_size, 0};
    SylvesVector2 stride_y = {0, chunk_size};
    SylvesVector2 aabb_min = {-margin, -margin};
    SylvesVector2 aabb_max = {chunk_size + margin, chunk_size + margin};
    
    return sylves_planar_lazy_mesh_grid_create(
        get_mesh_data, stride_x, stride_y, aabb_min, aabb_max,
        translate_mesh_data, options, bound, cache_policy, user_data);
}

SylvesGrid* sylves_planar_lazy_mesh_grid_create_hex(
    SylvesGetMeshDataFunc get_mesh_data,
    double hex_size,
    double margin,
    bool translate_mesh_data,
    const SylvesMeshGridOptions* options,
    const SylvesBound* bound,
    SylvesCachePolicy cache_policy,
    void* user_data) {
    
    /* Hexagonal chunk layout */
    double sqrt3 = sqrt(3.0);
    SylvesVector2 stride_x = {hex_size * 3.0 / 2.0, 0};
    SylvesVector2 stride_y = {hex_size * 3.0 / 4.0, hex_size * sqrt3 / 2.0};
    SylvesVector2 aabb_min = {-hex_size - margin, -hex_size - margin};
    SylvesVector2 aabb_max = {hex_size + margin, hex_size + margin};
    
    return sylves_planar_lazy_mesh_grid_create(
        get_mesh_data, stride_x, stride_y, aabb_min, aabb_max,
        translate_mesh_data, options, bound, cache_policy, user_data);
}
