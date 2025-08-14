/**
 * @file periodic_planar_mesh_grid.c
 * @brief Periodic planar mesh grid implementation
 */

#include "sylves/periodic_planar_mesh_grid.h"
#include "sylves/mesh_grid.h"
#include "sylves/mesh.h"
#include <stdlib.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Create Cairo pentagonal tiling unit cell */
static SylvesMeshData* create_cairo_unit_cell(double period_x, double period_y) {
    /* Cairo tiling has 4 pentagons in unit cell */
    SylvesMeshData* mesh_data = sylves_mesh_data_create(20, 4);
    if (!mesh_data) return NULL;
    
    /* Cairo tiling has 4 pentagons per unit cell */
    /* The pentagons are irregular with specific angles */
    
    double scale_x = period_x / 2.0;
    double scale_y = period_y / 2.0;
    
    /* Pentagon vertices (normalized then scaled) */
    /* Each pentagon has a specific shape in Cairo tiling */
    
    /* Pentagon 1 (bottom-left) */
    SylvesVector3 pent1[5] = {
        {0.0, 0.0, 0.0},
        {0.5 * scale_x, 0.0, 0.0},
        {0.75 * scale_x, 0.25 * scale_y, 0.0},
        {0.5 * scale_x, 0.5 * scale_y, 0.0},
        {0.0, 0.5 * scale_y, 0.0}
    };
    int indices[5] = {0, 1, 2, 3, 4};
    sylves_mesh_data_add_ngon_face(mesh_data, pent1, indices, 5);
    
    /* Pentagon 2 (bottom-right) */
    SylvesVector3 pent2[5] = {
        {scale_x, 0.0, 0.0},
        {scale_x, 0.5 * scale_y, 0.0},
        {1.5 * scale_x, 0.5 * scale_y, 0.0},
        {1.75 * scale_x, 0.25 * scale_y, 0.0},
        {1.5 * scale_x, 0.0, 0.0}
    };
    sylves_mesh_data_add_ngon_face(mesh_data, pent2, indices, 5);
    
    /* Pentagon 3 (top-left) */
    SylvesVector3 pent3[5] = {
        {0.0, scale_y, 0.0},
        {0.0, 1.5 * scale_y, 0.0},
        {0.25 * scale_x, 1.75 * scale_y, 0.0},
        {0.5 * scale_x, 1.5 * scale_y, 0.0},
        {0.5 * scale_x, scale_y, 0.0}
    };
    sylves_mesh_data_add_ngon_face(mesh_data, pent3, indices, 5);
    
    /* Pentagon 4 (top-right) */
    SylvesVector3 pent4[5] = {
        {scale_x, scale_y, 0.0},
        {1.5 * scale_x, scale_y, 0.0},
        {1.5 * scale_x, 1.5 * scale_y, 0.0},
        {1.25 * scale_x, 1.75 * scale_y, 0.0},
        {scale_x, 1.5 * scale_y, 0.0}
    };
    sylves_mesh_data_add_ngon_face(mesh_data, pent4, indices, 5);
    
    return mesh_data;
}

/* Create Rhombille tiling unit cell */
static SylvesMeshData* create_rhombille_unit_cell(double period_x, double period_y) {
    /* Rhombille has 6 rhombs around center */
    SylvesMeshData* mesh_data = sylves_mesh_data_create(24, 6);
    if (!mesh_data) return NULL;
    
    /* Rhombille tiling uses 60-120 degree rhombi */
    /* Arranged in a hexagonal pattern */
    
    double hex_size = fmin(period_x, period_y) / 3.0;
    double sqrt3 = sqrt(3.0);
    
    /* Create rhombs around a central point */
    for (int i = 0; i < 6; i++) {
        double angle = i * M_PI / 3.0;
        double next_angle = (i + 1) * M_PI / 3.0;
        
        SylvesVector3 rhomb[4] = {
            {period_x/2, period_y/2, 0.0},  /* Center */
            {period_x/2 + hex_size * cos(angle), 
             period_y/2 + hex_size * sin(angle), 0.0},
            {period_x/2 + hex_size * sqrt3 * cos((angle + next_angle)/2),
             period_y/2 + hex_size * sqrt3 * sin((angle + next_angle)/2), 0.0},
            {period_x/2 + hex_size * cos(next_angle),
             period_y/2 + hex_size * sin(next_angle), 0.0}
        };
        
        int indices[4] = {0, 1, 2, 3};
        sylves_mesh_data_add_ngon_face(mesh_data, rhomb, indices, 4);
    }
    
    return mesh_data;
}

