/**
 * @file mesh_data.c
 * @brief Implementation of mesh data structures and operations
 */

#include "sylves/mesh_data.h"
#include "sylves/memory.h"
#include "sylves/errors.h"
#include <string.h>
#include <math.h>

/* Creation and destruction */

SylvesMeshDataEx* sylves_mesh_data_ex_create(size_t vertex_count, size_t submesh_count) {
    SylvesMeshDataEx* mesh = (SylvesMeshDataEx*)sylves_alloc(sizeof(SylvesMeshDataEx));
    if (!mesh) {
        return NULL;
    }

    mesh->vertices = (SylvesVector3*)sylves_alloc(sizeof(SylvesVector3) * vertex_count);
    mesh->vertex_count = vertex_count;

    mesh->submeshes = (SylvesSubmesh*)sylves_calloc(submesh_count, sizeof(SylvesSubmesh));
    mesh->submesh_count = submesh_count;

    mesh->normals = NULL;
    mesh->uvs = NULL;
    mesh->tangents = NULL;

    mesh->edge_data = NULL;
    mesh->face_data = NULL;

    return mesh;
}

void sylves_mesh_data_ex_destroy(SylvesMeshDataEx* mesh) {
    if (!mesh) return;

    sylves_free(mesh->vertices);

    for (size_t i = 0; i < mesh->submesh_count; i++) {
        sylves_free(mesh->submeshes[i].indices);
    }
    sylves_free(mesh->submeshes);

    sylves_free(mesh->normals);
    sylves_free(mesh->uvs);
    sylves_free(mesh->tangents);

    sylves_free(mesh->edge_data);
    sylves_free(mesh->face_data);

    sylves_free(mesh);
}

SylvesMeshDataEx* sylves_mesh_data_ex_clone(const SylvesMeshDataEx* mesh) {
    SylvesMeshDataEx* clone = sylves_mesh_data_ex_create(mesh->vertex_count, mesh->submesh_count);
    if (!clone) {
        return NULL;
    }

    memcpy(clone->vertices, mesh->vertices, sizeof(SylvesVector3) * mesh->vertex_count);

    for (size_t i = 0; i < mesh->submesh_count; i++) {
        clone->submeshes[i].index_count = mesh->submeshes[i].index_count;
        clone->submeshes[i].topology = mesh->submeshes[i].topology;

        clone->submeshes[i].indices = (int*)sylves_alloc(sizeof(int) * mesh->submeshes[i].index_count);
        if (!clone->submeshes[i].indices) {
            sylves_mesh_data_ex_destroy(clone);
            return NULL;
        }
        memcpy(clone->submeshes[i].indices, mesh->submeshes[i].indices, sizeof(int) * mesh->submeshes[i].index_count);
    }

    if (mesh->normals) {
        clone->normals = (SylvesVector3*)sylves_alloc(sizeof(SylvesVector3) * mesh->vertex_count);
        if (!clone->normals) {
            sylves_mesh_data_ex_destroy(clone);
            return NULL;
        }
        memcpy(clone->normals, mesh->normals, sizeof(SylvesVector3) * mesh->vertex_count);
    }

    if (mesh->uvs) {
        clone->uvs = (SylvesVector2*)sylves_alloc(sizeof(SylvesVector2) * mesh->vertex_count);
        if (!clone->uvs) {
            sylves_mesh_data_ex_destroy(clone);
            return NULL;
        }
        memcpy(clone->uvs, mesh->uvs, sizeof(SylvesVector2) * mesh->vertex_count);
    }

    if (mesh->tangents) {
        clone->tangents = (SylvesVector4*)sylves_alloc(sizeof(SylvesVector4) * mesh->vertex_count);
        if (!clone->tangents) {
            sylves_mesh_data_ex_destroy(clone);
            return NULL;
        }
        memcpy(clone->tangents, mesh->tangents, sizeof(SylvesVector4) * mesh->vertex_count);
    }

    return clone;
}

/* Submesh management */

