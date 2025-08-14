#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include "src/include/sylves/sylves.h"
#include "src/include/sylves/planar_lazy_mesh_grid.h"
#include "src/include/sylves/mesh.h"
#include "src/include/sylves/cell.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Generate a tiled pattern with different shapes per chunk */
SylvesMeshData* generate_mixed_pattern(int chunk_x, int chunk_y, void* user_data) {
    printf("Generating mixed pattern chunk (%d, %d)...\n", chunk_x, chunk_y);
    
    /* Determine pattern type based on chunk coordinates */
    int pattern_type = (abs(chunk_x) + abs(chunk_y)) % 3;
    
    SylvesMeshData* mesh = NULL;
    
    switch (pattern_type) {
        case 0: {
            /* Hexagonal pattern */
            int cells_per_side = 2;
            int num_cells = cells_per_side * cells_per_side;
            mesh = sylves_mesh_data_create(num_cells * 6, num_cells);
            if (!mesh) return NULL;
            
            double hex_size = 1.5;
            double spacing = hex_size * sqrt(3.0);
            int vertex_idx = 0;
            int face_idx = 0;
            
            for (int y = 0; y < cells_per_side; y++) {
                for (int x = 0; x < cells_per_side; x++) {
                    double cx = x * spacing + 2.0;
                    double cy = y * spacing + 2.0;
                    if (y % 2 == 1) cx += spacing / 2.0;
                    
                    /* Add hex vertices */
                    for (int v = 0; v < 6; v++) {
                        double angle = v * M_PI / 3.0;
                        mesh->vertices[vertex_idx].x = cx + hex_size * cos(angle);
                        mesh->vertices[vertex_idx].y = cy + hex_size * sin(angle);
                        mesh->vertices[vertex_idx].z = 0;
                        vertex_idx++;
                    }
                    
                    /* Create face */
                    mesh->faces[face_idx].vertex_count = 6;
                    mesh->faces[face_idx].vertices = malloc(sizeof(int) * 6);
                    mesh->faces[face_idx].neighbors = malloc(sizeof(int) * 6);
                    
                    for (int v = 0; v < 6; v++) {
                        mesh->faces[face_idx].vertices[v] = (face_idx * 6) + v;
                        mesh->faces[face_idx].neighbors[v] = -1;
                    }
                    face_idx++;
                }
            }
            break;
        }
        
        case 1: {
            /* Square pattern */
            int cells_per_side = 3;
            int num_cells = cells_per_side * cells_per_side;
            mesh = sylves_mesh_data_create(num_cells * 4, num_cells);
            if (!mesh) return NULL;
            
            double square_size = 2.0;
            int vertex_idx = 0;
            int face_idx = 0;
            
            for (int y = 0; y < cells_per_side; y++) {
                for (int x = 0; x < cells_per_side; x++) {
                    double cx = x * square_size + 1.0;
                    double cy = y * square_size + 1.0;
                    
                    /* Add square vertices */
                    mesh->vertices[vertex_idx++] = (SylvesVector3){cx, cy, 0};
                    mesh->vertices[vertex_idx++] = (SylvesVector3){cx + square_size * 0.9, cy, 0};
                    mesh->vertices[vertex_idx++] = (SylvesVector3){cx + square_size * 0.9, cy + square_size * 0.9, 0};
                    mesh->vertices[vertex_idx++] = (SylvesVector3){cx, cy + square_size * 0.9, 0};
                    
                    /* Create face */
                    mesh->faces[face_idx].vertex_count = 4;
                    mesh->faces[face_idx].vertices = malloc(sizeof(int) * 4);
                    mesh->faces[face_idx].neighbors = malloc(sizeof(int) * 4);
                    
                    for (int v = 0; v < 4; v++) {
                        mesh->faces[face_idx].vertices[v] = (face_idx * 4) + v;
                        mesh->faces[face_idx].neighbors[v] = -1;
                    }
                    face_idx++;
                }
            }
            break;
        }
        
        case 2: {
            /* Pentagon pattern */
            int num_cells = 2;
            mesh = sylves_mesh_data_create(num_cells * 5, num_cells);
            if (!mesh) return NULL;
            
            int vertex_idx = 0;
            
            for (int i = 0; i < num_cells; i++) {
                double cx = 3.0 + i * 3.0;
                double cy = 3.0 + i * 1.5;
                double size = 2.0;
                
                /* Add pentagon vertices */
                for (int v = 0; v < 5; v++) {
                    double angle = v * 2.0 * M_PI / 5.0 - M_PI / 2.0;
                    mesh->vertices[vertex_idx].x = cx + size * cos(angle);
                    mesh->vertices[vertex_idx].y = cy + size * sin(angle);
                    mesh->vertices[vertex_idx].z = 0;
                    vertex_idx++;
                }
                
                /* Create face */
                mesh->faces[i].vertex_count = 5;
                mesh->faces[i].vertices = malloc(sizeof(int) * 5);
                mesh->faces[i].neighbors = malloc(sizeof(int) * 5);
                
                for (int v = 0; v < 5; v++) {
                    mesh->faces[i].vertices[v] = i * 5 + v;
                    mesh->faces[i].neighbors[v] = -1;
                }
            }
            break;
        }
    }
    
    if (mesh) {
        sylves_mesh_compute_adjacency(mesh);
    }
    
    return mesh;
}