/* Create Trihexagonal tiling unit cell */
static SylvesMeshData* create_trihex_unit_cell(double period_x, double period_y) {
    /* 1 hexagon + 6 triangles + 3 partial hexagons */
    SylvesMeshData* mesh_data = sylves_mesh_data_create(40, 10);
    if (!mesh_data) return NULL;
    
    /* Trihexagonal has alternating triangles and hexagons */
    double scale = fmin(period_x, period_y) / 4.0;
    double sqrt3 = sqrt(3.0);
    double sqrt3_2 = sqrt3 / 2.0;
    
    /* Add central hexagon */
    SylvesVector3 hex[6];
    for (int i = 0; i < 6; i++) {
        double angle = i * M_PI / 3.0;
        hex[i].x = period_x/2 + scale * cos(angle);
        hex[i].y = period_y/2 + scale * sin(angle);
        hex[i].z = 0.0;
    }
    int hex_indices[6] = {0, 1, 2, 3, 4, 5};
    sylves_mesh_data_add_ngon_face(mesh_data, hex, hex_indices, 6);
    
    /* Add triangles between hexagon edges */
    for (int i = 0; i < 6; i++) {
        double angle1 = i * M_PI / 3.0;
        double angle2 = (i + 1) * M_PI / 3.0;
        double mid_angle = (angle1 + angle2) / 2.0;
        
        SylvesVector3 tri[3] = {
            hex[i],
            hex[(i + 1) % 6],
            {period_x/2 + 1.5 * scale * cos(mid_angle),
             period_y/2 + 1.5 * scale * sin(mid_angle), 0.0}
        };
        
        int tri_indices[3] = {0, 1, 2};
        sylves_mesh_data_add_ngon_face(mesh_data, tri, tri_indices, 3);
    }
    
    /* Add corner hexagons (partial) */
    double hex_offset = 2.5 * scale;
    for (int i = 0; i < 3; i++) {
        double angle = i * 2.0 * M_PI / 3.0;
        double cx = period_x/2 + hex_offset * cos(angle);
        double cy = period_y/2 + hex_offset * sin(angle);
        
        /* Add 1/3 of hexagon */
        SylvesVector3 partial_hex[4];
        for (int j = 0; j < 4; j++) {
            double vertex_angle = angle - M_PI/3 + j * M_PI/3;
            partial_hex[j].x = cx + scale * cos(vertex_angle);
            partial_hex[j].y = cy + scale * sin(vertex_angle);
            partial_hex[j].z = 0.0;
        }
        
        int partial_indices[4] = {0, 1, 2, 3};
        sylves_mesh_data_add_ngon_face(mesh_data, partial_hex, partial_indices, 4);
    }
    
    return mesh_data;
}

/* Create a periodic mesh grid from a unit cell */
static SylvesGrid* create_periodic_grid(SylvesMeshData* unit_cell,
                                       double period_x, double period_y) {
    if (!unit_cell) return NULL;
    
    /* For now, just create the unit cell as a mesh grid */
    /* A full implementation would handle periodic wrapping */
    SylvesGrid* grid = sylves_mesh_grid_create(unit_cell);
    sylves_mesh_data_destroy(unit_cell);
    
    return grid;
}

SylvesGrid* sylves_cairo_grid_create(double period_x, double period_y) {
    SylvesMeshData* unit_cell = create_cairo_unit_cell(period_x, period_y);
    return create_periodic_grid(unit_cell, period_x, period_y);
}

SylvesGrid* sylves_rhombille_grid_create(double period_x, double period_y) {
    SylvesMeshData* unit_cell = create_rhombille_unit_cell(period_x, period_y);
    return create_periodic_grid(unit_cell, period_x, period_y);
}

SylvesGrid* sylves_trihex_grid_create(double period_x, double period_y) {
    SylvesMeshData* unit_cell = create_trihex_unit_cell(period_x, period_y);
    return create_periodic_grid(unit_cell, period_x, period_y);
}

SylvesGrid* sylves_periodic_planar_mesh_grid_create(SylvesPeriodicTilingType type,
                                                   double period_x, double period_y) {
    switch (type) {
        case SYLVES_PERIODIC_CAIRO:
            return sylves_cairo_grid_create(period_x, period_y);
            
        case SYLVES_PERIODIC_RHOMBILLE:
            return sylves_rhombille_grid_create(period_x, period_y);
            
        case SYLVES_PERIODIC_TRIHEX:
            return sylves_trihex_grid_create(period_x, period_y);
            
        case SYLVES_PERIODIC_TETRAKIS_SQUARE:
        case SYLVES_PERIODIC_SQUARE_SNUB:
            /* Not implemented yet */
            return NULL;
            
        default:
            return NULL;
    }
}