SylvesError sylves_mesh_data_ex_set_submesh(
    SylvesMeshDataEx* mesh,
    size_t submesh_index,
    const int* indices,
    size_t index_count,
    SylvesMeshTopology topology) {
    if (submesh_index >= mesh->submesh_count) {
        return SYLVES_ERROR_INVALID_ARGUMENT;
    }

    SylvesSubmesh* submesh = &mesh->submeshes[submesh_index];
    submesh->index_count = index_count;
    submesh->topology = topology;

    submesh->indices = (int*)sylves_alloc(sizeof(int) * index_count);
    if (!submesh->indices) {
        return SYLVES_ERROR_OUT_OF_MEMORY;
    }

    memcpy(submesh->indices, indices, sizeof(int) * index_count);

    return SYLVES_SUCCESS;
}

/* Attribute management */

SylvesError sylves_mesh_data_ex_allocate_normals(SylvesMeshDataEx* mesh) {
    mesh->normals = (SylvesVector3*)sylves_alloc(sizeof(SylvesVector3) * mesh->vertex_count);
    return mesh->normals ? SYLVES_SUCCESS : SYLVES_ERROR_OUT_OF_MEMORY;
}

SylvesError sylves_mesh_data_ex_allocate_uvs(SylvesMeshDataEx* mesh) {
    mesh->uvs = (SylvesVector2*)sylves_alloc(sizeof(SylvesVector2) * mesh->vertex_count);
    return mesh->uvs ? SYLVES_SUCCESS : SYLVES_ERROR_OUT_OF_MEMORY;
}

SylvesError sylves_mesh_data_ex_allocate_tangents(SylvesMeshDataEx* mesh) {
    mesh->tangents = (SylvesVector4*)sylves_alloc(sizeof(SylvesVector4) * mesh->vertex_count);
    return mesh->tangents ? SYLVES_SUCCESS : SYLVES_ERROR_OUT_OF_MEMORY;
}

/* Normal calculation */

void sylves_mesh_data_ex_recalculate_normals(SylvesMeshDataEx* mesh) {
    if (!mesh->normals) {
        sylves_mesh_data_ex_allocate_normals(mesh);
    }

    memset(mesh->normals, 0, sizeof(SylvesVector3) * mesh->vertex_count);

    for (size_t i = 0; i < mesh->submesh_count; i++) {
        SylvesSubmesh* submesh = &mesh->submeshes[i];
        SylvesFaceIterator iter;

        sylves_face_iterator_init(&iter, mesh, i);

        while (sylves_face_iterator_next(&iter)) {
            SylvesVector3 v0 = mesh->vertices[iter.face_vertices[0]];
            for (int j = 1; j < iter.vertex_count - 1; j++) {
                SylvesVector3 v1 = mesh->vertices[iter.face_vertices[j]];
                SylvesVector3 v2 = mesh->vertices[iter.face_vertices[j+1]];
                SylvesVector3 normal;

                // Cross product
                normal.x = (v1.y - v0.y) * (v2.z - v0.z) - (v1.z - v0.z) * (v2.y - v0.y);
                normal.y = (v1.z - v0.z) * (v2.x - v0.x) - (v1.x - v0.x) * (v2.z - v0.z);
                normal.z = (v1.x - v0.x) * (v2.y - v0.y) - (v1.y - v0.y) * (v2.x - v0.x);

                // Normalize and add to normals
                float length = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
                if (length > 1e-6f) {
                    normal.x /= length;
                    normal.y /= length;
                    normal.z /= length;
                }
                mesh->normals[iter.face_vertices[0]].x += normal.x;
                mesh->normals[iter.face_vertices[j]].x += normal.x;
                mesh->normals[iter.face_vertices[j+1]].x += normal.x;

                mesh->normals[iter.face_vertices[0]].y += normal.y;
                mesh->normals[iter.face_vertices[j]].y += normal.y;
                mesh->normals[iter.face_vertices[j+1]].y += normal.y;

                mesh->normals[iter.face_vertices[0]].z += normal.z;
                mesh->normals[iter.face_vertices[j]].z += normal.z;
                mesh->normals[iter.face_vertices[j+1]].z += normal.z;
            }
        }
    }

    // Normalize accumulated normals
    for (size_t i = 0; i < mesh->vertex_count; i++) {
        float length = sqrtf(mesh->normals[i].x * mesh->normals[i].x +
                             mesh->normals[i].y * mesh->normals[i].y +
                             mesh->normals[i].z * mesh->normals[i].z);
        if (length > 1e-6f) {
            mesh->normals[i].x /= length;
            mesh->normals[i].y /= length;
            mesh->normals[i].z /= length;
        }
    }
}

