/**
 * @file substitution_tiling_grid.c
 * @brief Substitution tiling grid implementation
 */

#include "sylves/substitution_tiling_grid.h"
#include "sylves/mesh_grid.h"
#include "sylves/mesh.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Golden ratio */
#define PHI ((1.0 + sqrt(5.0)) / 2.0)

/* Tile types for Penrose rhombs */
typedef enum {
    PENROSE_THIN,  /* 36-144 degree rhomb */
    PENROSE_THICK  /* 72-108 degree rhomb */
} PenroseTileType;

/* A tile in the substitution tiling */
typedef struct {
    double x, y;      /* Position */
    double angle;     /* Orientation */
    double scale;     /* Size */
    int type;         /* Tile type */
    int depth;        /* Subdivision depth */
} SubstitutionTile;

/* Generate vertices for a Penrose rhomb */
static void penrose_rhomb_vertices(PenroseTileType type, double x, double y, 
                                  double angle, double scale,
                                  SylvesVector3 vertices[4]) {
    double half_angle, side_length;
    
    if (type == PENROSE_THIN) {
        half_angle = M_PI / 5.0;  /* 36 degrees */
        side_length = scale;
    } else {
        half_angle = 2.0 * M_PI / 5.0;  /* 72 degrees */
        side_length = scale;
    }
    
    /* Create rhomb centered at (x,y) with given orientation */
    double cos_a = cos(angle);
    double sin_a = sin(angle);
    double cos_h = cos(half_angle);
    double sin_h = sin(half_angle);
    
    /* Four vertices of the rhomb */
    vertices[0].x = x + side_length * cos_a;
    vertices[0].y = y + side_length * sin_a;
    vertices[0].z = 0;
    
    vertices[1].x = x + side_length * cos_h * cos(angle + M_PI/2) - side_length * sin_h * sin(angle + M_PI/2);
    vertices[1].y = y + side_length * cos_h * sin(angle + M_PI/2) + side_length * sin_h * cos(angle + M_PI/2);
    vertices[1].z = 0;
    
    vertices[2].x = x - side_length * cos_a;
    vertices[2].y = y - side_length * sin_a;
    vertices[2].z = 0;
    
    vertices[3].x = x - side_length * cos_h * cos(angle + M_PI/2) + side_length * sin_h * sin(angle + M_PI/2);
    vertices[3].y = y - side_length * cos_h * sin(angle + M_PI/2) - side_length * sin_h * cos(angle + M_PI/2);
    vertices[3].z = 0;
}

/* Subdivide a Penrose rhomb into smaller rhombs */
static void penrose_subdivide(SubstitutionTile* tile, SubstitutionTile* children, int* num_children) {
    *num_children = 0;
    double new_scale = tile->scale / PHI;
    
    if (tile->type == PENROSE_THICK) {
        /* Thick rhomb subdivides into 1 thick + 2 thin */
        
        /* Central thick rhomb */
        children[0].x = tile->x;
        children[0].y = tile->y;
        children[0].angle = tile->angle;
        children[0].scale = new_scale;
        children[0].type = PENROSE_THICK;
        children[0].depth = tile->depth - 1;
        (*num_children)++;
        
        /* Two thin rhombs */
        double offset = new_scale * (1.0 + 1.0/PHI) * 0.5;
        
        children[1].x = tile->x + offset * cos(tile->angle + M_PI/5);
        children[1].y = tile->y + offset * sin(tile->angle + M_PI/5);
        children[1].angle = tile->angle + 3*M_PI/5;
        children[1].scale = new_scale;
        children[1].type = PENROSE_THIN;
        children[1].depth = tile->depth - 1;
        (*num_children)++;
        
        children[2].x = tile->x + offset * cos(tile->angle - M_PI/5);
        children[2].y = tile->y + offset * sin(tile->angle - M_PI/5);
        children[2].angle = tile->angle - 3*M_PI/5;
        children[2].scale = new_scale;
        children[2].type = PENROSE_THIN;
        children[2].depth = tile->depth - 1;
        (*num_children)++;
        
    } else {
        /* Thin rhomb subdivides into 2 thin */
        double offset = new_scale * 0.5 * (1.0 + 1.0/PHI);
        
        children[0].x = tile->x + offset * cos(tile->angle);
        children[0].y = tile->y + offset * sin(tile->angle);
        children[0].angle = tile->angle + M_PI;
        children[0].scale = new_scale;
        children[0].type = PENROSE_THIN;
        children[0].depth = tile->depth - 1;
        (*num_children)++;
        
        children[1].x = tile->x - offset * cos(tile->angle);
        children[1].y = tile->y - offset * sin(tile->angle);
        children[1].angle = tile->angle;
        children[1].scale = new_scale;
        children[1].type = PENROSE_THIN;
        children[1].depth = tile->depth - 1;
        (*num_children)++;
    }
}

