/**
 * @file voronoi_grid.c
 * @brief Voronoi grid implementation
 */

#include "sylves/voronoi_grid.h"
#include "sylves/mesh_grid.h"
#include "sylves/mesh.h"
#include "sylves/memory.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

/* Simple Delaunay triangulation for Voronoi construction */
typedef struct {
    int p0, p1, p2;  /* Indices of triangle vertices */
    double cx, cy;   /* Circumcenter */
    double r2;       /* Squared circumradius */
} Triangle;

typedef struct {
    int p0, p1;      /* Edge vertices */
    int tri;         /* Associated triangle */
} Edge;

/* Helper to compute circumcircle of a triangle */
static void compute_circumcircle(const SylvesVector2* points, int p0, int p1, int p2,
                                double* cx, double* cy, double* r2) {
    double ax = points[p0].x, ay = points[p0].y;
    double bx = points[p1].x, by = points[p1].y;
    double cx_pt = points[p2].x, cy_pt = points[p2].y;
    
    double d = 2.0 * (ax * (by - cy_pt) + bx * (cy_pt - ay) + cx_pt * (ay - by));
    
    if (fabs(d) < DBL_EPSILON) {
        /* Degenerate triangle */
        *cx = (ax + bx + cx_pt) / 3.0;
        *cy = (ay + by + cy_pt) / 3.0;
        *r2 = DBL_MAX;
        return;
    }
    
    double a2 = ax * ax + ay * ay;
    double b2 = bx * bx + by * by;
    double c2 = cx_pt * cx_pt + cy_pt * cy_pt;
    
    *cx = (a2 * (by - cy_pt) + b2 * (cy_pt - ay) + c2 * (ay - by)) / d;
    *cy = (a2 * (cx_pt - bx) + b2 * (ax - cx_pt) + c2 * (bx - ax)) / d;
    
    double dx = ax - *cx;
    double dy = ay - *cy;
    *r2 = dx * dx + dy * dy;
}

/* Check if a point is inside a triangle's circumcircle */
static bool in_circumcircle(const Triangle* tri, double px, double py) {
    double dx = px - tri->cx;
    double dy = py - tri->cy;
    return (dx * dx + dy * dy) < tri->r2;
}

/* Super simple Bowyer-Watson triangulation - not optimal but works for small point sets */
static void triangulate(const SylvesVector2* points, size_t num_points,
                       Triangle** triangles, size_t* num_triangles) {
    /* Start with super-triangle containing all points */
    double min_x = DBL_MAX, min_y = DBL_MAX;
    double max_x = -DBL_MAX, max_y = -DBL_MAX;
    
    for (size_t i = 0; i < num_points; i++) {
        if (points[i].x < min_x) min_x = points[i].x;
        if (points[i].y < min_y) min_y = points[i].y;
        if (points[i].x > max_x) max_x = points[i].x;
        if (points[i].y > max_y) max_y = points[i].y;
    }
    
    double dx = max_x - min_x;
    double dy = max_y - min_y;
    double delta_max = fmax(dx, dy);
    double mid_x = (min_x + max_x) / 2.0;
    double mid_y = (min_y + max_y) / 2.0;
    
    /* Create super-triangle vertices */
    SylvesVector2* all_points = malloc((num_points + 3) * sizeof(SylvesVector2));
    memcpy(all_points, points, num_points * sizeof(SylvesVector2));
    
    /* Super-triangle vertices */
    all_points[num_points].x = mid_x - 20 * delta_max;
    all_points[num_points].y = mid_y - delta_max;
    all_points[num_points + 1].x = mid_x;
    all_points[num_points + 1].y = mid_y + 20 * delta_max;
    all_points[num_points + 2].x = mid_x + 20 * delta_max;
    all_points[num_points + 2].y = mid_y - delta_max;
    
    /* Initialize triangulation with super-triangle */
    size_t tri_capacity = num_points * 3;
    *triangles = malloc(tri_capacity * sizeof(Triangle));
    *num_triangles = 1;
    
    Triangle* tris = *triangles;
    tris[0].p0 = num_points;
    tris[0].p1 = num_points + 1;
    tris[0].p2 = num_points + 2;
    compute_circumcircle(all_points, tris[0].p0, tris[0].p1, tris[0].p2,
                        &tris[0].cx, &tris[0].cy, &tris[0].r2);
    
    /* Add points one by one */
    for (size_t i = 0; i < num_points; i++) {
        double px = points[i].x;
        double py = points[i].y;
        
        /* Find triangles whose circumcircle contains this point */
        Edge* polygon = malloc(num_points * 3 * sizeof(Edge));
        size_t poly_size = 0;
        
        size_t j = 0;
        while (j < *num_triangles) {
            if (in_circumcircle(&tris[j], px, py)) {
                /* Add triangle edges to polygon */
                Edge e0 = {tris[j].p0, tris[j].p1, j};
                Edge e1 = {tris[j].p1, tris[j].p2, j};
                Edge e2 = {tris[j].p2, tris[j].p0, j};
                
                /* Remove triangle by swapping with last */
                tris[j] = tris[*num_triangles - 1];
                (*num_triangles)--;
                
                /* Add edges, removing duplicates */
                Edge edges[3] = {e0, e1, e2};
                for (int k = 0; k < 3; k++) {
                    bool duplicate = false;
                    for (size_t m = 0; m < poly_size; m++) {
                        if ((edges[k].p0 == polygon[m].p1 && edges[k].p1 == polygon[m].p0) ||
                            (edges[k].p0 == polygon[m].p0 && edges[k].p1 == polygon[m].p1)) {
                            /* Remove duplicate edge */
                            polygon[m] = polygon[poly_size - 1];
                            poly_size--;
                            duplicate = true;
                            break;
                        }
                    }
                    if (!duplicate) {
                        polygon[poly_size++] = edges[k];
                    }
                }
            } else {
                j++;
            }
        }
        
        /* Create new triangles from polygon */
        for (size_t j = 0; j < poly_size; j++) {
            if (*num_triangles >= tri_capacity) {
                tri_capacity *= 2;
                *triangles = realloc(*triangles, tri_capacity * sizeof(Triangle));
                tris = *triangles;
            }
            
            tris[*num_triangles].p0 = polygon[j].p0;
            tris[*num_triangles].p1 = polygon[j].p1;
            tris[*num_triangles].p2 = i;
            compute_circumcircle(all_points, 
                               tris[*num_triangles].p0,
                               tris[*num_triangles].p1, 
                               tris[*num_triangles].p2,
                               &tris[*num_triangles].cx,
                               &tris[*num_triangles].cy,
                               &tris[*num_triangles].r2);
            (*num_triangles)++;
        }
        
        free(polygon);
    }
    
    /* Remove triangles connected to super-triangle */
    size_t j = 0;
    while (j < *num_triangles) {
        if (tris[j].p0 >= (int)num_points || 
            tris[j].p1 >= (int)num_points || 
            tris[j].p2 >= (int)num_points) {
            tris[j] = tris[*num_triangles - 1];
            (*num_triangles)--;
        } else {
            j++;
        }
    }
    
    free(all_points);
}

