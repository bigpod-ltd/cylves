/**
 * @file dual_mesh_builder.c
 * @brief Implementation of dual mesh generation from primal meshes
 */

#include "sylves/dual_mesh_builder.h"
#include "sylves/mesh_data.h"
#include "sylves/mesh_utilities.h"
#include "sylves/memory.h"
#include "sylves/errors.h"
#include <string.h>
#include <math.h>
#include <float.h>

/* Forward declarations */
void sylves_dual_mesh_builder_destroy(SylvesDualMeshBuilder* builder);

/* Far vertices threshold - vertices beyond this are considered at infinity */
#define FAR_THRESHOLD 1e10f

/* Hash table for halfedge lookups */
typedef struct HalfEdgeKey {
    int face;
    int edge;
} HalfEdgeKey;

typedef struct HalfEdgeEntry {
    HalfEdgeKey key;
    HalfEdgeKey flip;  /* Face and edge on other side */
    bool has_flip;
    struct HalfEdgeEntry* next;
} HalfEdgeEntry;

typedef struct HalfEdgeMap {
    HalfEdgeEntry** buckets;
    size_t bucket_count;
    size_t entry_count;
} HalfEdgeMap;

struct SylvesDualMeshBuilder {
    const SylvesMeshDataEx* primal_mesh;
    SylvesMeshDataEx* dual_mesh;
    
    /* Mapping between primal and dual */
    SylvesDualMapping* mappings;
    size_t mapping_count;
    size_t mapping_capacity;
    
    /* Primal mesh analysis */
    int* face_centroids;      /* Index of centroid vertex for each face */
    bool* is_far_vertex;      /* Whether vertex is at infinity */
    size_t face_count;
    
    /* Halfedge connectivity */
    HalfEdgeMap* halfedge_map;
    
    /* Temporary buffers */
    int* face_buffer;
    size_t face_buffer_capacity;
};

/* Hash function for halfedge keys */
static size_t halfedge_hash(HalfEdgeKey key, size_t bucket_count) {
    return ((size_t)key.face * 73856093u + (size_t)key.edge * 83492791u) % bucket_count;
}

/* Create halfedge map */
static HalfEdgeMap* halfedge_map_create(size_t expected_size) {
    HalfEdgeMap* map = (HalfEdgeMap*)sylves_alloc(sizeof(HalfEdgeMap));
    if (!map) return NULL;
    
    map->bucket_count = expected_size * 2;
    map->buckets = (HalfEdgeEntry**)sylves_calloc(map->bucket_count, sizeof(HalfEdgeEntry*));
    if (!map->buckets) {
        sylves_free(map);
        return NULL;
    }
    
    map->entry_count = 0;
    return map;
}

/* Destroy halfedge map */
static void halfedge_map_destroy(HalfEdgeMap* map) {
    if (!map) return;
    
    for (size_t i = 0; i < map->bucket_count; i++) {
        HalfEdgeEntry* entry = map->buckets[i];
        while (entry) {
            HalfEdgeEntry* next = entry->next;
            sylves_free(entry);
            entry = next;
        }
    }
    
    sylves_free(map->buckets);
    sylves_free(map);
}

/* Add halfedge to map */
static void halfedge_map_add(HalfEdgeMap* map, HalfEdgeKey key, HalfEdgeKey flip) {
    size_t hash = halfedge_hash(key, map->bucket_count);
    
    HalfEdgeEntry* entry = (HalfEdgeEntry*)sylves_alloc(sizeof(HalfEdgeEntry));
    entry->key = key;
    entry->flip = flip;
    entry->has_flip = true;
    entry->next = map->buckets[hash];
    map->buckets[hash] = entry;
    map->entry_count++;
}

/* Find flip of halfedge */
static bool halfedge_map_find_flip(HalfEdgeMap* map, HalfEdgeKey key, HalfEdgeKey* flip) {
    size_t hash = halfedge_hash(key, map->bucket_count);
    
    HalfEdgeEntry* entry = map->buckets[hash];
    while (entry) {
        if (entry->key.face == key.face && entry->key.edge == key.edge) {
            if (entry->has_flip) {
                *flip = entry->flip;
                return true;
            }
            return false;
        }
        entry = entry->next;
    }
    
    return false;
}