/* Recursively generate Penrose tiling */
static void generate_penrose_tiles(SubstitutionTile* tile, SylvesMeshData* mesh_data) {
    if (tile->depth <= 0) {
        /* Add tile to mesh */
        SylvesVector3 vertices[4];
        penrose_rhomb_vertices(tile->type, tile->x, tile->y, tile->angle, tile->scale, vertices);
        
        int indices[4] = {0, 1, 2, 3};
        sylves_mesh_data_add_ngon_face(mesh_data, vertices, indices, 4);
    } else {
        /* Subdivide and recurse */
        SubstitutionTile children[3];
        int num_children;
        penrose_subdivide(tile, children, &num_children);
        
        for (int i = 0; i < num_children; i++) {
            generate_penrose_tiles(&children[i], mesh_data);
        }
    }
}

/* Create initial configuration for Penrose tiling */
static SylvesMeshData* create_penrose_mesh(int subdivision_depth, double scale) {
    /* Estimate number of tiles needed */
    int est_tiles = 5 * (1 << (subdivision_depth * 2)); /* Rough estimate */
    SylvesMeshData* mesh_data = sylves_mesh_data_create(est_tiles * 4, est_tiles);
    if (!mesh_data) return NULL;
    
    /* Start with a ring of 5 thick rhombs */
    for (int i = 0; i < 5; i++) {
        SubstitutionTile tile;
        double angle = i * 2.0 * M_PI / 5.0;
        tile.x = 0;
        tile.y = 0;
        tile.angle = angle;
        tile.scale = scale;
        tile.type = PENROSE_THICK;
        tile.depth = subdivision_depth;
        
        generate_penrose_tiles(&tile, mesh_data);
    }
    
    return mesh_data;
}

/* Generate vertices for Ammann-Beenker square */
static void ammann_square_vertices(double x, double y, double angle, double scale,
                                   SylvesVector3 vertices[4]) {
    double half = scale * 0.5;
    double cos_a = cos(angle);
    double sin_a = sin(angle);
    
    /* Square vertices */
    vertices[0].x = x + half * (cos_a - sin_a);
    vertices[0].y = y + half * (sin_a + cos_a);
    vertices[0].z = 0;
    
    vertices[1].x = x + half * (-cos_a - sin_a);
    vertices[1].y = y + half * (-sin_a + cos_a);
    vertices[1].z = 0;
    
    vertices[2].x = x + half * (-cos_a + sin_a);
    vertices[2].y = y + half * (-sin_a - cos_a);
    vertices[2].z = 0;
    
    vertices[3].x = x + half * (cos_a + sin_a);
    vertices[3].y = y + half * (sin_a - cos_a);
    vertices[3].z = 0;
}