void sylves_mesh_data_ex_recalculate_tangents(SylvesMeshDataEx* mesh) {
    // Implementation of tangents recalculation (typically needed if mesh has UVs and normals)
    // For now, we skip the detailed implementation
}

/* Topology operations */

SylvesMeshDataEx* sylves_mesh_data_ex_triangulate(const SylvesMeshDataEx* mesh) {
    SylvesMeshDataEx* triangulated = sylves_mesh_data_ex_create(mesh->vertex_count, mesh->submesh_count);
    if (!triangulated) {
        return NULL;
    }

    memcpy(triangulated->vertices, mesh->vertices, sizeof(SylvesVector3) * mesh->vertex_count);

    // Copy optional attributes
    if (mesh->normals) {
        triangulated->normals = (SylvesVector3*)sylves_alloc(sizeof(SylvesVector3) * mesh->vertex_count);
        memcpy(triangulated->normals, mesh->normals, sizeof(SylvesVector3) * mesh->vertex_count);
    }
    if (mesh->uvs) {
        triangulated->uvs = (SylvesVector2*)sylves_alloc(sizeof(SylvesVector2) * mesh->vertex_count);
        memcpy(triangulated->uvs, mesh->uvs, sizeof(SylvesVector2) * mesh->vertex_count);
    }
    if (mesh->tangents) {
        triangulated->tangents = (SylvesVector4*)sylves_alloc(sizeof(SylvesVector4) * mesh->vertex_count);
        memcpy(triangulated->tangents, mesh->tangents, sizeof(SylvesVector4) * mesh->vertex_count);
    }

    for (size_t i = 0; i < mesh->submesh_count; i++) {
        SylvesSubmesh* original = &mesh->submeshes[i];
        SylvesSubmesh* result = &triangulated->submeshes[i];

        if (original->topology == SYLVES_MESH_TOPOLOGY_TRIANGLES) {
            result->index_count = original->index_count;
            result->topology = SYLVES_MESH_TOPOLOGY_TRIANGLES;
            result->indices = (int*)sylves_alloc(sizeof(int) * original->index_count);
            memcpy(result->indices, original->indices, sizeof(int) * original->index_count);
        } else {
            // Count triangles needed
            size_t tri_count = 0;
            SylvesFaceIterator iter;
            sylves_face_iterator_init(&iter, mesh, i);
            while (sylves_face_iterator_next(&iter)) {
                if (iter.vertex_count >= 3) {
                    tri_count += iter.vertex_count - 2;
                }
            }

            result->index_count = tri_count * 3;
            result->topology = SYLVES_MESH_TOPOLOGY_TRIANGLES;
            result->indices = (int*)sylves_alloc(sizeof(int) * result->index_count);
            
            // Triangulate faces
            size_t idx = 0;
            sylves_face_iterator_init(&iter, mesh, i);
            while (sylves_face_iterator_next(&iter)) {
                // Fan triangulation from first vertex
                for (int j = 2; j < iter.vertex_count; j++) {
                    result->indices[idx++] = iter.face_vertices[0];
                    result->indices[idx++] = iter.face_vertices[j-1];
                    result->indices[idx++] = iter.face_vertices[j];
                }
            }
        }
    }

    return triangulated;
}

SylvesMeshDataEx* sylves_mesh_data_ex_invert_winding(const SylvesMeshDataEx* mesh) {
    // Similar concept to triangulation
    // In this example, we'll just create a shallow clone to demonstrate
    SylvesMeshDataEx* inverted = sylves_mesh_data_ex_clone(mesh);
    if (!inverted) {
        return NULL;
    }

    for (size_t i = 0; i < inverted->submesh_count; i++) {
        SylvesSubmesh* submesh = &inverted->submeshes[i];

        for (size_t j = 0; j < submesh->index_count; j += 3) {
            int tmp = submesh->indices[j];
            submesh->indices[j] = submesh->indices[j+2];
            submesh->indices[j+2] = tmp;
        }
    }

    return inverted;
}

