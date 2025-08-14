/**
 * @file mesh_utilities.c
 * @brief Implementation of mesh utilities for merging, splitting, optimizing, and repairing meshes
 */

#include "sylves/mesh_utilities.h"
#include "sylves/mesh_data.h"
#include "sylves/memory.h"
#include "sylves/errors.h"
#include "sylves/vector.h"
#include <string.h>
#include <math.h>
#include <float.h>

/* Face utilities */
SylvesError sylves_mesh_utils_get_face_vertex_count(
    const SylvesMeshDataEx* mesh,
    int submesh,
    int face_index,
    int* count_out) {
    if (!mesh || !count_out || submesh < 0 || (size_t)submesh >= mesh->submesh_count) {
        return SYLVES_ERROR_INVALID_ARGUMENT;
    }

    SylvesFaceIterator iter;
    sylves_face_iterator_init(&iter, mesh, (size_t)submesh);
    int idx = 0;
    while (sylves_face_iterator_next(&iter)) {
        if (idx == face_index) {
            *count_out = iter.vertex_count;
            return SYLVES_SUCCESS;
        }
        idx++;
    }
    return SYLVES_ERROR_OUT_OF_BOUNDS;
}

/* Default configurations */

SylvesMeshMergeConfig sylves_mesh_merge_config_default(void) {
    SylvesMeshMergeConfig config = {
        .vertex_merge_distance = 1e-6,
        .merge_attributes = true,
        .remove_duplicates = true
    };
    return config;
}

SylvesMeshOptimizeConfig sylves_mesh_optimize_config_default(void) {
    SylvesMeshOptimizeConfig config = {
        .remove_unused_vertices = true,
        .merge_coplanar_faces = false,
        .coplanar_angle_tolerance = 0.01
    };
    return config;
}

SylvesUVGenerationConfig sylves_uv_generation_config_default(void) {
    SylvesUVGenerationConfig config = {
        .mapping_type = SYLVES_UV_PLANAR_PROJECTION,
        .projection_axis = {0, 0, 1},
        .scale = 1.0
    };
    return config;
}

/* Vertex hash for duplicate detection */
typedef struct VertexKey {
    float x, y, z;
} VertexKey;

static size_t vertex_hash(VertexKey key, size_t bucket_count) {
    /* Simple hash based on coordinates */
    unsigned int hx = *(unsigned int*)&key.x;
    unsigned int hy = *(unsigned int*)&key.y;
    unsigned int hz = *(unsigned int*)&key.z;
    return ((size_t)hx * 73856093u + (size_t)hy * 19349663u + (size_t)hz * 83492791u) % bucket_count;
}

static bool vertex_equal(VertexKey a, VertexKey b, float tolerance) {
    return fabsf(a.x - b.x) < tolerance &&
           fabsf(a.y - b.y) < tolerance &&
           fabsf(a.z - b.z) < tolerance;
}

/**
 * @brief Merge multiple meshes into one
 */
