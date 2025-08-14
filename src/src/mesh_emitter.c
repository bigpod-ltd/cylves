/**
 * @file mesh_emitter.c
 * @brief Implementation of mesh emitter for building meshes incrementally
 */

#include "sylves/mesh_emitter.h"
#include "sylves/mesh_data.h"
#include "sylves/memory.h"
#include "sylves/errors.h"
#include <string.h>

struct SylvesMeshEmitter {
    const SylvesMeshDataEx* original_mesh;
    
    /* Output data */
    SylvesVector3* vertices;
    size_t vertex_count;
    size_t vertex_capacity;
    
    SylvesVector2* uvs;
    SylvesVector3* normals;
    SylvesVector4* tangents;
    
    /* Submesh building */
    int** indices;
    size_t* index_counts;
    size_t* index_capacities;
    SylvesMeshTopology* topologies;
    size_t submesh_count;
    size_t submesh_capacity;
    
    /* Current submesh being built */
    int current_submesh;
    SylvesMeshTopology current_topology;
};

/* Create emitter */
SylvesMeshEmitter* sylves_mesh_emitter_create(const SylvesMeshDataEx* original_mesh) {
    SylvesMeshEmitter* emitter = (SylvesMeshEmitter*)sylves_alloc(sizeof(SylvesMeshEmitter));
    if (!emitter) return NULL;
    
    emitter->original_mesh = original_mesh;
    
    emitter->vertices = NULL;
    emitter->vertex_count = 0;
    emitter->vertex_capacity = 0;
    
    emitter->uvs = NULL;
    emitter->normals = NULL;
    emitter->tangents = NULL;
    
    emitter->indices = NULL;
    emitter->index_counts = NULL;
    emitter->index_capacities = NULL;
    emitter->topologies = NULL;
    emitter->submesh_count = 0;
    emitter->submesh_capacity = 0;
    
    emitter->current_submesh = -1;
    
    /* Allocate initial vertex capacity */
    emitter->vertex_capacity = original_mesh ? original_mesh->vertex_count * 2 : 256;
    emitter->vertices = (SylvesVector3*)sylves_alloc(sizeof(SylvesVector3) * emitter->vertex_capacity);
    if (!emitter->vertices) {
        sylves_mesh_emitter_destroy(emitter);
        return NULL;
    }
    
    /* Allocate attributes if original has them */
    if (original_mesh && original_mesh->uvs) {
        emitter->uvs = (SylvesVector2*)sylves_alloc(sizeof(SylvesVector2) * emitter->vertex_capacity);
    }
    if (original_mesh && original_mesh->normals) {
        emitter->normals = (SylvesVector3*)sylves_alloc(sizeof(SylvesVector3) * emitter->vertex_capacity);
    }
    if (original_mesh && original_mesh->tangents) {
        emitter->tangents = (SylvesVector4*)sylves_alloc(sizeof(SylvesVector4) * emitter->vertex_capacity);
    }
    
    return emitter;
}

/* Destroy emitter */
void sylves_mesh_emitter_destroy(SylvesMeshEmitter* emitter) {
    if (!emitter) return;
    
    sylves_free(emitter->vertices);
    sylves_free(emitter->uvs);
    sylves_free(emitter->normals);
    sylves_free(emitter->tangents);
    
    if (emitter->indices) {
        for (size_t i = 0; i < emitter->submesh_count; i++) {
            sylves_free(emitter->indices[i]);
        }
        sylves_free(emitter->indices);
    }
    
    sylves_free(emitter->index_counts);
    sylves_free(emitter->index_capacities);
    sylves_free(emitter->topologies);
    
    sylves_free(emitter);
}