/* Build halfedge connectivity from mesh */
static HalfEdgeMap* build_halfedge_map(const SylvesMeshDataEx* mesh) {
    /* Count approximate edges */
    size_t approx_edges = 0;
    for (size_t s = 0; s < mesh->submesh_count; s++) {
        approx_edges += mesh->submeshes[s].index_count;
    }
    
    HalfEdgeMap* map = halfedge_map_create(approx_edges);
    if (!map) return NULL;
    
    /* Build edge lookup table */
    typedef struct EdgeKey {
        int v0, v1;
    } EdgeKey;
    
    typedef struct EdgeRecord {
        int face;
        int edge;
    } EdgeRecord;
    
    /* Simple hash table for edge lookups */
    size_t edge_buckets = approx_edges * 2;
    EdgeRecord** edge_table = (EdgeRecord**)sylves_calloc(edge_buckets, sizeof(EdgeRecord*));
    
    /* Process all faces */
    int face_idx = 0;
    for (size_t s = 0; s < mesh->submesh_count; s++) {
        SylvesFaceIterator iter;
        sylves_face_iterator_init(&iter, mesh, s);
        
        while (sylves_face_iterator_next(&iter)) {
            /* Process each edge of face */
            for (int e = 0; e < iter.vertex_count; e++) {
                int v0 = iter.face_vertices[e];
                int v1 = iter.face_vertices[(e + 1) % iter.vertex_count];
                
                /* Canonical edge key (smaller vertex first) */
                EdgeKey key = {v0 < v1 ? v0 : v1, v0 < v1 ? v1 : v0};
                size_t hash = ((size_t)key.v0 * 19260817u + (size_t)key.v1 * 37159393u) % edge_buckets;
                
                /* Look for existing edge */
                EdgeRecord* rec = edge_table[hash];
                while (rec) {
                    /* Check if this completes a pair */
                    if (rec->face != face_idx) {
                        /* Found matching edge - add both halfedges */
                        HalfEdgeKey he1 = {rec->face, rec->edge};
                        HalfEdgeKey he2 = {face_idx, e};
                        halfedge_map_add(map, he1, he2);
                        halfedge_map_add(map, he2, he1);
                        break;
                    }
                    rec = (EdgeRecord*)((char*)rec + sizeof(EdgeRecord));
                }
                
                /* Add new edge record */
                EdgeRecord* new_rec = (EdgeRecord*)sylves_alloc(sizeof(EdgeRecord));
                new_rec->face = face_idx;
                new_rec->edge = e;
                /* Simple linked list in hash bucket */
                void* next = edge_table[hash];
                edge_table[hash] = new_rec;
            }
            face_idx++;
        }
    }
    
    /* Clean up edge table */
    for (size_t i = 0; i < edge_buckets; i++) {
        EdgeRecord* rec = edge_table[i];
        while (rec) {
            EdgeRecord* next = (EdgeRecord*)((char*)rec + sizeof(EdgeRecord));
            sylves_free(rec);
            rec = next;
        }
    }
    sylves_free(edge_table);
    
    return map;
}

/* Count faces in mesh */
static size_t count_faces(const SylvesMeshDataEx* mesh) {
    size_t count = 0;
    for (size_t s = 0; s < mesh->submesh_count; s++) {
        SylvesFaceIterator iter;
        sylves_face_iterator_init(&iter, mesh, s);
        while (sylves_face_iterator_next(&iter)) {
            count++;
        }
    }
    return count;
}

/* Create and destroy */

SylvesDualMeshBuilder* sylves_dual_mesh_builder_create(const SylvesMeshDataEx* primal_mesh) {
    if (!primal_mesh || primal_mesh->submesh_count != 1) {
        return NULL;
    }
    
    SylvesDualMeshBuilder* builder = (SylvesDualMeshBuilder*)sylves_alloc(sizeof(SylvesDualMeshBuilder));
    if (!builder) return NULL;
    
    builder->primal_mesh = primal_mesh;
    builder->dual_mesh = NULL;
    
    builder->mappings = NULL;
    builder->mapping_count = 0;
    builder->mapping_capacity = 0;
    
    builder->face_count = count_faces(primal_mesh);
    builder->face_centroids = (int*)sylves_alloc(sizeof(int) * builder->face_count);
    builder->is_far_vertex = (bool*)sylves_alloc(sizeof(bool) * primal_mesh->vertex_count);
    
    builder->halfedge_map = build_halfedge_map(primal_mesh);
    
    builder->face_buffer = NULL;
    builder->face_buffer_capacity = 0;
    
    if (!builder->face_centroids || !builder->is_far_vertex || !builder->halfedge_map) {
        sylves_dual_mesh_builder_destroy(builder);
        return NULL;
    }
    
    /* Mark far vertices */
    for (size_t i = 0; i < primal_mesh->vertex_count; i++) {
        SylvesVector3 v = primal_mesh->vertices[i];
        float mag2 = v.x * v.x + v.y * v.y + v.z * v.z;
        builder->is_far_vertex[i] = mag2 >= FAR_THRESHOLD * FAR_THRESHOLD;
    }
    
    return builder;
}

void sylves_dual_mesh_builder_destroy(SylvesDualMeshBuilder* builder) {
    if (!builder) return;
    
    sylves_mesh_data_ex_destroy(builder->dual_mesh);
    sylves_free(builder->mappings);
    sylves_free(builder->face_centroids);
    sylves_free(builder->is_far_vertex);
    halfedge_map_destroy(builder->halfedge_map);
    sylves_free(builder->face_buffer);
    sylves_free(builder);
}