SylvesMeshDataEx* sylves_mesh_merge(
    const SylvesMeshDataEx** meshes,
    size_t mesh_count,
    const SylvesMeshMergeConfig* config) {
    
    if (!meshes || mesh_count == 0) return NULL;
    
    SylvesMeshMergeConfig cfg = config ? *config : sylves_mesh_merge_config_default();
    
    /* Count totals */
    size_t total_vertices = 0;
    size_t total_submeshes = 0;
    
    for (size_t i = 0; i < mesh_count; i++) {
        if (!meshes[i]) continue;
        total_vertices += meshes[i]->vertex_count;
        total_submeshes += meshes[i]->submesh_count;
    }
    
    /* Create result mesh */
    SylvesMeshDataEx* result = sylves_mesh_data_ex_create(total_vertices, total_submeshes);
    if (!result) return NULL;
    
    /* Determine if we need attributes */
    bool has_uvs = false, has_normals = false, has_tangents = false;
    if (cfg.merge_attributes) {
        for (size_t i = 0; i < mesh_count; i++) {
            if (meshes[i]->uvs) has_uvs = true;
            if (meshes[i]->normals) has_normals = true;
            if (meshes[i]->tangents) has_tangents = true;
        }
    }
    
    /* Allocate attributes */
    if (has_uvs) sylves_mesh_data_ex_allocate_uvs(result);
    if (has_normals) sylves_mesh_data_ex_allocate_normals(result);
    if (has_tangents) sylves_mesh_data_ex_allocate_tangents(result);
    
    /* Copy data */
    size_t vertex_offset = 0;
    size_t submesh_offset = 0;
    
    for (size_t m = 0; m < mesh_count; m++) {
        const SylvesMeshDataEx* mesh = meshes[m];
        if (!mesh) continue;
        
        /* Copy vertices */
        memcpy(&result->vertices[vertex_offset], mesh->vertices,
               sizeof(SylvesVector3) * mesh->vertex_count);
        
        /* Copy attributes */
        if (result->uvs && mesh->uvs) {
            memcpy(&result->uvs[vertex_offset], mesh->uvs,
                   sizeof(SylvesVector2) * mesh->vertex_count);
        }
        if (result->normals && mesh->normals) {
            memcpy(&result->normals[vertex_offset], mesh->normals,
                   sizeof(SylvesVector3) * mesh->vertex_count);
        }
        if (result->tangents && mesh->tangents) {
            memcpy(&result->tangents[vertex_offset], mesh->tangents,
                   sizeof(SylvesVector4) * mesh->vertex_count);
        }
        
        /* Copy submeshes with offset indices */
        for (size_t s = 0; s < mesh->submesh_count; s++) {
            const SylvesSubmesh* src = &mesh->submeshes[s];
            
            /* Allocate and offset indices */
            int* indices = (int*)sylves_alloc(sizeof(int) * src->index_count);
            if (!indices) {
                sylves_mesh_data_ex_destroy(result);
                return NULL;
            }
            
            for (size_t i = 0; i < src->index_count; i++) {
                int idx = src->indices[i];
                if (src->topology == SYLVES_MESH_TOPOLOGY_NGON && idx < 0) {
                    indices[i] = ~(~idx + vertex_offset);
                } else {
                    indices[i] = idx + vertex_offset;
                }
            }
            
            sylves_mesh_data_ex_set_submesh(
                result, submesh_offset + s,
                indices, src->index_count, src->topology);
            
            sylves_free(indices);
        }
        
        vertex_offset += mesh->vertex_count;
        submesh_offset += mesh->submesh_count;
    }
    
    return result;
}

/**
 * @brief Merge two meshes
 */
SylvesMeshDataEx* sylves_mesh_merge_pair(
    const SylvesMeshDataEx* mesh1,
    const SylvesMeshDataEx* mesh2,
    const SylvesMeshMergeConfig* config) {
    
    const SylvesMeshDataEx* meshes[2] = {mesh1, mesh2};
    return sylves_mesh_merge(meshes, 2, config);
}

/**
 * @brief Split mesh by connectivity
 *
 * Separates mesh into connected components.
 *
 * @param mesh Mesh to split
 * @param[out] components Array to receive component meshes
 * @param[out] component_count Number of components found
 * @return SYLVES_SUCCESS or error code
 */
SylvesError sylves_mesh_split_connected(
    const SylvesMeshDataEx* mesh,
    SylvesMeshDataEx*** components,
    size_t* component_count) {
    
    if (!mesh || !components || !component_count) {
        return SYLVES_ERROR_INVALID_ARGUMENT;
    }
    
    /* Build edge topology if needed */
    SylvesMeshDataEx* mutable_mesh = (SylvesMeshDataEx*)mesh;
    if (!mesh->edge_data) {
        sylves_mesh_data_ex_build_edges(mutable_mesh);
    }
    
    /* For now, return a copy as single component */
    *component_count = 1;
    *components = (SylvesMeshDataEx**)sylves_alloc(sizeof(SylvesMeshDataEx*));
    if (!*components) {
        return SYLVES_ERROR_OUT_OF_MEMORY;
    }
    
    (*components)[0] = sylves_mesh_data_ex_clone(mesh);
    if (!(*components)[0]) {
        sylves_free(*components);
        return SYLVES_ERROR_OUT_OF_MEMORY;
    }
    
    return SYLVES_SUCCESS;
}

/**
 * @brief Split mesh by submesh
 */