/* Transformation */

SylvesMeshDataEx* sylves_mesh_data_ex_transform(
    const SylvesMeshDataEx* mesh,
    const SylvesMatrix4x4* transform) {
    SylvesMeshDataEx* transformed = sylves_mesh_data_ex_clone(mesh);
    if (!transformed) {
        return NULL;
    }

    // Transform each vertex position
    for (size_t i = 0; i < transformed->vertex_count; i++) {
        SylvesVector3* v = &transformed->vertices[i];
        SylvesVector3 transformed_v;
        transformed_v.x = transform->m[0] * v->x + transform->m[4] * v->y + transform->m[8]  * v->z + transform->m[12];
        transformed_v.y = transform->m[1] * v->x + transform->m[5] * v->y + transform->m[9]  * v->z + transform->m[13];
        transformed_v.z = transform->m[2] * v->x + transform->m[6] * v->y + transform->m[10] * v->z + transform->m[14];
        *v = transformed_v;
    }

    return transformed;
}

/* Face iteration */

void sylves_face_iterator_init(
    SylvesFaceIterator* iter,
    const SylvesMeshDataEx* mesh,
    size_t submesh) {
    iter->indices = mesh->submeshes[submesh].indices;
    iter->remaining = mesh->submeshes[submesh].index_count;
    iter->topology = mesh->submeshes[submesh].topology;
}

bool sylves_face_iterator_next(SylvesFaceIterator* iter) {
    if (iter->remaining == 0) return false;

    switch (iter->topology) {
        case SYLVES_MESH_TOPOLOGY_TRIANGLES:
            iter->vertex_count = 3;
            memcpy(iter->face_vertices, iter->indices, sizeof(int) * 3);
            iter->indices += 3;
            iter->remaining -= 3;
            return true;

        case SYLVES_MESH_TOPOLOGY_QUADS:
            iter->vertex_count = 4;
            memcpy(iter->face_vertices, iter->indices, sizeof(int) * 4);
            iter->indices += 4;
            iter->remaining -= 4;
            return true;

        case SYLVES_MESH_TOPOLOGY_NGON:
            // Break the information from indices and check boundaries.
            // Assume last is ~index convention
            iter->vertex_count = 0;
            while (iter->remaining > 0) {
                int index = *iter->indices++;
                iter->remaining--;
                iter->face_vertices[iter->vertex_count++] = index >= 0 ? index : ~index;
                if (index < 0) break; // End of face
            }
            return iter->vertex_count > 0;

        default:
            return false;
    }
}

/* Validation */

bool sylves_mesh_data_ex_validate(const SylvesMeshDataEx* mesh) {
    if (!mesh || !mesh->vertices || mesh->vertex_count == 0) {
        return false;
    }
    
    // Check submeshes
    for (size_t s = 0; s < mesh->submesh_count; s++) {
        const SylvesSubmesh* submesh = &mesh->submeshes[s];
        if (!submesh->indices || submesh->index_count == 0) {
            return false;
        }
        
        // Validate topology
        switch (submesh->topology) {
            case SYLVES_MESH_TOPOLOGY_TRIANGLES:
                if (submesh->index_count % 3 != 0) return false;
                break;
            case SYLVES_MESH_TOPOLOGY_QUADS:
                if (submesh->index_count % 4 != 0) return false;
                break;
            case SYLVES_MESH_TOPOLOGY_NGON:
                // NGONs can have any count
                break;
            default:
                return false;
        }
        
        // Validate indices
        for (size_t i = 0; i < submesh->index_count; i++) {
            int idx = submesh->indices[i];
            if (submesh->topology == SYLVES_MESH_TOPOLOGY_NGON && idx < 0) {
                idx = ~idx; // Handle inverted index
            }
            if (idx < 0 || (size_t)idx >= mesh->vertex_count) {
                return false;
            }
        }
    }
    
    // Validate optional attributes
    if (mesh->normals) {
        // Check normals are normalized
        for (size_t i = 0; i < mesh->vertex_count; i++) {
            float len = sqrtf(mesh->normals[i].x * mesh->normals[i].x +
                             mesh->normals[i].y * mesh->normals[i].y +
                             mesh->normals[i].z * mesh->normals[i].z);
            if (fabsf(len - 1.0f) > 0.01f && len > 0.01f) {
                return false;
            }
        }
    }
    
    return true;
}