/* Apply Lloyd relaxation to points */
static void lloyd_relaxation(SylvesVector2* points, size_t num_points,
                           const SylvesVoronoiGridOptions* options) {
    Triangle* triangles = NULL;
    size_t num_triangles = 0;
    
    /* Compute Delaunay triangulation */
    triangulate(points, num_points, &triangles, &num_triangles);
    
    /* For each point, find its Voronoi cell centroid */
    SylvesVector2* new_points = malloc(num_points * sizeof(SylvesVector2));
    
    for (size_t i = 0; i < num_points; i++) {
        double cx = 0, cy = 0;
        int count = 0;
        
        /* Find all triangles adjacent to this point */
        for (size_t j = 0; j < num_triangles; j++) {
            if (triangles[j].p0 == (int)i || 
                triangles[j].p1 == (int)i || 
                triangles[j].p2 == (int)i) {
                cx += triangles[j].cx;
                cy += triangles[j].cy;
                count++;
            }
        }
        
        if (count > 0) {
            new_points[i].x = cx / count;
            new_points[i].y = cy / count;
            
            /* Apply clipping if specified */
            if (options->clip_min && options->clip_max) {
                if (new_points[i].x < options->clip_min->x) 
                    new_points[i].x = options->clip_min->x;
                if (new_points[i].y < options->clip_min->y) 
                    new_points[i].y = options->clip_min->y;
                if (new_points[i].x > options->clip_max->x) 
                    new_points[i].x = options->clip_max->x;
                if (new_points[i].y > options->clip_max->y) 
                    new_points[i].y = options->clip_max->y;
            }
            
            /* Pin border points if requested */
            if (options->pin_border_during_relaxation && options->clip_min && options->clip_max) {
                double eps = 1e-6;
                bool on_border = (fabs(points[i].x - options->clip_min->x) < eps ||
                                 fabs(points[i].x - options->clip_max->x) < eps ||
                                 fabs(points[i].y - options->clip_min->y) < eps ||
                                 fabs(points[i].y - options->clip_max->y) < eps);
                if (on_border) {
                    new_points[i] = points[i];
                }
            }
        } else {
            new_points[i] = points[i];
        }
    }
    
    /* Copy back */
    memcpy(points, new_points, num_points * sizeof(SylvesVector2));
    
    free(new_points);
    free(triangles);
}