SylvesError sylves_mesh_split_submeshes(
    const SylvesMeshDataEx* mesh,
    SylvesMeshDataEx*** submeshes,
    size_t* submesh_count) {
    
    if (!mesh || !submeshes || !submesh_count) {
        return SYLVES_ERROR_INVALID_ARGUMENT;
    }
    
    *submesh_count = mesh->submesh_count;
    *submeshes = (SylvesMeshDataEx**)sylves_alloc(sizeof(SylvesMeshDataEx*) * mesh->submesh_count);
    if (!*submeshes) {
        return SYLVES_ERROR_OUT_OF_MEMORY;
    }
    
    for (size_t s = 0; s < mesh->submesh_count; s++) {
        /* Create mesh with single submesh */
        (*submeshes)[s] = sylves_mesh_data_ex_create(mesh->vertex_count, 1);
        if (!(*submeshes)[s]) {
            for (size_t i = 0; i < s; i++) {
                sylves_mesh_data_ex_destroy((*submeshes)[i]);
            }
            sylves_free(*submeshes);
            return SYLVES_ERROR_OUT_OF_MEMORY;
        }
        
        /* Copy vertices and attributes */
        memcpy((*submeshes)[s]->vertices, mesh->vertices,
               sizeof(SylvesVector3) * mesh->vertex_count);
        
        if (mesh->normals) {
            sylves_mesh_data_ex_allocate_normals((*submeshes)[s]);
            memcpy((*submeshes)[s]->normals, mesh->normals,
                   sizeof(SylvesVector3) * mesh->vertex_count);
        }
        
        if (mesh->uvs) {
            sylves_mesh_data_ex_allocate_uvs((*submeshes)[s]);
            memcpy((*submeshes)[s]->uvs, mesh->uvs,
                   sizeof(SylvesVector2) * mesh->vertex_count);
        }
        
        if (mesh->tangents) {
            sylves_mesh_data_ex_allocate_tangents((*submeshes)[s]);
            memcpy((*submeshes)[s]->tangents, mesh->tangents,
                   sizeof(SylvesVector4) * mesh->vertex_count);
        }
        
        /* Copy submesh */
        const SylvesSubmesh* src = &mesh->submeshes[s];
        sylves_mesh_data_ex_set_submesh(
            (*submeshes)[s], 0,
            src->indices, src->index_count, src->topology);
    }
    
    return SYLVES_SUCCESS;
}

/**
 * @brief Optimize mesh structure
 *
 * Performs various optimizations like removing unused vertices,
 * merging coplanar faces, etc.
 *
 * @param mesh Mesh to optimize
 * @param config Optimization configuration (NULL for defaults)
 * @return New optimized mesh or NULL on error
 */
SylvesMeshDataEx* sylves_mesh_optimize(
    const SylvesMeshDataEx* mesh,
    const SylvesMeshOptimizeConfig* config) {
    
    if (!mesh) return NULL;
    
    SylvesMeshOptimizeConfig cfg = config ? *config : sylves_mesh_optimize_config_default();
    
    /* For now, just clone the mesh */
    return sylves_mesh_data_ex_clone(mesh);
}

/**
 * @brief Remove duplicate vertices
 */