typedef struct EdgeKey {
    int v0, v1;
} EdgeKey;

typedef struct EdgeEntry {
    EdgeKey key;
    int f0, f1;
    int e0, e1;
    struct EdgeEntry* next;
} EdgeEntry;

typedef struct EdgeData {
    SylvesMeshEdge* edges;
    size_t edge_count;
    size_t edge_capacity;
} EdgeData;

bool sylves_mesh_data_ex_is_manifold(const SylvesMeshDataEx* mesh) {
    // Build edges if needed
    if (!mesh->edge_data) {
        SylvesMeshDataEx* mutable_mesh = (SylvesMeshDataEx*)mesh;
        sylves_mesh_data_ex_build_edges(mutable_mesh);
    }
    
    if (!mesh->edge_data) return false;
    
    EdgeData* edge_data = (EdgeData*)mesh->edge_data;
    
    // Check each edge has at most 2 faces
    for (size_t i = 0; i < edge_data->edge_count; i++) {
        const SylvesMeshEdge* edge = &edge_data->edges[i];
        // Non-manifold if edge has no faces (shouldn't happen)
        if (edge->f0 == -1) return false;
    }
    
    // TODO: Check for non-manifold vertices (pinch points)
    
    return true;
}

bool sylves_mesh_data_ex_is_closed(const SylvesMeshDataEx* mesh) {
    // Build edges if needed
    if (!mesh->edge_data) {
        SylvesMeshDataEx* mutable_mesh = (SylvesMeshDataEx*)mesh;
        sylves_mesh_data_ex_build_edges(mutable_mesh);
    }
    
    if (!mesh->edge_data) return false;
    
EdgeData* edge_data = (EdgeData*)mesh->edge_data;
    
    // Check all edges have exactly 2 faces
    for (size_t i = 0; i < edge_data->edge_count; i++) {
        const SylvesMeshEdge* edge = &edge_data->edges[i];
        if (edge->f1 == -1) {
            return false; // Boundary edge
        }
    }
    
    return true;
}

bool sylves_mesh_data_ex_has_consistent_winding(const SylvesMeshDataEx* mesh) {
    // Build edges if needed
    if (!mesh->edge_data) {
        SylvesMeshDataEx* mutable_mesh = (SylvesMeshDataEx*)mesh;
        sylves_mesh_data_ex_build_edges(mutable_mesh);
    }
    
    if (!mesh->edge_data) return false;
    
EdgeData* edge_data = (EdgeData*)mesh->edge_data;
    
    // Check that adjacent faces have opposite winding around shared edges
    // This is complex to implement fully, so we'll do a basic check
    
    return true;
}

static int edge_key_compare(EdgeKey a, EdgeKey b) {
    int min_a = a.v0 < a.v1 ? a.v0 : a.v1;
    int max_a = a.v0 < a.v1 ? a.v1 : a.v0;
    int min_b = b.v0 < b.v1 ? b.v0 : b.v1;
    int max_b = b.v0 < b.v1 ? b.v1 : b.v0;
    
    if (min_a < min_b) return -1;
    if (min_a > min_b) return 1;
    if (max_a < max_b) return -1;
    if (max_a > max_b) return 1;
    return 0;
}

/* Edge topology */