/* Ensure vertex capacity */
static bool ensure_vertex_capacity(SylvesMeshEmitter* emitter, size_t required) {
    if (emitter->vertex_capacity >= required) return true;
    
    size_t new_capacity = emitter->vertex_capacity * 2;
    while (new_capacity < required) new_capacity *= 2;
    
    SylvesVector3* new_vertices = (SylvesVector3*)sylves_realloc(
        emitter->vertices, sizeof(SylvesVector3) * new_capacity);
    if (!new_vertices) return false;
    emitter->vertices = new_vertices;
    
    if (emitter->uvs) {
        SylvesVector2* new_uvs = (SylvesVector2*)sylves_realloc(
            emitter->uvs, sizeof(SylvesVector2) * new_capacity);
        if (!new_uvs) return false;
        emitter->uvs = new_uvs;
    }
    
    if (emitter->normals) {
        SylvesVector3* new_normals = (SylvesVector3*)sylves_realloc(
            emitter->normals, sizeof(SylvesVector3) * new_capacity);
        if (!new_normals) return false;
        emitter->normals = new_normals;
    }
    
    if (emitter->tangents) {
        SylvesVector4* new_tangents = (SylvesVector4*)sylves_realloc(
            emitter->tangents, sizeof(SylvesVector4) * new_capacity);
        if (!new_tangents) return false;
        emitter->tangents = new_tangents;
    }
    
    emitter->vertex_capacity = new_capacity;
    return true;
}

/* Ensure submesh capacity */
static bool ensure_submesh_capacity(SylvesMeshEmitter* emitter, size_t required) {
    if (emitter->submesh_capacity >= required) return true;
    
    size_t new_capacity = emitter->submesh_capacity == 0 ? 4 : emitter->submesh_capacity * 2;
    while (new_capacity < required) new_capacity *= 2;
    
    int** new_indices = (int**)sylves_realloc(
        emitter->indices, sizeof(int*) * new_capacity);
    if (!new_indices) return false;
    emitter->indices = new_indices;
    
    size_t* new_counts = (size_t*)sylves_realloc(
        emitter->index_counts, sizeof(size_t) * new_capacity);
    if (!new_counts) return false;
    emitter->index_counts = new_counts;
    
    size_t* new_capacities = (size_t*)sylves_realloc(
        emitter->index_capacities, sizeof(size_t) * new_capacity);
    if (!new_capacities) return false;
    emitter->index_capacities = new_capacities;
    
    SylvesMeshTopology* new_topologies = (SylvesMeshTopology*)sylves_realloc(
        emitter->topologies, sizeof(SylvesMeshTopology) * new_capacity);
    if (!new_topologies) return false;
    emitter->topologies = new_topologies;
    
    /* Initialize new entries */
    for (size_t i = emitter->submesh_capacity; i < new_capacity; i++) {
        new_indices[i] = NULL;
        new_counts[i] = 0;
        new_capacities[i] = 0;
    }
    
    emitter->submesh_capacity = new_capacity;
    return true;
}

/* Ensure current submesh index capacity */
static bool ensure_index_capacity(SylvesMeshEmitter* emitter, size_t required) {
    if (emitter->current_submesh < 0) return false;
    
    size_t* capacity = &emitter->index_capacities[emitter->current_submesh];
    if (*capacity >= required) return true;
    
    size_t new_capacity = *capacity == 0 ? 256 : *capacity * 2;
    while (new_capacity < required) new_capacity *= 2;
    
    int* new_indices = (int*)sylves_realloc(
        emitter->indices[emitter->current_submesh], sizeof(int) * new_capacity);
    if (!new_indices) return false;
    
    emitter->indices[emitter->current_submesh] = new_indices;
    *capacity = new_capacity;
    
    return true;
}