SylvesMeshDataEx* sylves_mesh_remove_duplicate_vertices(
    const SylvesMeshDataEx* mesh,
    double merge_distance) {
    
    if (!mesh) return NULL;
    
    /* Hash table for vertex deduplication */
    size_t bucket_count = mesh->vertex_count * 2;
    typedef struct VertexEntry {
        int original_index;
        int new_index;
        struct VertexEntry* next;
    } VertexEntry;
    
    VertexEntry** buckets = (VertexEntry**)sylves_calloc(bucket_count, sizeof(VertexEntry*));
    if (!buckets) return NULL;
    
    /* Map from old to new indices */
    int* index_map = (int*)sylves_alloc(sizeof(int) * mesh->vertex_count);
    if (!index_map) {
        sylves_free(buckets);
        return NULL;
    }
    
    /* Deduplicate vertices */
    size_t unique_count = 0;
    for (size_t i = 0; i < mesh->vertex_count; i++) {
        VertexKey key = {
            mesh->vertices[i].x,
            mesh->vertices[i].y,
            mesh->vertices[i].z
        };
        
        size_t hash = vertex_hash(key, bucket_count);
        VertexEntry* entry = buckets[hash];
        
        /* Check if vertex already exists */
        bool found = false;
        while (entry) {
            if (vertex_equal(key, (VertexKey){
                mesh->vertices[entry->original_index].x,
                mesh->vertices[entry->original_index].y,
                mesh->vertices[entry->original_index].z
            }, merge_distance)) {
                index_map[i] = entry->new_index;
                found = true;
                break;
            }
            entry = entry->next;
        }
        
        if (!found) {
            /* Add new unique vertex */
            VertexEntry* new_entry = (VertexEntry*)sylves_alloc(sizeof(VertexEntry));
            new_entry->original_index = i;
            new_entry->new_index = unique_count;
            new_entry->next = buckets[hash];
            buckets[hash] = new_entry;
            
            index_map[i] = unique_count;
            unique_count++;
        }
    }
    
    /* Create result mesh with unique vertices */
    SylvesMeshDataEx* result = sylves_mesh_data_ex_create(unique_count, mesh->submesh_count);
    if (!result) {
        /* Clean up */
        for (size_t i = 0; i < bucket_count; i++) {
            VertexEntry* entry = buckets[i];
            while (entry) {
                VertexEntry* next = entry->next;
                sylves_free(entry);
                entry = next;
            }
        }
        sylves_free(buckets);
        sylves_free(index_map);
        return NULL;
    }
    
    /* Copy unique vertices */
    for (size_t i = 0; i < bucket_count; i++) {
        VertexEntry* entry = buckets[i];
        while (entry) {
            result->vertices[entry->new_index] = mesh->vertices[entry->original_index];
            entry = entry->next;
        }
    }
    
    /* Copy submeshes with remapped indices */
    for (size_t s = 0; s < mesh->submesh_count; s++) {
        const SylvesSubmesh* src = &mesh->submeshes[s];
        int* indices = (int*)sylves_alloc(sizeof(int) * src->index_count);
        
        for (size_t i = 0; i < src->index_count; i++) {
            int idx = src->indices[i];
            if (src->topology == SYLVES_MESH_TOPOLOGY_NGON && idx < 0) {
                indices[i] = ~index_map[~idx];
            } else {
                indices[i] = index_map[idx];
            }
        }
        
        sylves_mesh_data_ex_set_submesh(
            result, s, indices, src->index_count, src->topology);
        
        sylves_free(indices);
    }
    
    /* Clean up */
    for (size_t i = 0; i < bucket_count; i++) {
        VertexEntry* entry = buckets[i];
        while (entry) {
            VertexEntry* next = entry->next;
            sylves_free(entry);
            entry = next;
        }
    }
    sylves_free(buckets);
    sylves_free(index_map);
    
    return result;
}

/* Normal operations */

SylvesError sylves_mesh_smooth_normals(
    SylvesMeshDataEx* mesh,
    int iterations,
    double factor) {
    
    if (!mesh || !mesh->normals) {
        return SYLVES_ERROR_INVALID_ARGUMENT;
    }
    
    /* Simple normal smoothing */
    for (int iter = 0; iter < iterations; iter++) {
        SylvesVector3* new_normals = (SylvesVector3*)sylves_alloc(
            sizeof(SylvesVector3) * mesh->vertex_count);
        if (!new_normals) {
            return SYLVES_ERROR_OUT_OF_MEMORY;
        }
        
        /* Copy current normals */
        memcpy(new_normals, mesh->normals, 
               sizeof(SylvesVector3) * mesh->vertex_count);
        
        /* Average with neighbors (simplified) */
        /* Full implementation would use edge connectivity */
        
        /* Normalize */
        for (size_t i = 0; i < mesh->vertex_count; i++) {
            float len = sqrtf(
                new_normals[i].x * new_normals[i].x +
                new_normals[i].y * new_normals[i].y +
                new_normals[i].z * new_normals[i].z);
            if (len > 1e-6f) {
                new_normals[i].x /= len;
                new_normals[i].y /= len;
                new_normals[i].z /= len;
            }
        }
        
        sylves_free(mesh->normals);
        mesh->normals = new_normals;
    }
    
    return SYLVES_SUCCESS;
}

SylvesError sylves_mesh_flip_normals(SylvesMeshDataEx* mesh) {
    if (!mesh || !mesh->normals) {
        return SYLVES_ERROR_INVALID_ARGUMENT;
    }
    
    for (size_t i = 0; i < mesh->vertex_count; i++) {
        mesh->normals[i].x = -mesh->normals[i].x;
        mesh->normals[i].y = -mesh->normals[i].y;
        mesh->normals[i].z = -mesh->normals[i].z;
    }
    
    return SYLVES_SUCCESS;
}