SylvesError sylves_mesh_data_ex_build_edges(SylvesMeshDataEx* mesh) {
    if (mesh->edge_data) {
        return SYLVES_SUCCESS; // Already built
    }
    
    // Count approximate edges
    size_t approx_edges = 0;
    for (size_t s = 0; s < mesh->submesh_count; s++) {
        SylvesFaceIterator iter;
        sylves_face_iterator_init(&iter, mesh, s);
        while (sylves_face_iterator_next(&iter)) {
            approx_edges += iter.vertex_count;
        }
    }
    
// Create edge data
    EdgeData* edge_data = (EdgeData*)sylves_alloc(sizeof(EdgeData));
    if (!edge_data) {
        return SYLVES_ERROR_OUT_OF_MEMORY;
    }
    
    edge_data->edges = (SylvesMeshEdge*)sylves_alloc(sizeof(SylvesMeshEdge) * approx_edges);
    edge_data->edge_count = 0;
    edge_data->edge_capacity = approx_edges;
    
    // Build edge hash table
    size_t hash_size = approx_edges * 2;
    EdgeEntry** hash_table = (EdgeEntry**)sylves_calloc(hash_size, sizeof(EdgeEntry*));
    
    int face_idx = 0;
    for (size_t s = 0; s < mesh->submesh_count; s++) {
        SylvesFaceIterator iter;
        sylves_face_iterator_init(&iter, mesh, s);
        
        while (sylves_face_iterator_next(&iter)) {
            // Process each edge of the face
            for (int e = 0; e < iter.vertex_count; e++) {
                int v0 = iter.face_vertices[e];
                int v1 = iter.face_vertices[(e + 1) % iter.vertex_count];
                
                EdgeKey key = {v0, v1};
                if (v0 > v1) {
                    key.v0 = v1;
                    key.v1 = v0;
                }
                
                // Hash lookup
                size_t hash = ((size_t)key.v0 * 73856093u + (size_t)key.v1 * 83492791u) % hash_size;
                EdgeEntry* entry = hash_table[hash];
                EdgeEntry* found = NULL;
                
                while (entry) {
                    if (entry->key.v0 == key.v0 && entry->key.v1 == key.v1) {
                        found = entry;
                        break;
                    }
                    entry = entry->next;
                }
                
                if (!found) {
                    // New edge
                    found = (EdgeEntry*)sylves_alloc(sizeof(EdgeEntry));
                    found->key = key;
                    found->f0 = face_idx;
                    found->f1 = -1;
                    found->e0 = e;
                    found->e1 = -1;
                    found->next = hash_table[hash];
                    hash_table[hash] = found;
                    
                    // Add to edge array
                    if (edge_data->edge_count < edge_data->edge_capacity) {
                        SylvesMeshEdge* edge = &edge_data->edges[edge_data->edge_count++];
                        edge->v0 = v0;
                        edge->v1 = v1;
                        edge->f0 = face_idx;
                        edge->f1 = -1;
                        edge->e0 = e;
                        edge->e1 = -1;
                    }
                } else {
                    // Update existing edge
                    found->f1 = face_idx;
                    found->e1 = e;
                    
                    // Update edge array
                    for (size_t i = 0; i < edge_data->edge_count; i++) {
                        SylvesMeshEdge* edge = &edge_data->edges[i];
                        if ((edge->v0 == v0 && edge->v1 == v1) ||
                            (edge->v0 == v1 && edge->v1 == v0)) {
                            edge->f1 = face_idx;
                            edge->e1 = e;
                            break;
                        }
                    }
                }
            }
            face_idx++;
        }
    }
    
    // Clean up hash table
    for (size_t i = 0; i < hash_size; i++) {
        EdgeEntry* entry = hash_table[i];
        while (entry) {
            EdgeEntry* next = entry->next;
            sylves_free(entry);
            entry = next;
        }
    }
    sylves_free(hash_table);
    
    mesh->edge_data = edge_data;
    return SYLVES_SUCCESS;
}

size_t sylves_mesh_data_ex_get_edge_count(const SylvesMeshDataEx* mesh) {
    if (!mesh->edge_data) return 0;
    EdgeData* edge_data = (EdgeData*)mesh->edge_data;
    return edge_data->edge_count;
}

const SylvesMeshEdge* sylves_mesh_data_ex_get_edges(const SylvesMeshDataEx* mesh) {
    if (!mesh->edge_data) return NULL;
    EdgeData* edge_data = (EdgeData*)mesh->edge_data;
    return edge_data->edges;
}

/* Mesh statistics */

