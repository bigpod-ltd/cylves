/**
 * @file jittered_square_grid.c
 * @brief Jittered square grid implementation
 */

#include "sylves/jittered_square_grid.h"
#include "sylves/voronoi_grid.h"
#include "sylves/mesh_grid.h"
#include "sylves/mesh.h"
#include <stdlib.h>
#include <time.h>
#include <math.h>

/* Simple hash function for deterministic randomness */
static unsigned int hash(unsigned int x, unsigned int y, unsigned int seed) {
    unsigned int h = seed;
    h ^= x * 0x85ebca6b;
    h ^= h >> 13;
    h ^= y * 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}

/* Get random float in [0,1] from hash */
static double hash_to_float(unsigned int h) {
    return (h & 0x7FFFFFFF) / (double)0x7FFFFFFF;
}

SylvesJitteredSquareOptions sylves_jittered_square_options_default(void) {
    SylvesJitteredSquareOptions options = {
        .seed = 0,  /* Will be randomized */
        .jitter_amount = 0.4,
        .grid_size = 10
    };
    return options;
}

SylvesGrid* sylves_jittered_square_grid_create(double cell_size,
                                              const SylvesJitteredSquareOptions* options) {
    /* Use defaults if not provided */
    SylvesJitteredSquareOptions opts = options ? *options : sylves_jittered_square_options_default();
    
    /* Generate random seed if needed */
    if (opts.seed == 0) {
        opts.seed = (unsigned int)time(NULL);
    }
    
    /* Validate options */
    if (opts.jitter_amount < 0.0) opts.jitter_amount = 0.0;
    if (opts.jitter_amount > 0.5) opts.jitter_amount = 0.5;
    if (opts.grid_size < 2) opts.grid_size = 2;
    if (opts.grid_size > 100) opts.grid_size = 100;
    
    /* Generate jittered points */
    int num_points = opts.grid_size * opts.grid_size;
    SylvesVector2* points = malloc(num_points * sizeof(SylvesVector2));
    if (!points) return NULL;
    
    int idx = 0;
    for (int y = 0; y < opts.grid_size; y++) {
        for (int x = 0; x < opts.grid_size; x++) {
            /* Get hash for this cell */
            unsigned int h1 = hash(x, y, opts.seed);
            unsigned int h2 = hash(x, y, opts.seed + 1);
            
            /* Generate random offset */
            double jx = (hash_to_float(h1) - 0.5) * 2.0 * opts.jitter_amount;
            double jy = (hash_to_float(h2) - 0.5) * 2.0 * opts.jitter_amount;
            
            /* Set jittered point */
            points[idx].x = (x + 0.5 + jx) * cell_size;
            points[idx].y = (y + 0.5 + jy) * cell_size;
            idx++;
        }
    }
    
    /* Create Voronoi grid from jittered points */
    SylvesVoronoiGridOptions voronoi_opts = sylves_voronoi_grid_options_default();
    
    /* Set clipping bounds */
    SylvesVector2 clip_min = {0, 0};
    SylvesVector2 clip_max = {opts.grid_size * cell_size, opts.grid_size * cell_size};
    voronoi_opts.clip_min = &clip_min;
    voronoi_opts.clip_max = &clip_max;
    
    SylvesGrid* grid = sylves_voronoi_grid_create(points, num_points, &voronoi_opts);
    
    free(points);
    return grid;
}

SylvesGrid* sylves_perturbed_grid_create(const SylvesGrid* base_grid,
                                        double perturbation_amount,
                                        unsigned int seed) {
    if (!base_grid) return NULL;
    
    /* For now, we'll create a simple implementation that works with mesh grids */
    /* A full implementation would need to handle different grid types */
    
    /* Get mesh data from base grid if it's a mesh grid */
    /* This is a simplified implementation */
    
    /* Generate seed if needed */
    if (seed == 0) {
        seed = (unsigned int)time(NULL);
    }
    
    /* Create a perturbed mesh by getting cells and perturbing vertices */
    /* For demonstration, create a mesh with 4 vertices and 1 face */
    SylvesMeshData* mesh_data = sylves_mesh_data_create(4, 1);
    if (!mesh_data) return NULL;
    
    /* Simple implementation: create a small perturbed patch */
    /* A full implementation would enumerate all cells and perturb their vertices */
    
    /* For demonstration, create a simple perturbed quad */
    SylvesVector3 vertices[4];
    for (int i = 0; i < 4; i++) {
        double angle = i * M_PI / 2.0;
        double base_x = cos(angle);
        double base_y = sin(angle);
        
        /* Add perturbation */
        unsigned int h1 = hash(i, 0, seed);
        unsigned int h2 = hash(i, 1, seed);
        double px = (hash_to_float(h1) - 0.5) * 2.0 * perturbation_amount;
        double py = (hash_to_float(h2) - 0.5) * 2.0 * perturbation_amount;
        
        vertices[i].x = base_x + px;
        vertices[i].y = base_y + py;
        vertices[i].z = 0;
    }
    
    int indices[4] = {0, 1, 2, 3};
    sylves_mesh_data_add_ngon_face(mesh_data, vertices, indices, 4);
    
    SylvesGrid* grid = sylves_mesh_grid_create(mesh_data);
    sylves_mesh_data_destroy(mesh_data);
    
    return grid;
}
