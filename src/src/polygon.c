#include "sylves/polygon.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

SylvesError sylves_polygon_init(SylvesPolygon* polygon) {
    if (!polygon) return SYLVES_ERROR_INVALID_ARGUMENT;
    
    polygon->vertices = NULL;
    polygon->vertex_count = 0;
    polygon->capacity = 0;
    
    return SYLVES_SUCCESS;
}

void sylves_polygon_destroy(SylvesPolygon* polygon) {
    if (polygon && polygon->vertices) {
        free(polygon->vertices);
        polygon->vertices = NULL;
        polygon->vertex_count = 0;
        polygon->capacity = 0;
    }
}

SylvesError sylves_polygon_add_vertex(SylvesPolygon* polygon, SylvesVector3 vertex) {
    if (!polygon) return SYLVES_ERROR_INVALID_ARGUMENT;
    
    if (polygon->vertex_count >= polygon->capacity) {
        size_t new_capacity = polygon->capacity == 0 ? 8 : polygon->capacity * 2;
        SylvesVector3* new_vertices = (SylvesVector3*)realloc(polygon->vertices, 
                                                               new_capacity * sizeof(SylvesVector3));
        if (!new_vertices) return SYLVES_ERROR_OUT_OF_MEMORY;
        
        polygon->vertices = new_vertices;
        polygon->capacity = new_capacity;
    }
    
    polygon->vertices[polygon->vertex_count++] = vertex;
    return SYLVES_SUCCESS;
}

void sylves_polygon_clear(SylvesPolygon* polygon) {
    if (polygon) {
        polygon->vertex_count = 0;
    }
}

float sylves_polygon_area_2d(const SylvesPolygon* polygon) {
    if (!polygon || polygon->vertex_count < 3) return 0.0f;
    
    float area = 0.0f;
    for (size_t i = 0; i < polygon->vertex_count; i++) {
        size_t j = (i + 1) % polygon->vertex_count;
        area += polygon->vertices[i].x * polygon->vertices[j].y;
        area -= polygon->vertices[j].x * polygon->vertices[i].y;
    }
    
    return fabsf(area) * 0.5f;
}

float sylves_polygon_perimeter(const SylvesPolygon* polygon) {
    if (!polygon || polygon->vertex_count < 2) return 0.0f;
    
    float perimeter = 0.0f;
    for (size_t i = 0; i < polygon->vertex_count; i++) {
        size_t j = (i + 1) % polygon->vertex_count;
        SylvesVector3 diff = sylves_vector3_subtract(polygon->vertices[j], polygon->vertices[i]);
        perimeter += (float)sylves_vector3_length(diff);
    }
    
    return perimeter;
}

SylvesVector3 sylves_polygon_centroid(const SylvesPolygon* polygon) {
    SylvesVector3 centroid = {0, 0, 0};
    
    if (!polygon || polygon->vertex_count == 0) return centroid;
    
    for (size_t i = 0; i < polygon->vertex_count; i++) {
        centroid = sylves_vector3_add(centroid, polygon->vertices[i]);
    }
    
    float inv_count = 1.0f / (float)polygon->vertex_count;
    return sylves_vector3_scale(centroid, inv_count);
}

int sylves_polygon_contains_point_2d(const SylvesPolygon* polygon, SylvesVector2 point) {
    if (!polygon || polygon->vertex_count < 3) return 0;
    
    int inside = 0;
    for (size_t i = 0, j = polygon->vertex_count - 1; i < polygon->vertex_count; j = i++) {
        float xi = polygon->vertices[i].x, yi = polygon->vertices[i].y;
        float xj = polygon->vertices[j].x, yj = polygon->vertices[j].y;
        
        int intersect = ((yi > point.y) != (yj > point.y)) &&
                       (point.x < (xj - xi) * (point.y - yi) / (yj - yi) + xi);
        if (intersect) inside = !inside;
    }
    
    return inside;
}