/* Copy vertices from original mesh */
void sylves_mesh_emitter_copy_vertices(SylvesMeshEmitter* emitter) {
    if (!emitter || !emitter->original_mesh) return;
    
    const SylvesMeshDataEx* mesh = emitter->original_mesh;
    
    if (!ensure_vertex_capacity(emitter, mesh->vertex_count)) return;
    
    memcpy(emitter->vertices, mesh->vertices, sizeof(SylvesVector3) * mesh->vertex_count);
    
    if (emitter->uvs && mesh->uvs) {
        memcpy(emitter->uvs, mesh->uvs, sizeof(SylvesVector2) * mesh->vertex_count);
    }
    
    if (emitter->normals && mesh->normals) {
        memcpy(emitter->normals, mesh->normals, sizeof(SylvesVector3) * mesh->vertex_count);
    }
    
    if (emitter->tangents && mesh->tangents) {
        memcpy(emitter->tangents, mesh->tangents, sizeof(SylvesVector4) * mesh->vertex_count);
    }
    
    emitter->vertex_count = mesh->vertex_count;
}

/* Start new submesh */
void sylves_mesh_emitter_start_submesh(
    SylvesMeshEmitter* emitter,
    SylvesMeshTopology topology) {
    
    if (!emitter) return;
    
    if (!ensure_submesh_capacity(emitter, emitter->submesh_count + 1)) return;
    
    emitter->current_submesh = emitter->submesh_count;
    emitter->current_topology = topology;
    emitter->topologies[emitter->current_submesh] = topology;
    emitter->submesh_count++;
}

/* End current submesh */
void sylves_mesh_emitter_end_submesh(SylvesMeshEmitter* emitter) {
    if (!emitter) return;
    emitter->current_submesh = -1;
}

/* Add vertex */
int sylves_mesh_emitter_add_vertex(
    SylvesMeshEmitter* emitter,
    const SylvesVector3* position,
    const SylvesVector2* uv,
    const SylvesVector3* normal,
    const SylvesVector4* tangent) {
    
    if (!emitter || !position) return -1;
    
    if (!ensure_vertex_capacity(emitter, emitter->vertex_count + 1)) return -1;
    
    int index = emitter->vertex_count;
    
    emitter->vertices[index] = *position;
    
    if (emitter->uvs) {
        emitter->uvs[index] = uv ? *uv : (SylvesVector2){0, 0};
    }
    
    if (emitter->normals) {
        emitter->normals[index] = normal ? *normal : (SylvesVector3){0, 0, 0};
    }
    
    if (emitter->tangents) {
        emitter->tangents[index] = tangent ? *tangent : (SylvesVector4){0, 0, 0, 0};
    }
    
    emitter->vertex_count++;
    
    return index;
}

/* Average two vertices */
int sylves_mesh_emitter_average_vertices(
    SylvesMeshEmitter* emitter, int i1, int i2) {
    
    if (!emitter || i1 < 0 || i2 < 0 || 
        (size_t)i1 >= emitter->vertex_count || 
        (size_t)i2 >= emitter->vertex_count) {
        return -1;
    }
    
    SylvesVector3 v = {
        (emitter->vertices[i1].x + emitter->vertices[i2].x) * 0.5f,
        (emitter->vertices[i1].y + emitter->vertices[i2].y) * 0.5f,
        (emitter->vertices[i1].z + emitter->vertices[i2].z) * 0.5f
    };
    
    SylvesVector2* uv = NULL;
    SylvesVector2 uv_avg;
    if (emitter->uvs) {
        uv_avg.x = (emitter->uvs[i1].x + emitter->uvs[i2].x) * 0.5f;
        uv_avg.y = (emitter->uvs[i1].y + emitter->uvs[i2].y) * 0.5f;
        uv = &uv_avg;
    }
    
    SylvesVector3* normal = NULL;
    SylvesVector3 normal_avg;
    if (emitter->normals) {
        normal_avg.x = emitter->normals[i1].x + emitter->normals[i2].x;
        normal_avg.y = emitter->normals[i1].y + emitter->normals[i2].y;
        normal_avg.z = emitter->normals[i1].z + emitter->normals[i2].z;
        float len = sqrtf(normal_avg.x * normal_avg.x + 
                         normal_avg.y * normal_avg.y + 
                         normal_avg.z * normal_avg.z);
        if (len > 1e-6f) {
            normal_avg.x /= len;
            normal_avg.y /= len;
            normal_avg.z /= len;
        }
        normal = &normal_avg;
    }
    
    SylvesVector4* tangent = NULL;
    SylvesVector4 tangent_avg;
    if (emitter->tangents) {
        tangent_avg.x = (emitter->tangents[i1].x + emitter->tangents[i2].x) * 0.5f;
        tangent_avg.y = (emitter->tangents[i1].y + emitter->tangents[i2].y) * 0.5f;
        tangent_avg.z = (emitter->tangents[i1].z + emitter->tangents[i2].z) * 0.5f;
        tangent_avg.w = (emitter->tangents[i1].w + emitter->tangents[i2].w) * 0.5f;
        tangent = &tangent_avg;
    }
    
    return sylves_mesh_emitter_add_vertex(emitter, &v, uv, normal, tangent);
}