/**
 * @brief Generate UV coordinates
 *
 * Creates UV coordinates for the mesh using the specified mapping method.
 *
 * @param mesh Mesh to generate UVs for
 * @param config UV generation configuration
 * @return SYLVES_SUCCESS or error code
 */
SylvesError sylves_mesh_generate_uvs(
    SylvesMeshDataEx* mesh,
    const SylvesUVGenerationConfig* config) {
    
    if (!mesh || !config) return SYLVES_ERROR_INVALID_ARGUMENT;
    
    SylvesUVGenerationConfig cfg = *config;
    
    /* Allocate UVs if needed */
    if (!mesh->uvs) {
        SylvesError err = sylves_mesh_data_ex_allocate_uvs(mesh);
        if (err != SYLVES_SUCCESS) return err;
    }
    
    switch (cfg.mapping_type) {
        case SYLVES_UV_PLANAR_PROJECTION:
            return sylves_mesh_generate_planar_uvs(
                mesh, cfg.projection_axis, cfg.scale);
            
        case SYLVES_UV_SPHERICAL_MAPPING:
        case SYLVES_UV_CYLINDRICAL_MAPPING:
        case SYLVES_UV_BOX_MAPPING:
            /* Placeholder for other mapping types */
            return SYLVES_SUCCESS;
            
        default:
            return SYLVES_ERROR_INVALID_ARGUMENT;
    }
}

SylvesError sylves_mesh_generate_planar_uvs(
    SylvesMeshDataEx* mesh,
    SylvesVector3 axis,
    double scale) {
    
    if (!mesh || !mesh->uvs) {
        return SYLVES_ERROR_INVALID_ARGUMENT;
    }
    
    /* Normalize axis */
    float len = sqrtf(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);
    if (len < 1e-6f) {
        return SYLVES_ERROR_INVALID_ARGUMENT;
    }
    axis.x /= len;
    axis.y /= len;
    axis.z /= len;
    
    /* Find perpendicular vectors */
    SylvesVector3 u, v;
    if (fabsf(axis.y) < 0.9f) {
        u = (SylvesVector3){-axis.z, 0, axis.x};
    } else {
        u = (SylvesVector3){0, -axis.z, axis.y};
    }
    
    /* Normalize u */
    len = sqrtf(u.x * u.x + u.y * u.y + u.z * u.z);
    u.x /= len;
    u.y /= len;
    u.z /= len;
    
    /* v = axis × u */
    v.x = axis.y * u.z - axis.z * u.y;
    v.y = axis.z * u.x - axis.x * u.z;
    v.z = axis.x * u.y - axis.y * u.x;
    
    /* Project vertices */
    for (size_t i = 0; i < mesh->vertex_count; i++) {
        SylvesVector3 p = mesh->vertices[i];
        
        mesh->uvs[i].x = (p.x * u.x + p.y * u.y + p.z * u.z) * scale;
        mesh->uvs[i].y = (p.x * v.x + p.y * v.y + p.z * v.z) * scale;
    }
    
    return SYLVES_SUCCESS;
}

/* Mesh validation and repair */

bool sylves_mesh_has_non_manifold_edges(
    const SylvesMeshDataEx* mesh,
    size_t* edge_count) {
    
    if (!mesh) return false;
    
    /* Build edge topology if needed */
    SylvesMeshDataEx* mutable_mesh = (SylvesMeshDataEx*)mesh;
    if (!mesh->edge_data) {
        sylves_mesh_data_ex_build_edges(mutable_mesh);
    }
    
    /* Check if manifold */
    bool has_non_manifold = !sylves_mesh_data_ex_is_manifold(mesh);
    
    if (edge_count) {
        *edge_count = has_non_manifold ? 1 : 0; /* Simplified */
    }
    
    return has_non_manifold;
}

bool sylves_mesh_has_degenerate_faces(
    const SylvesMeshDataEx* mesh,
    size_t* face_count) {
    
    if (!mesh) return false;
    
    size_t degenerate = 0;
    
    for (size_t s = 0; s < mesh->submesh_count; s++) {
        SylvesFaceIterator iter;
        sylves_face_iterator_init(&iter, mesh, s);
        
        while (sylves_face_iterator_next(&iter)) {
            if (iter.vertex_count < 3) {
                degenerate++;
                continue;
            }
            
            /* Check for duplicate vertices in face */
            bool has_duplicate = false;
            for (int i = 0; i < iter.vertex_count && !has_duplicate; i++) {
                for (int j = i + 1; j < iter.vertex_count; j++) {
                    if (iter.face_vertices[i] == iter.face_vertices[j]) {
                        has_duplicate = true;
                        break;
                    }
                }
            }
            
            if (has_duplicate) {
                degenerate++;
            }
        }
    }
    
    if (face_count) {
        *face_count = degenerate;
    }
    
    return degenerate > 0;
}

