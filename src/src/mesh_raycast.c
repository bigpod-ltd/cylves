/**
 * @file mesh_raycast.c
 * @brief Implementation of mesh raycasting and intersection algorithms
 */

/* removed missing header include */
#include "sylves/mesh_data.h"
#include "sylves/memory.h"
#include "sylves/errors.h"
#include <string.h>
#include <math.h>

#define EPSILON 1e-7f

/**
 * @brief Ray-triangle intersection using Moller-Trumbore algorithm
 */
bool sylves_raycast_triangle(
    const SylvesVector3* origin,
    const SylvesVector3* direction,
    const SylvesVector3* v0,
    const SylvesVector3* v1,
    const SylvesVector3* v2,
    SylvesVector3* out_intersection,
    float* out_distance) {

    SylvesVector3 edge1, edge2, h, s, q;
    float a,f,u,v;

    edge1.x = v1->x - v0->x;
    edge1.y = v1->y - v0->y;
    edge1.z = v1->z - v0->z;

    edge2.x = v2->x - v0->x;
    edge2.y = v2->y - v0->y;
    edge2.z = v2->z - v0->z;

    h.x = direction->y * edge2.z - direction->z * edge2.y;
    h.y = direction->z * edge2.x - direction->x * edge2.z;
    h.z = direction->x * edge2.y - direction->y * edge2.x;

    a = edge1.x * h.x + edge1.y * h.y + edge1.z * h.z;

    if (fabs(a) < EPSILON) {
        return false; // This means the ray is parallel to the triangle.
    }

    f = 1.0f / a;
    s.x = origin->x - v0->x;
    s.y = origin->y - v0->y;
    s.z = origin->z - v0->z;

    u = f * (s.x * h.x + s.y * h.y + s.z * h.z);

    if (u < 0.0f || u > 1.0f) {
        return false;
    }

    q.x = s.y * edge1.z - s.z * edge1.y;
    q.y = s.z * edge1.x - s.x * edge1.z;
    q.z = s.x * edge1.y - s.y * edge1.x;

    v = f * (direction->x * q.x + direction->y * q.y + direction->z * q.z);

    if (v < 0.0f || u + v > 1.0f) {
        return false;
    }

    // Compute the distance along the ray to the intersection point
    float t = f * (edge2.x * q.x + edge2.y * q.y + edge2.z * q.z);

    if (t < EPSILON) {
        return false; // This means that there is a line intersection but not a ray intersection.
    }

    if (out_intersection) {
        out_intersection->x = origin->x + direction->x * t;
        out_intersection->y = origin->y + direction->y * t;
        out_intersection->z = origin->z + direction->z * t;
    }

    if (out_distance) {
        *out_distance = t;
    }

    return true;
}

/**
 * @brief Raycast against a mesh
 *
 * @param mesh The mesh to raycast against
 * @param origin The origin of the ray
 * @param direction The direction of the ray
 * @param[out] out_intersection The intersection point
 * @param[out] out_distance The distance to the intersection point
 * @param[out] out_submesh The submesh index of the intersection
 * @param[out] out_face The face index in the submesh
 * @return True if the ray intersects with the mesh
 */
bool sylves_mesh_raycast(
    const SylvesMeshDataEx* mesh,
    const SylvesVector3* origin,
    const SylvesVector3* direction,
    SylvesVector3* out_intersection,
    float* out_distance,
    int* out_submesh,
    int* out_face) {

    bool hit = false;
    float closest_distance = INFINITY;

    for (size_t s = 0; s < mesh->submesh_count; s++) {
        SylvesFaceIterator iter;
        sylves_face_iterator_init(&iter, mesh, s);

        int face_idx = 0;
        while (sylves_face_iterator_next(&iter)) {
            if (iter.vertex_count < 3) continue;
            // Assume triangulated face for now
            SylvesVector3 intersection;
            float distance;

            bool intersected = sylves_raycast_triangle(origin, direction,
                                                       &mesh->vertices[iter.face_vertices[0]],
                                                       &mesh->vertices[iter.face_vertices[1]],
                                                       &mesh->vertices[iter.face_vertices[2]],
                                                       &intersection, &distance);

            if (intersected && distance < closest_distance) {
                closest_distance = distance;
                hit = true;
                if (out_intersection) *out_intersection = intersection;
                if (out_distance) *out_distance = distance;
                if (out_submesh) *out_submesh = s;
                if (out_face) *out_face = face_idx;
            }
            face_idx++;
        }
    }

    return hit;
}

// Additional utility functions and spatial acceleration structures could be implemented here.

/**
 * @brief Placeholder for spatial acceleration structure
 */
typedef struct SylvesSpatialAcceleration {} SylvesSpatialAcceleration;

/**
 * @brief Initialize spatial acceleration
 */
SylvesSpatialAcceleration* sylves_spatial_acceleration_init(const SylvesMeshDataEx* mesh) {
    return NULL; // Placeholder
}

/**
 * @brief Destroy spatial acceleration
 */
void sylves_spatial_acceleration_destroy(SylvesSpatialAcceleration* sa) {
    // Placeholder
} 

/**
 * @brief Raycast with spatial acceleration
 *
 * This function should use the spatial acceleration structure to speed up raycasting.
 */
bool sylves_mesh_raycast_accelerated(
    const SylvesMeshDataEx* mesh,
    const SylvesVector3* origin,
    const SylvesVector3* direction,
    SylvesVector3* out_intersection,
    float* out_distance,
    int* out_submesh,
    int* out_face,
    const SylvesSpatialAcceleration* sa) {

    // Use the SA to quickly find potential candidates
    return sylves_mesh_raycast(mesh, origin, direction, out_intersection, out_distance, out_submesh, out_face); // Placeholder
} 

/**
 * @brief Build spatial acceleration
 *
 * Generate the spatial acceleration structure from the mesh.
 */
SylvesError sylves_build_spatial_acceleration(
    const SylvesMeshDataEx* mesh,
    SylvesSpatialAcceleration* sa) {
    return SYLVES_SUCCESS; // Placeholder
} 

/* Additional functions for spatial partitioning, such as k-d trees or bounding volume hierarchies, could be added here.*/ 

/**
 * @brief Clean up any resources used by the mesh raycasting module.
 */
void sylves_mesh_raycast_cleanup() {
    // Placeholder
} 

/**
 * @brief Initialize any resources needed by the mesh raycasting module.
 */
void sylves_mesh_raycast_init() {
    // Placeholder
} 

/** Functionality for bounding box or sphere tests could be extended here.*/

/** Expose other interfaces needed to extend the mesh ray casting to height maps or other primitives.*/ 

// Placeholder end.


/****** End of mesh_raycast.c ******/
 