/* Average multiple vertices */
int sylves_mesh_emitter_average_face(
    SylvesMeshEmitter* emitter,
    const int* indices,
    size_t count) {
    
    if (!emitter || !indices || count == 0) return -1;
    
    SylvesVector3 v = {0, 0, 0};
    SylvesVector2 uv = {0, 0};
    SylvesVector3 normal = {0, 0, 0};
    SylvesVector4 tangent = {0, 0, 0, 0};
    
    for (size_t i = 0; i < count; i++) {
        int idx = indices[i];
        if (idx < 0 || (size_t)idx >= emitter->vertex_count) continue;
        
        v.x += emitter->vertices[idx].x;
        v.y += emitter->vertices[idx].y;
        v.z += emitter->vertices[idx].z;
        
        if (emitter->uvs) {
            uv.x += emitter->uvs[idx].x;
            uv.y += emitter->uvs[idx].y;
        }
        
        if (emitter->normals) {
            normal.x += emitter->normals[idx].x;
            normal.y += emitter->normals[idx].y;
            normal.z += emitter->normals[idx].z;
        }
        
        if (emitter->tangents) {
            tangent.x += emitter->tangents[idx].x;
            tangent.y += emitter->tangents[idx].y;
            tangent.z += emitter->tangents[idx].z;
            tangent.w += emitter->tangents[idx].w;
        }
    }
    
    float inv_count = 1.0f / count;
    v.x *= inv_count;
    v.y *= inv_count;
    v.z *= inv_count;
    
    SylvesVector2* uv_ptr = NULL;
    if (emitter->uvs) {
        uv.x *= inv_count;
        uv.y *= inv_count;
        uv_ptr = &uv;
    }
    
    SylvesVector3* normal_ptr = NULL;
    if (emitter->normals) {
        float len = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
        if (len > 1e-6f) {
            normal.x /= len;
            normal.y /= len;
            normal.z /= len;
        }
        normal_ptr = &normal;
    }
    
    SylvesVector4* tangent_ptr = NULL;
    if (emitter->tangents) {
        tangent.x *= inv_count;
        tangent.y *= inv_count;
        tangent.z *= inv_count;
        tangent.w *= inv_count;
        tangent_ptr = &tangent;
    }
    
    return sylves_mesh_emitter_add_vertex(emitter, &v, uv_ptr, normal_ptr, tangent_ptr);
}

/* Add face indices */
static void add_indices(SylvesMeshEmitter* emitter, const int* indices, size_t count) {
    if (emitter->current_submesh < 0) return;
    
    size_t current_count = emitter->index_counts[emitter->current_submesh];
    if (!ensure_index_capacity(emitter, current_count + count)) return;
    
    memcpy(&emitter->indices[emitter->current_submesh][current_count], 
           indices, sizeof(int) * count);
    
    emitter->index_counts[emitter->current_submesh] += count;
}