/* Generate vertices for Ammann-Beenker rhomb */
static void ammann_rhomb_vertices(double x, double y, double angle, double scale,
                                 SylvesVector3 vertices[4]) {
    double cos_a = cos(angle);
    double sin_a = sin(angle);
    double sqrt2 = sqrt(2.0);
    
    /* 45-degree rhomb vertices */
    vertices[0].x = x + scale * cos_a;
    vertices[0].y = y + scale * sin_a;
    vertices[0].z = 0;
    
    vertices[1].x = x + scale/sqrt2 * cos(angle + M_PI/4);
    vertices[1].y = y + scale/sqrt2 * sin(angle + M_PI/4);
    vertices[1].z = 0;
    
    vertices[2].x = x + scale * cos(angle + M_PI/2);
    vertices[2].y = y + scale * sin(angle + M_PI/2);
    vertices[2].z = 0;
    
    vertices[3].x = x + scale/sqrt2 * cos(angle - M_PI/4);
    vertices[3].y = y + scale/sqrt2 * sin(angle - M_PI/4);
    vertices[3].z = 0;
}

/* Create Ammann-Beenker tiling */
static SylvesMeshData* create_ammann_beenker_mesh(int subdivision_depth, double scale) {
    /* Estimate number of tiles needed */
    int size = 1 << subdivision_depth;
    int est_tiles = (2 * size + 1) * (2 * size + 1) * 2;
    SylvesMeshData* mesh_data = sylves_mesh_data_create(est_tiles * 4, est_tiles);
    if (!mesh_data) return NULL;
    
    /* Simple implementation: create a pattern of squares and rhombs */
    /* This is a simplified version - true Ammann-Beenker would use substitution rules */
    
    double spacing = scale;
    
    for (int i = -size; i <= size; i++) {
        for (int j = -size; j <= size; j++) {
            double x = i * spacing;
            double y = j * spacing;
            
            /* Add square */
            SylvesVector3 square_verts[4];
            ammann_square_vertices(x, y, 0, scale * 0.7, square_verts);
            int indices[4] = {0, 1, 2, 3};
            sylves_mesh_data_add_ngon_face(mesh_data, square_verts, indices, 4);
            
            /* Add rhombs in gaps */
            if (i < size && j < size) {
                SylvesVector3 rhomb_verts[4];
                ammann_rhomb_vertices(x + spacing/2, y + spacing/2, M_PI/4, scale * 0.5, rhomb_verts);
                sylves_mesh_data_add_ngon_face(mesh_data, rhomb_verts, indices, 4);
            }
        }
    }
    
    return mesh_data;
}

SylvesGrid* sylves_penrose_rhomb_grid_create(int subdivision_depth, double scale) {
    if (subdivision_depth < 0 || subdivision_depth > 10) {
        return NULL;
    }
    
    SylvesMeshData* mesh_data = create_penrose_mesh(subdivision_depth, scale);
    if (!mesh_data) {
        return NULL;
    }
    
    SylvesGrid* grid = sylves_mesh_grid_create(mesh_data);
    sylves_mesh_data_destroy(mesh_data);
    
    return grid;
}

SylvesGrid* sylves_ammann_beenker_grid_create(int subdivision_depth, double scale) {
    if (subdivision_depth < 0 || subdivision_depth > 10) {
        return NULL;
    }
    
    SylvesMeshData* mesh_data = create_ammann_beenker_mesh(subdivision_depth, scale);
    if (!mesh_data) {
        return NULL;
    }
    
    SylvesGrid* grid = sylves_mesh_grid_create(mesh_data);
    sylves_mesh_data_destroy(mesh_data);
    
    return grid;
}

SylvesGrid* sylves_substitution_tiling_grid_create(SylvesSubstitutionType type,
                                                  int subdivision_depth,
                                                  double scale) {
    switch (type) {
        case SYLVES_SUBSTITUTION_PENROSE_RHOMB:
            return sylves_penrose_rhomb_grid_create(subdivision_depth, scale);
            
        case SYLVES_SUBSTITUTION_AMMANN_BEENKER:
            return sylves_ammann_beenker_grid_create(subdivision_depth, scale);
            
        case SYLVES_SUBSTITUTION_PINWHEEL:
        case SYLVES_SUBSTITUTION_CHAIR:
            /* Not implemented yet */
            return NULL;
            
        default:
            return NULL;
    }
}