/* Configuration */

void sylves_dual_mesh_builder_set_config(
    SylvesDualMeshBuilder* builder,
    const SylvesDualMeshConfig* config) {
    /* Configuration can be extended in the future */
}

/* Add mapping entry */
static void add_mapping(
    SylvesDualMeshBuilder* builder,
    int primal_face, int primal_vert,
    int dual_face, int dual_vert) {
    
    if (builder->mapping_count >= builder->mapping_capacity) {
        size_t new_capacity = builder->mapping_capacity == 0 ? 256 : builder->mapping_capacity * 2;
        SylvesDualMapping* new_mappings = (SylvesDualMapping*)sylves_realloc(
            builder->mappings, sizeof(SylvesDualMapping) * new_capacity);
        if (!new_mappings) return;
        builder->mappings = new_mappings;
        builder->mapping_capacity = new_capacity;
    }
    
    builder->mappings[builder->mapping_count].primal_face = primal_face;
    builder->mappings[builder->mapping_count].primal_vertex = primal_vert;
    builder->mappings[builder->mapping_count].dual_face = dual_face;
    builder->mappings[builder->mapping_count].dual_vertex = dual_vert;
    builder->mapping_count++;
}

/* Ensure face buffer capacity */
static bool ensure_face_buffer(SylvesDualMeshBuilder* builder, size_t capacity) {
    if (builder->face_buffer_capacity >= capacity) {
        return true;
    }
    
    int* new_buffer = (int*)sylves_realloc(
        builder->face_buffer, sizeof(int) * capacity);
    if (!new_buffer) return false;
    
    builder->face_buffer = new_buffer;
    builder->face_buffer_capacity = capacity;
    return true;
}

/* Build dual mesh */