/* Add triangle face */
void sylves_mesh_emitter_add_face3(
    SylvesMeshEmitter* emitter, int i0, int i1, int i2) {
    
    if (!emitter || emitter->current_submesh < 0) return;
    
    if (emitter->current_topology == SYLVES_MESH_TOPOLOGY_TRIANGLES) {
        int indices[3] = {i0, i1, i2};
        add_indices(emitter, indices, 3);
    } else if (emitter->current_topology == SYLVES_MESH_TOPOLOGY_NGON) {
        int indices[3] = {i0, i1, ~i2};
        add_indices(emitter, indices, 3);
    }
}

/* Add quad face */
void sylves_mesh_emitter_add_face4(
    SylvesMeshEmitter* emitter, int i0, int i1, int i2, int i3) {
    
    if (!emitter || emitter->current_submesh < 0) return;
    
    if (emitter->current_topology == SYLVES_MESH_TOPOLOGY_QUADS) {
        int indices[4] = {i0, i1, i2, i3};
        add_indices(emitter, indices, 4);
    } else if (emitter->current_topology == SYLVES_MESH_TOPOLOGY_NGON) {
        int indices[4] = {i0, i1, i2, ~i3};
        add_indices(emitter, indices, 4);
    }
}

/* Add polygon face */
void sylves_mesh_emitter_add_face(
    SylvesMeshEmitter* emitter,
    const int* indices,
    size_t count) {
    
    if (!emitter || emitter->current_submesh < 0 || !indices || count < 3) return;
    
    if (emitter->current_topology == SYLVES_MESH_TOPOLOGY_NGON) {
        /* Need to mark last index as negative */
        if (!ensure_index_capacity(emitter, 
            emitter->index_counts[emitter->current_submesh] + count)) return;
        
        for (size_t i = 0; i < count - 1; i++) {
            int idx = indices[i];
            add_indices(emitter, &idx, 1);
        }
        
        int last = ~indices[count - 1];
        add_indices(emitter, &last, 1);
    } else {
        /* For fixed topology, just add as-is */
        add_indices(emitter, indices, count);
    }
}

/* Convert to mesh */
SylvesMeshDataEx* sylves_mesh_emitter_to_mesh(SylvesMeshEmitter* emitter) {
    if (!emitter || emitter->vertex_count == 0 || emitter->submesh_count == 0) {
        return NULL;
    }
    
    SylvesMeshDataEx* mesh = sylves_mesh_data_ex_create(
        emitter->vertex_count, emitter->submesh_count);
    if (!mesh) return NULL;
    
    /* Copy vertices */
    memcpy(mesh->vertices, emitter->vertices, 
           sizeof(SylvesVector3) * emitter->vertex_count);
    
    /* Allocate and copy attributes if present */
    if (emitter->uvs) {
        if (sylves_mesh_data_ex_allocate_uvs(mesh) == SYLVES_SUCCESS) {
            memcpy(mesh->uvs, emitter->uvs, 
                   sizeof(SylvesVector2) * emitter->vertex_count);
        }
    }
    
    if (emitter->normals) {
        if (sylves_mesh_data_ex_allocate_normals(mesh) == SYLVES_SUCCESS) {
            memcpy(mesh->normals, emitter->normals, 
                   sizeof(SylvesVector3) * emitter->vertex_count);
        }
    }
    
    if (emitter->tangents) {
        if (sylves_mesh_data_ex_allocate_tangents(mesh) == SYLVES_SUCCESS) {
            memcpy(mesh->tangents, emitter->tangents, 
                   sizeof(SylvesVector4) * emitter->vertex_count);
        }
    }
    
    /* Copy submeshes */
    for (size_t s = 0; s < emitter->submesh_count; s++) {
        if (emitter->index_counts[s] > 0) {
            sylves_mesh_data_ex_set_submesh(
                mesh, s, 
                emitter->indices[s], 
                emitter->index_counts[s],
                emitter->topologies[s]);
        }
    }
    
    return mesh;
}