/* Create a beautiful Islamic-inspired star pattern */
SylvesMeshData* generate_star_pattern(int chunk_x, int chunk_y, void* user_data) {
    /* Create an 8-pointed star pattern */
    SylvesMeshData* mesh = sylves_mesh_data_create(17, 9);
    if (!mesh) return NULL;
    
    /* Center of the pattern */
    double cx = 5.0;
    double cy = 5.0;
    
    /* Center vertex */
    mesh->vertices[0] = (SylvesVector3){cx, cy, 0};
    
    /* Inner and outer vertices for 8-pointed star */
    for (int i = 0; i < 8; i++) {
        double angle = i * M_PI / 4.0;
        double inner_radius = 2.0;
        double outer_radius = 4.0;
        
        /* Outer vertex (points of star) */
        mesh->vertices[1 + i * 2] = (SylvesVector3){
            cx + outer_radius * cos(angle),
            cy + outer_radius * sin(angle),
            0
        };
        
        /* Inner vertex (between points) */
        double inner_angle = angle + M_PI / 8.0;
        mesh->vertices[2 + i * 2] = (SylvesVector3){
            cx + inner_radius * cos(inner_angle),
            cy + inner_radius * sin(inner_angle),
            0
        };
    }
    
    /* Create 8 triangular faces from center to star points */
    for (int i = 0; i < 8; i++) {
        mesh->faces[i].vertex_count = 3;
        mesh->faces[i].vertices = malloc(sizeof(int) * 3);
        mesh->faces[i].neighbors = malloc(sizeof(int) * 3);
        
        mesh->faces[i].vertices[0] = 0;  /* Center */
        mesh->faces[i].vertices[1] = 1 + i * 2;  /* Current point */
        mesh->faces[i].vertices[2] = 2 + i * 2;  /* Next inner vertex */
        
        for (int j = 0; j < 3; j++) {
            mesh->faces[i].neighbors[j] = -1;
        }
    }
    
    /* Add one more face to complete the star */
    mesh->faces[8].vertex_count = 3;
    mesh->faces[8].vertices = malloc(sizeof(int) * 3);
    mesh->faces[8].neighbors = malloc(sizeof(int) * 3);
    
    mesh->faces[8].vertices[0] = 0;  /* Center */
    mesh->faces[8].vertices[1] = 16; /* Last inner vertex */
    mesh->faces[8].vertices[2] = 1;  /* First outer vertex */
    
    for (int j = 0; j < 3; j++) {
        mesh->faces[8].neighbors[j] = -1;
    }
    
    return mesh;
}

