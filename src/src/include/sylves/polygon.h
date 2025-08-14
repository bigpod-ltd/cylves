/**
 * @file polygon.h
 * @brief Polygon data structure and operations
 */

#ifndef SYLVES_POLYGON_H
#define SYLVES_POLYGON_H

#include "types.h"
#include "vector.h"
#include "errors.h"

#ifdef __cplusplus
extern "C" {
#endif

// Polygon structure to hold vertex data
typedef struct SylvesPolygon {
    SylvesVector3* vertices;
    size_t vertex_count;
    size_t capacity;
} SylvesPolygon;

// Initialize polygon structure
SylvesError sylves_polygon_init(SylvesPolygon* polygon);

// Destroy polygon and free memory
void sylves_polygon_destroy(SylvesPolygon* polygon);

// Add vertex to polygon
SylvesError sylves_polygon_add_vertex(SylvesPolygon* polygon, SylvesVector3 vertex);

// Clear all vertices from polygon
void sylves_polygon_clear(SylvesPolygon* polygon);

// Get polygon area (2D)
float sylves_polygon_area_2d(const SylvesPolygon* polygon);

// Get polygon perimeter
float sylves_polygon_perimeter(const SylvesPolygon* polygon);

// Get polygon centroid
SylvesVector3 sylves_polygon_centroid(const SylvesPolygon* polygon);

// Check if point is inside polygon (2D)
int sylves_polygon_contains_point_2d(const SylvesPolygon* polygon, SylvesVector2 point);

#ifdef __cplusplus
}
#endif

#endif /* SYLVES_POLYGON_H */