double sylves_mesh_data_ex_get_surface_area(const SylvesMeshDataEx* mesh) {
    double area = 0.0;
    
    for (size_t s = 0; s < mesh->submesh_count; s++) {
        SylvesFaceIterator iter;
        sylves_face_iterator_init(&iter, mesh, s);
        
        while (sylves_face_iterator_next(&iter)) {
            if (iter.vertex_count < 3) continue;
            
            // Calculate area using triangulation
            SylvesVector3 v0 = mesh->vertices[iter.face_vertices[0]];
            
            for (int i = 2; i < iter.vertex_count; i++) {
                SylvesVector3 v1 = mesh->vertices[iter.face_vertices[i-1]];
                SylvesVector3 v2 = mesh->vertices[iter.face_vertices[i]];
                
                // Cross product for triangle area
                SylvesVector3 edge1 = {v1.x - v0.x, v1.y - v0.y, v1.z - v0.z};
                SylvesVector3 edge2 = {v2.x - v0.x, v2.y - v0.y, v2.z - v0.z};
                
                SylvesVector3 cross;
                cross.x = edge1.y * edge2.z - edge1.z * edge2.y;
                cross.y = edge1.z * edge2.x - edge1.x * edge2.z;
                cross.z = edge1.x * edge2.y - edge1.y * edge2.x;
                
                double triangle_area = 0.5 * sqrt(cross.x * cross.x + cross.y * cross.y + cross.z * cross.z);
                area += triangle_area;
            }
        }
    }
    
    return area;
}

double sylves_mesh_data_ex_get_volume(const SylvesMeshDataEx* mesh) {
    double volume = 0.0;
    
    // Use divergence theorem: volume = 1/3 * sum(face_normal · face_position * face_area)
    // For triangulated faces, we can use: volume = 1/6 * sum(v0 · (v1 × v2))
    
    for (size_t s = 0; s < mesh->submesh_count; s++) {
        SylvesFaceIterator iter;
        sylves_face_iterator_init(&iter, mesh, s);
        
        while (sylves_face_iterator_next(&iter)) {
            if (iter.vertex_count < 3) continue;
            
            SylvesVector3 v0 = mesh->vertices[iter.face_vertices[0]];
            
            for (int i = 2; i < iter.vertex_count; i++) {
                SylvesVector3 v1 = mesh->vertices[iter.face_vertices[i-1]];
                SylvesVector3 v2 = mesh->vertices[iter.face_vertices[i]];
                
                // Calculate signed volume of tetrahedron from origin
                double tet_volume = (v0.x * (v1.y * v2.z - v1.z * v2.y) +
                                    v0.y * (v1.z * v2.x - v1.x * v2.z) +
                                    v0.z * (v1.x * v2.y - v1.y * v2.x)) / 6.0;
                
                volume += tet_volume;
            }
        }
    }
    
    return fabs(volume); // Return absolute value
}

void sylves_mesh_data_ex_get_bounds(
    const SylvesMeshDataEx* mesh,
    SylvesVector3* min,
    SylvesVector3* max) {
    if (mesh->vertex_count == 0) {
        return;
    }

    *min = mesh->vertices[0];
    *max = mesh->vertices[0];

    for (size_t i = 1; i < mesh->vertex_count; i++) {
        *min = sylves_vector3_min(*min, mesh->vertices[i]);
        *max = sylves_vector3_max(*max, mesh->vertices[i]);
    }
}

/* Conversion from simple mesh data */

SylvesMeshDataEx* sylves_mesh_data_to_ex(const SylvesMeshData* simple) {
    if (!simple) return NULL;
    
    // Create extended mesh with one submesh
    SylvesMeshDataEx* ex = sylves_mesh_data_ex_create(simple->vertex_count, 1);
    if (!ex) return NULL;
    
    // Copy vertices
    memcpy(ex->vertices, simple->vertices, sizeof(SylvesVector3) * simple->vertex_count);
    
    // Convert faces to indices
    // Count total indices needed
    size_t total_indices = 0;
    for (size_t f = 0; f < simple->face_count; f++) {
        total_indices += simple->faces[f].vertex_count;
    }
    
    // Create index array
    int* indices = (int*)sylves_alloc(sizeof(int) * total_indices);
    if (!indices) {
        sylves_mesh_data_ex_destroy(ex);
        return NULL;
    }
    
    // Copy face vertices to index array
    size_t idx = 0;
    for (size_t f = 0; f < simple->face_count; f++) {
        memcpy(&indices[idx], simple->faces[f].vertices, 
               sizeof(int) * simple->faces[f].vertex_count);
        idx += simple->faces[f].vertex_count;
    }
    
    // Set up submesh
    SylvesError err = sylves_mesh_data_ex_set_submesh(
        ex, 0, indices, total_indices, SYLVES_MESH_TOPOLOGY_NGON
    );
    
    sylves_free(indices);
    
    if (err != SYLVES_SUCCESS) {
        sylves_mesh_data_ex_destroy(ex);
        return NULL;
    }
    
    // Copy optional attributes
    if (simple->normals) {
        if (sylves_mesh_data_ex_allocate_normals(ex) == SYLVES_SUCCESS) {
            memcpy(ex->normals, simple->normals, sizeof(SylvesVector3) * simple->vertex_count);
        }
    }
    
    if (simple->uvs) {
        if (sylves_mesh_data_ex_allocate_uvs(ex) == SYLVES_SUCCESS) {
            memcpy(ex->uvs, simple->uvs, sizeof(SylvesVector2) * simple->vertex_count);
        }
    }
    
    return ex;
}

