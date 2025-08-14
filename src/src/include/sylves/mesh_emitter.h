/**
 * @file mesh_emitter.h
 * @brief Public declarations for mesh emitter utilities
 */
#ifndef SYLVES_MESH_EMITTER_H
#define SYLVES_MESH_EMITTER_H

#include "mesh_data.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SylvesMeshEmitter SylvesMeshEmitter;

SylvesMeshEmitter* sylves_mesh_emitter_create(const SylvesMeshDataEx* original_mesh);
void sylves_mesh_emitter_destroy(SylvesMeshEmitter* emitter);

void sylves_mesh_emitter_copy_vertices(SylvesMeshEmitter* emitter);
void sylves_mesh_emitter_start_submesh(SylvesMeshEmitter* emitter, SylvesMeshTopology topology);

int sylves_mesh_emitter_add_vertex(SylvesMeshEmitter* emitter,
                                   const SylvesVector3* position,
                                   const SylvesVector2* uv,
                                   const SylvesVector3* normal,
                                   const SylvesVector4* tangent);

void sylves_mesh_emitter_add_face3(SylvesMeshEmitter* emitter, int i0, int i1, int i2);
void sylves_mesh_emitter_add_face4(SylvesMeshEmitter* emitter, int i0, int i1, int i2, int i3);

int sylves_mesh_emitter_average_vertices(SylvesMeshEmitter* emitter, int i0, int i1);
int sylves_mesh_emitter_average_face(SylvesMeshEmitter* emitter, const int* indices, size_t count);

void sylves_mesh_emitter_end_submesh(SylvesMeshEmitter* emitter);

SylvesMeshDataEx* sylves_mesh_emitter_to_mesh(SylvesMeshEmitter* emitter);

#ifdef __cplusplus
}
#endif

#endif /* SYLVES_MESH_EMITTER_H */