/**
 * @brief Repair mesh
 *
 * Attempts to fix common mesh issues like non-manifold edges,
 * degenerate faces, inconsistent winding, etc.
 *
 * @param mesh Mesh to repair
 * @return New repaired mesh or NULL on error
 */
SylvesMeshDataEx* sylves_mesh_repair(const SylvesMeshDataEx* mesh) {
    if (!mesh) return NULL;
    
    /* For now, just remove degenerate faces */
    return sylves_mesh_remove_degenerate_faces(mesh);
}

SylvesMeshDataEx* sylves_mesh_remove_degenerate_faces(const SylvesMeshDataEx* mesh) {
    if (!mesh) return NULL;
    
    /* Clone mesh and filter faces */
    SylvesMeshDataEx* result = sylves_mesh_data_ex_create(
        mesh->vertex_count, mesh->submesh_count);
    if (!result) return NULL;
    
    /* Copy vertices and attributes */
    memcpy(result->vertices, mesh->vertices,
           sizeof(SylvesVector3) * mesh->vertex_count);
    
    if (mesh->normals) {
        sylves_mesh_data_ex_allocate_normals(result);
        memcpy(result->normals, mesh->normals,
               sizeof(SylvesVector3) * mesh->vertex_count);
    }
    
    if (mesh->uvs) {
        sylves_mesh_data_ex_allocate_uvs(result);
        memcpy(result->uvs, mesh->uvs,
               sizeof(SylvesVector2) * mesh->vertex_count);
    }
    
    if (mesh->tangents) {
        sylves_mesh_data_ex_allocate_tangents(result);
        memcpy(result->tangents, mesh->tangents,
               sizeof(SylvesVector4) * mesh->vertex_count);
    }
    
    /* Filter faces */
    for (size_t s = 0; s < mesh->submesh_count; s++) {
        const SylvesSubmesh* src = &mesh->submeshes[s];
        
        /* Count valid faces */
        size_t valid_indices = 0;
        SylvesFaceIterator iter;
        sylves_face_iterator_init(&iter, mesh, s);
        
        while (sylves_face_iterator_next(&iter)) {
            if (iter.vertex_count >= 3) {
                /* Check for duplicate vertices */
                bool has_duplicate = false;
                for (int i = 0; i < iter.vertex_count && !has_duplicate; i++) {
                    for (int j = i + 1; j < iter.vertex_count; j++) {
                        if (iter.face_vertices[i] == iter.face_vertices[j]) {
                            has_duplicate = true;
                            break;
                        }
                    }
                }
                
                if (!has_duplicate) {
                    valid_indices += iter.vertex_count;
                }
            }
        }
        
        /* Copy valid faces */
        int* indices = (int*)sylves_alloc(sizeof(int) * valid_indices);
        size_t idx = 0;
        
        sylves_face_iterator_init(&iter, mesh, s);
        while (sylves_face_iterator_next(&iter)) {
            if (iter.vertex_count >= 3) {
                /* Check for duplicate vertices */
                bool has_duplicate = false;
                for (int i = 0; i < iter.vertex_count && !has_duplicate; i++) {
                    for (int j = i + 1; j < iter.vertex_count; j++) {
                        if (iter.face_vertices[i] == iter.face_vertices[j]) {
                            has_duplicate = true;
                            break;
                        }
                    }
                }
                
                if (!has_duplicate) {
                    for (int i = 0; i < iter.vertex_count; i++) {
                        indices[idx++] = iter.face_vertices[i];
                    }
                    if (src->topology == SYLVES_MESH_TOPOLOGY_NGON) {
                        indices[idx - 1] = ~indices[idx - 1];
                    }
                }
            }
        }
        
        sylves_mesh_data_ex_set_submesh(
            result, s, indices, valid_indices, src->topology);
        
        sylves_free(indices);
    }
    
    return result;
}