/* Render helper function */
void render_planar_lazy_grid(const char* filename, SylvesGrid* grid, 
                             int width, int height, float scale,
                             float view_x, float view_y) {
    uint8_t* pixels = (uint8_t*)calloc(width * height * 3, sizeof(uint8_t));
    
    /* Fill with white background */
    memset(pixels, 250, width * height * 3);
    
    float offset_x = width / 2.0f - view_x * scale;
    float offset_y = height / 2.0f + view_y * scale;
    
    int cells_drawn = 0;
    int cells_checked = 0;
    
    /* Check cells in the mesh grid (using face indices as x coordinate) */
    for (int face_idx = 0; face_idx < 100; face_idx++) {
        SylvesCell cell = sylves_cell_create_2d(face_idx, 0);
        cells_checked++;
        
        /* Get polygon vertices */
        SylvesVector3 vertices[12];
        int vertex_count = -1;
        
        if (grid->vtable && grid->vtable->get_polygon) {
            vertex_count = grid->vtable->get_polygon(grid, cell, vertices, 12);
        }
        
        if (vertex_count > 0 && vertex_count <= 12) {
            cells_drawn++;
            
            /* Check if polygon is in view */
            bool in_view = false;
            for (int i = 0; i < vertex_count; i++) {
                int px = (int)(vertices[i].x * scale + offset_x);
                int py = (int)(height - (vertices[i].y * scale + offset_y));
                if (px >= 0 && px < width && py >= 0 && py < height) {
                    in_view = true;
                    break;
                }
            }
            
            if (in_view) {
                /* Fill polygon with color based on chunk */
                int chunk_x = face_idx / 10;
                int chunk_y = 0;
                
                /* Calculate color based on position */
                int color_r = 100 + (chunk_x * 40) % 100;
                int color_g = 120 + (chunk_y * 40) % 100;
                int color_b = 180 + ((chunk_x + chunk_y) * 30) % 50;
                
                /* Simple polygon fill using bounding box */
                float min_x = vertices[0].x, max_x = vertices[0].x;
                float min_y = vertices[0].y, max_y = vertices[0].y;
                
                for (int i = 1; i < vertex_count; i++) {
                    if (vertices[i].x < min_x) min_x = vertices[i].x;
                    if (vertices[i].x > max_x) max_x = vertices[i].x;
                    if (vertices[i].y < min_y) min_y = vertices[i].y;
                    if (vertices[i].y > max_y) max_y = vertices[i].y;
                }
                
                int px_min = (int)(min_x * scale + offset_x) - 1;
                int px_max = (int)(max_x * scale + offset_x) + 1;
                int py_min = (int)(height - (max_y * scale + offset_y)) - 1;
                int py_max = (int)(height - (min_y * scale + offset_y)) + 1;
                
                /* Clip to image bounds */
                if (px_min < 0) px_min = 0;
                if (px_max >= width) px_max = width - 1;
                if (py_min < 0) py_min = 0;
                if (py_max >= height) py_max = height - 1;
                
                /* Simple point-in-polygon test for each pixel */
                for (int py = py_min; py <= py_max; py++) {
                    for (int px = px_min; px <= px_max; px++) {
                        /* Convert pixel to world coordinates */
                        float wx = (px - offset_x) / scale;
                        float wy = (height - py - offset_y) / scale;
                        
                        /* Point in polygon test using ray casting */
                        int inside = 0;
                        for (int i = 0; i < vertex_count; i++) {
                            int j = (i + 1) % vertex_count;
                            
                            if ((vertices[i].y > wy) != (vertices[j].y > wy)) {
                                float slope = (vertices[j].x - vertices[i].x) / 
                                            (vertices[j].y - vertices[i].y);
                                if (wx < vertices[i].x + slope * (wy - vertices[i].y)) {
                                    inside = !inside;
                                }
                            }
                        }
                        
                        if (inside) {
                            int idx = (py * width + px) * 3;
                            pixels[idx + 0] = color_r;
                            pixels[idx + 1] = color_g;
                            pixels[idx + 2] = color_b;
                        }
                    }
                }
                
                /* Draw edges */
                for (int i = 0; i < vertex_count; i++) {
                    int j = (i + 1) % vertex_count;
                    
                    int x1 = (int)(vertices[i].x * scale + offset_x);
                    int y1 = (int)(height - (vertices[i].y * scale + offset_y));
                    int x2 = (int)(vertices[j].x * scale + offset_x);
                    int y2 = (int)(height - (vertices[j].y * scale + offset_y));
                    
                    /* Draw line */
                    int steps = (int)fmaxf(abs(x2 - x1), abs(y2 - y1));
                    if (steps > 0) {
                        for (int s = 0; s <= steps; s++) {
                            float t = (float)s / steps;
                            int x = (int)(x1 + t * (x2 - x1));
                            int y = (int)(y1 + t * (y2 - y1));
                            
                            if (x >= 0 && x < width && y >= 0 && y < height) {
                                int idx = (y * width + x) * 3;
                                pixels[idx + 0] = 20;
                                pixels[idx + 1] = 20;
                                pixels[idx + 2] = 40;
                            }
                        }
                    }
                }
            }
        }
    }
    
    printf("Checked %d cells, drew %d cells\n", cells_checked, cells_drawn);
    
    /* Add a title text pattern in top-left */
    const char* title = "PLANAR LAZY MESH GRID";
    for (int i = 0; title[i]; i++) {
        int px = 10 + i * 8;
        int py = 15;
        if (px < width - 5 && py < height - 5) {
            /* Simple dot pattern for text */
            for (int dy = -2; dy <= 2; dy++) {
                for (int dx = -2; dx <= 2; dx++) {
                    if (abs(dx) + abs(dy) <= 2) {
                        int idx = ((py + dy) * width + (px + dx)) * 3;
                        if (idx >= 0 && idx < width * height * 3 - 2) {
                            pixels[idx + 0] = 255;
                            pixels[idx + 1] = 100;
                            pixels[idx + 2] = 50;
                        }
                    }
                }
            }
        }
    }
    
    /* Write PPM file */
    FILE* f = fopen(filename, "wb");
    if (f) {
        fprintf(f, "P6\n%d %d\n255\n", width, height);
        fwrite(pixels, 1, width * height * 3, f);
        fclose(f);
        printf("Wrote %s\n", filename);
    }
    
    free(pixels);
}