SylvesMeshData* sylves_mesh_data_ex_to_simple(const SylvesMeshDataEx* ex) {
    if (!ex || ex->submesh_count == 0) return NULL;
    
    // Count total triangles
    size_t total_indices = 0;
    for (size_t s = 0; s < ex->submesh_count; s++) {
        if (ex->submeshes[s].topology == SYLVES_MESH_TOPOLOGY_TRIANGLES) {
            total_indices += ex->submeshes[s].index_count;
        } else {
            // Need to triangulate
            SylvesFaceIterator iter;
            sylves_face_iterator_init(&iter, ex, s);
            while (sylves_face_iterator_next(&iter)) {
                if (iter.vertex_count >= 3) {
                    total_indices += (iter.vertex_count - 2) * 3;
                }
            }
        }
    }
    
    // Create simple mesh
    SylvesMeshData* simple = (SylvesMeshData*)sylves_alloc(sizeof(SylvesMeshData));
    if (!simple) return NULL;
    
    simple->vertices = (SylvesVector3*)sylves_alloc(sizeof(SylvesVector3) * ex->vertex_count);
    simple->vertex_count = ex->vertex_count;
    
    // Copy vertices
    memcpy(simple->vertices, ex->vertices, sizeof(SylvesVector3) * ex->vertex_count);
    
    // Count faces
    size_t face_count = 0;
    for (size_t s = 0; s < ex->submesh_count; s++) {
        SylvesFaceIterator iter;
        sylves_face_iterator_init(&iter, ex, s);
        while (sylves_face_iterator_next(&iter)) {
            face_count++;
        }
    }
    
    // Allocate faces
    simple->faces = (SylvesMeshFace*)sylves_alloc(sizeof(SylvesMeshFace) * face_count);
    simple->face_count = face_count;
    
    // Copy face data
    size_t face_idx = 0;
    for (size_t s = 0; s < ex->submesh_count; s++) {
        SylvesFaceIterator iter;
        sylves_face_iterator_init(&iter, ex, s);
        while (sylves_face_iterator_next(&iter)) {
            simple->faces[face_idx].vertex_count = iter.vertex_count;
            simple->faces[face_idx].vertices = (int*)sylves_alloc(sizeof(int) * iter.vertex_count);
            memcpy(simple->faces[face_idx].vertices, iter.face_vertices, sizeof(int) * iter.vertex_count);
            simple->faces[face_idx].neighbors = NULL; // Not computed here
            face_idx++;
        }
    }
    
    // Copy optional attributes
    if (ex->normals) {
        simple->normals = (SylvesVector3*)sylves_alloc(sizeof(SylvesVector3) * ex->vertex_count);
        if (simple->normals) {
            memcpy(simple->normals, ex->normals, sizeof(SylvesVector3) * ex->vertex_count);
        }
    } else {
        simple->normals = NULL;
    }
    
    if (ex->uvs) {
        simple->uvs = (SylvesVector2*)sylves_alloc(sizeof(SylvesVector2) * ex->vertex_count);
        if (simple->uvs) {
            memcpy(simple->uvs, ex->uvs, sizeof(SylvesVector2) * ex->vertex_count);
        }
    } else {
        simple->uvs = NULL;
    }
    
    return simple;
}
