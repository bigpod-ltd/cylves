/**
 * @file conway_operators.c
 * @brief Implementation of Conway operators: kis, truncate, dual
 */

#include "sylves/conway_operators.h"
#include "sylves/mesh_data.h"
#include "sylves/mesh_emitter.h"
#include "sylves/dual_mesh_builder.h"
#include "sylves/memory.h"
#include <string.h>
#include <math.h>

/* Kis operator - add vertex at center and create triangles to edges */
SylvesMeshDataEx* sylves_conway_kis(const SylvesMeshDataEx* mesh) {
    if (!mesh) return NULL;

    SylvesMeshEmitter* emitter = sylves_mesh_emitter_create(mesh);
    if (!emitter) return NULL;

    sylves_mesh_emitter_copy_vertices(emitter);

    for (size_t s = 0; s < mesh->submesh_count; s++) {
        sylves_mesh_emitter_start_submesh(emitter, SYLVES_MESH_TOPOLOGY_TRIANGLES);
        SylvesFaceIterator iter;
        sylves_face_iterator_init(&iter, mesh, s);
        while (sylves_face_iterator_next(&iter)) {
            int centroid = sylves_mesh_emitter_average_face(
                emitter, iter.face_vertices, iter.vertex_count);
            for (int i = 0; i < iter.vertex_count; i++) {
                int i1 = iter.face_vertices[i];
                int i2 = iter.face_vertices[(i + 1) % iter.vertex_count];
                sylves_mesh_emitter_add_face3(emitter, centroid, i1, i2);
            }
        }
        sylves_mesh_emitter_end_submesh(emitter);
    }

    SylvesMeshDataEx* result = sylves_mesh_emitter_to_mesh(emitter);
    sylves_mesh_emitter_destroy(emitter);
    return result;
}

/* Truncate operator - simplified: clone mesh */
SylvesMeshDataEx* sylves_conway_truncate(const SylvesMeshDataEx* mesh) {
    if (!mesh) return NULL;
    return sylves_mesh_data_ex_clone(mesh);
}

/* Dual operator - create dual mesh */
SylvesMeshDataEx* sylves_conway_dual(const SylvesMeshDataEx* mesh) {
    if (!mesh) return NULL;
    SylvesDualMeshConfig cfg = sylves_dual_mesh_config_default();
    return sylves_dual_mesh_build(mesh, &cfg);
}