/* Create mesh data from Voronoi diagram */
static SylvesMeshData* create_voronoi_mesh(const SylvesVector2* points, size_t num_points,
                                          const SylvesVoronoiGridOptions* options) {
    Triangle* triangles = NULL;
    size_t num_triangles = 0;
    
    /* Compute Delaunay triangulation */
    triangulate(points, num_points, &triangles, &num_triangles);
    
    /* Estimate mesh data size - worst case each point has as many vertices as triangles */
    size_t max_vertices = num_triangles * 3;  /* Overestimate */
    size_t max_faces = num_points;  /* One face per Voronoi cell */
    
    /* Create mesh data */
    SylvesMeshData* mesh_data = sylves_mesh_data_create(max_vertices, max_faces);
    if (!mesh_data) {
        free(triangles);
        return NULL;
    }
    
    /* Track actual vertex count */
    size_t total_vertices = 0;
    
    /* For each point, create its Voronoi cell */
    for (size_t i = 0; i < num_points; i++) {
        /* Collect circumcenters of adjacent triangles */
        SylvesVector3* cell_vertices = NULL;
        size_t num_verts = 0;
        size_t vert_capacity = 10;
        cell_vertices = malloc(vert_capacity * sizeof(SylvesVector3));
        
        /* Find all triangles adjacent to this point */
        for (size_t j = 0; j < num_triangles; j++) {
            if (triangles[j].p0 == (int)i || 
                triangles[j].p1 == (int)i || 
                triangles[j].p2 == (int)i) {
                
                /* Add circumcenter as vertex */
                if (num_verts >= vert_capacity) {
                    vert_capacity *= 2;
                    cell_vertices = realloc(cell_vertices, vert_capacity * sizeof(SylvesVector3));
                }
                
                cell_vertices[num_verts].x = triangles[j].cx;
                cell_vertices[num_verts].y = triangles[j].cy;
                cell_vertices[num_verts].z = 0;
                
                /* Apply clipping if needed */
                if (options->clip_min && options->clip_max) {
                    if (cell_vertices[num_verts].x < options->clip_min->x)
                        cell_vertices[num_verts].x = options->clip_min->x;
                    if (cell_vertices[num_verts].y < options->clip_min->y)
                        cell_vertices[num_verts].y = options->clip_min->y;
                    if (cell_vertices[num_verts].x > options->clip_max->x)
                        cell_vertices[num_verts].x = options->clip_max->x;
                    if (cell_vertices[num_verts].y > options->clip_max->y)
                        cell_vertices[num_verts].y = options->clip_max->y;
                }
                
                num_verts++;
            }
        }
        
        /* Sort vertices by angle to create proper polygon */
        if (num_verts > 0) {
            double cx = points[i].x;
            double cy = points[i].y;
            
            /* Simple bubble sort by angle */
            for (size_t j = 0; j < num_verts - 1; j++) {
                for (size_t k = j + 1; k < num_verts; k++) {
                    double angle1 = atan2(cell_vertices[j].y - cy, cell_vertices[j].x - cx);
                    double angle2 = atan2(cell_vertices[k].y - cy, cell_vertices[k].x - cx);
                    if (angle2 < angle1) {
                        SylvesVector3 temp = cell_vertices[j];
                        cell_vertices[j] = cell_vertices[k];
                        cell_vertices[k] = temp;
                    }
                }
            }
            
            /* TODO: Add face to mesh - need to implement proper face creation */
            /* For now, just store vertices */
            if (total_vertices + num_verts <= mesh_data->vertex_count) {
                for (size_t j = 0; j < num_verts; j++) {
                    mesh_data->vertices[total_vertices + j] = cell_vertices[j];
                }
                
                /* Create face */
                if (i < mesh_data->face_count) {
                    mesh_data->faces[i].vertex_count = num_verts;
                    mesh_data->faces[i].vertices = malloc(num_verts * sizeof(int));
                    mesh_data->faces[i].neighbors = malloc(num_verts * sizeof(int));
                    for (size_t j = 0; j < num_verts; j++) {
                        mesh_data->faces[i].vertices[j] = total_vertices + j;
                        mesh_data->faces[i].neighbors[j] = -1;  /* No neighbors for now */
                    }
                }
                
                total_vertices += num_verts;
            }
        }
        
        free(cell_vertices);
    }
    
    free(triangles);
    return mesh_data;
}

SylvesVoronoiGridOptions sylves_voronoi_grid_options_default(void) {
    SylvesVoronoiGridOptions options = {
        .clip_min = NULL,
        .clip_max = NULL,
        .lloyd_relaxation_iterations = 0,
        .pin_border_during_relaxation = true
    };
    return options;
}

SylvesGrid* sylves_voronoi_grid_create(const SylvesVector2* points, size_t num_points,
                                      const SylvesVoronoiGridOptions* options) {
    if (!points || num_points < 3) {
        return NULL;
    }
    
    /* Use default options if none provided */
    SylvesVoronoiGridOptions default_opts = sylves_voronoi_grid_options_default();
    if (!options) {
        options = &default_opts;
    }
    
    /* Copy points for modification */
    SylvesVector2* work_points = malloc(num_points * sizeof(SylvesVector2));
    memcpy(work_points, points, num_points * sizeof(SylvesVector2));
    
    /* Apply Lloyd relaxation if requested */
    for (int i = 0; i < options->lloyd_relaxation_iterations; i++) {
        lloyd_relaxation(work_points, num_points, options);
    }
    
    /* Create mesh data */
    SylvesMeshData* mesh_data = create_voronoi_mesh(work_points, num_points, options);
    free(work_points);
    
    if (!mesh_data) {
        return NULL;
    }
    
    /* Create mesh grid from Voronoi mesh */
    SylvesGrid* grid = sylves_mesh_grid_create(mesh_data);
    sylves_mesh_data_destroy(mesh_data);
    
    return grid;
}