SylvesError sylves_dual_mesh_builder_build(SylvesDualMeshBuilder* builder) {
    if (!builder) return SYLVES_ERROR_INVALID_ARGUMENT;
    
    /* Create dual mesh */
    builder->dual_mesh = sylves_mesh_data_ex_create(
        builder->primal_mesh->vertex_count + builder->face_count, 1);
    if (!builder->dual_mesh) return SYLVES_ERROR_OUT_OF_MEMORY;
    
    /* Copy original vertices */
    memcpy(builder->dual_mesh->vertices, builder->primal_mesh->vertices,
           sizeof(SylvesVector3) * builder->primal_mesh->vertex_count);
    
    /* Add face centroids */
    int face_idx = 0;
    for (size_t s = 0; s < builder->primal_mesh->submesh_count; s++) {
        SylvesFaceIterator iter;
        sylves_face_iterator_init(&iter, builder->primal_mesh, s);
        
        while (sylves_face_iterator_next(&iter)) {
            /* Compute centroid */
            SylvesVector3 centroid = {0, 0, 0};
            for (int i = 0; i < iter.vertex_count; i++) {
                SylvesVector3 v = builder->primal_mesh->vertices[iter.face_vertices[i]];
                centroid.x += v.x;
                centroid.y += v.y;
                centroid.z += v.z;
            }
            centroid.x /= iter.vertex_count;
            centroid.y /= iter.vertex_count;
            centroid.z /= iter.vertex_count;
            
            int centroid_idx = builder->primal_mesh->vertex_count + face_idx;
            builder->dual_mesh->vertices[centroid_idx] = centroid;
            builder->face_centroids[face_idx] = centroid_idx;
            
            face_idx++;
        }
    }
    
    /* Build dual faces around each vertex */
    int* dual_indices = (int*)sylves_alloc(sizeof(int) * builder->face_count * 10);
    size_t dual_index_count = 0;
    int dual_face_count = 0;
    
    /* Track visited halfedges */
    size_t visited_size = builder->halfedge_map->entry_count;
    bool* visited = (bool*)sylves_calloc(builder->face_count * 10, sizeof(bool));
    
    /* Process arcs first (boundary), then loops (interior) */
    for (int is_arc = 1; is_arc >= 0; is_arc--) {
        face_idx = 0;
        for (size_t s = 0; s < builder->primal_mesh->submesh_count; s++) {
            SylvesFaceIterator iter;
            sylves_face_iterator_init(&iter, builder->primal_mesh, s);
            
            while (sylves_face_iterator_next(&iter)) {
                for (int e = 0; e < iter.vertex_count; e++) {
                    HalfEdgeKey start_he = {face_idx, e};
                    
                    /* Skip if not start of arc when we want arcs */
                    HalfEdgeKey flip;
                    bool has_flip = halfedge_map_find_flip(builder->halfedge_map, start_he, &flip);
                    if (is_arc && has_flip) continue;
                    
                    /* Skip if already visited */
                    size_t visited_idx = face_idx * 10 + e;
                    if (visited[visited_idx]) continue;
                    
                    /* Get vertex we're walking around */
                    int vertex = iter.face_vertices[e];
                    bool is_far = builder->is_far_vertex[vertex];
                    
                    /* Walk around vertex */
                    if (!ensure_face_buffer(builder, 100)) {
                        sylves_free(dual_indices);
                        sylves_free(visited);
                        return SYLVES_ERROR_OUT_OF_MEMORY;
                    }
                    
                    int face_vertex_count = 0;
                    HalfEdgeKey current_he = start_he;
                    HalfEdgeKey end_he = {-1, -1};
                    
                    while (true) {
                        /* Mark visited */
                        visited_idx = current_he.face * 10 + current_he.edge;
                        visited[visited_idx] = true;
                        
                        /* Add mapping */
                        add_mapping(builder, current_he.face, current_he.edge,
                                   dual_face_count, face_vertex_count + (is_arc ? 1 : 0));
                        
                        /* Add face centroid to dual face */
                        builder->face_buffer[face_vertex_count++] = builder->face_centroids[current_he.face];
                        
                        /* Move to previous edge in face */
                        int face_size = 0;
                        {
                            // Map global face index to submesh and local face index
                            int target = current_he.face;
                            int accum = 0;
                            int submesh_index = -1;
                            int face_in_submesh = -1;
                            for (size_t ss = 0; ss < builder->primal_mesh->submesh_count; ss++) {
                                SylvesFaceIterator it2;
                                sylves_face_iterator_init(&it2, builder->primal_mesh, ss);
                                int local = 0;
                                while (sylves_face_iterator_next(&it2)) {
                                    if (accum == target) {
                                        submesh_index = (int)ss;
                                        face_in_submesh = local;
                                        break;
                                    }
                                    accum++;
                                    local++;
                                }
                                if (submesh_index >= 0) break;
                            }
                            if (submesh_index >= 0) {
                                sylves_mesh_utils_get_face_vertex_count(
                                    builder->primal_mesh, submesh_index, face_in_submesh, &face_size);
                            }
                        }
                        current_he.edge = (current_he.edge + face_size - 1) % face_size;
                        
                        /* Try to flip to adjacent face */
                        if (halfedge_map_find_flip(builder->halfedge_map, current_he, &flip)) {
                            current_he = flip;
                            if (current_he.face == start_he.face && current_he.edge == start_he.edge) {
                                /* Completed loop */
                                if (is_arc) break; /* Shouldn't happen */
                                break;
                            }
                        } else {
                            /* Hit boundary */
                            if (!is_arc) break; /* Shouldn't happen */
                            end_he = current_he;
                            break;
                        }
                    }
                    
                    /* Create dual face if vertex is not at infinity */
                    if (!is_far) {
                        /* Add infinity vertices for arcs */
                        if (is_arc) {
                            /* Add start infinity vertex */
                            if (dual_index_count + face_vertex_count + 2 >= builder->face_count * 10) {
                                sylves_free(dual_indices);
                                sylves_free(visited);
                                return SYLVES_ERROR_OUT_OF_MEMORY;
                            }
                            
                            /* Compute infinity vertex at start */
                            // This is simplified - Sylves computes bisector direction
                            dual_indices[dual_index_count++] = builder->face_centroids[start_he.face];
                        }
                        
                        /* Add face vertices */
                        for (int i = 0; i < face_vertex_count; i++) {
                            dual_indices[dual_index_count++] = builder->face_buffer[i];
                        }
                        
                        /* Add infinity vertex for arcs */
                        if (is_arc) {
                            /* Compute infinity vertex at end */
                            dual_indices[dual_index_count++] = builder->face_centroids[end_he.face];
                        }
                        
                        /* Mark end of face with inverted index */
                        dual_indices[dual_index_count - 1] = ~dual_indices[dual_index_count - 1];
                        dual_face_count++;
                    }
                }
                face_idx++;
            }
        }
    }
    
    /* Set dual mesh indices */
    sylves_mesh_data_ex_set_submesh(
        builder->dual_mesh, 0, dual_indices, dual_index_count, SYLVES_MESH_TOPOLOGY_NGON);
    
    sylves_free(dual_indices);
    sylves_free(visited);
    
    return SYLVES_SUCCESS;
}

/* Get results */

const SylvesMeshDataEx* sylves_dual_mesh_builder_get_mesh(
    const SylvesDualMeshBuilder* builder) {
    return builder ? builder->dual_mesh : NULL;
}

const SylvesDualMapping* sylves_dual_mesh_builder_get_mappings(
    const SylvesDualMeshBuilder* builder,
    size_t* count) {
    if (!builder || !count) return NULL;
    *count = builder->mapping_count;
    return builder->mappings;
}