int main() {
    printf("=== PlanarLazyMeshGrid Demonstration ===\n\n");
    
    /* Create a lazy mesh grid with mixed patterns */
    printf("Creating PlanarLazyMeshGrid with mixed patterns...\n");
    
    SylvesMeshGridOptions options;
    sylves_mesh_grid_options_init(&options);
    
    SylvesGrid* mixed_grid = sylves_planar_lazy_mesh_grid_create_square(
        generate_mixed_pattern,
        10.0,      /* chunk size */
        0.0,       /* no margin */
        true,      /* translate mesh data */
        &options,
        NULL,      /* no bound */
        SYLVES_CACHE_ALWAYS,
        NULL       /* no user data */
    );
    
    if (mixed_grid) {
        printf("Successfully created mixed pattern lazy grid\n");
        
        /* Test some cell access to trigger chunk generation */
        for (int i = 0; i < 5; i++) {
            SylvesCell test_cell = sylves_cell_create_2d(i, 0);
            if (mixed_grid->vtable->get_cell_center) {
                SylvesVector3 center = mixed_grid->vtable->get_cell_center(mixed_grid, test_cell);
                printf("  Cell %d center: (%.2f, %.2f)\n", i, center.x, center.y);
            }
        }
        
        /* Render the grid */
        printf("\nRendering mixed pattern grid...\n");
        render_planar_lazy_grid("planar_lazy_mixed.ppm", mixed_grid, 
                                800, 600, 15.0f, 0.0f, 0.0f);
        
        /* Clean up */
        if (mixed_grid->vtable->destroy) {
            mixed_grid->vtable->destroy(mixed_grid);
        }
    }
    
    /* Create a star pattern grid */
    printf("\nCreating PlanarLazyMeshGrid with star pattern...\n");
    
    SylvesGrid* star_grid = sylves_planar_lazy_mesh_grid_create_square(
        generate_star_pattern,
        10.0,      /* chunk size */
        0.0,       /* no margin */
        true,      /* translate mesh data */
        &options,
        NULL,      /* no bound */
        SYLVES_CACHE_ALWAYS,
        NULL       /* no user data */
    );
    
    if (star_grid) {
        printf("Successfully created star pattern lazy grid\n");
        
        /* Render the grid */
        printf("Rendering star pattern grid...\n");
        render_planar_lazy_grid("planar_lazy_star.ppm", star_grid, 
                                800, 600, 25.0f, 5.0f, 5.0f);
        
        /* Clean up */
        if (star_grid->vtable->destroy) {
            star_grid->vtable->destroy(star_grid);
        }
    }
    
    printf("\n=== Demonstration Complete ===\n");
    printf("Generated output files:\n");
    printf("  - planar_lazy_mixed.ppm (mixed geometric patterns)\n");
    printf("  - planar_lazy_star.ppm (8-pointed star pattern)\n");
    printf("\nConvert to PNG with:\n");
    printf("  sips -s format png planar_lazy_mixed.ppm planar_lazy_star.ppm --out .\n");
    
    return 0;
}
