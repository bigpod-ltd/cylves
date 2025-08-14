#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sylves/sylves.h"
#include "sylves/planar_lazy_mesh_grid.h"
#include "sylves/mesh.h"
#include "sylves/cell.h"

/* Example: Generate a procedural hexagonal tiling for each chunk */
SylvesMeshData* generate_hex_chunk(int chunk_x, int chunk_y, void* user_data) {
    printf("Generating chunk (%d, %d)...\n", chunk_x, chunk_y);
    
    /* Create a simple hexagonal pattern for this chunk */
    int hexes_per_chunk = 3;  /* 3x3 hexagons per chunk */
    int vertices_per_hex = 6;
    int num_hexes = hexes_per_chunk * hexes_per_chunk;
    
    /* Each hex has 6 vertices, but we'll share vertices between adjacent hexes */
    /* For simplicity, just create independent hexagons */
    SylvesMeshData* mesh = sylves_mesh_data_create(
        num_hexes * vertices_per_hex,  /* vertices */
        num_hexes                       /* faces */
    );
    
    if (!mesh) {
        printf("Failed to allocate mesh data\n");
        return NULL;
    }
    
    /* Generate hexagons */
    double hex_size = 1.0;
    double spacing = hex_size * sqrt(3.0);
    int vertex_idx = 0;
    
    for (int hy = 0; hy < hexes_per_chunk; hy++) {
        for (int hx = 0; hx < hexes_per_chunk; hx++) {
            /* Calculate hex center within chunk */
            double cx = hx * spacing;
            double cy = hy * spacing;
            
            /* Offset for hex arrangement */
            if (hy % 2 == 1) {
                cx += spacing / 2.0;
            }
            
            /* Generate hex vertices */
            int start_vertex = vertex_idx;
            for (int v = 0; v < 6; v++) {
                double angle = v * M_PI / 3.0;
                mesh->vertices[vertex_idx].x = cx + hex_size * cos(angle);
                mesh->vertices[vertex_idx].y = cy + hex_size * sin(angle);
                mesh->vertices[vertex_idx].z = 0;
                vertex_idx++;
            }
            
            /* Create face */
            int face_idx = hy * hexes_per_chunk + hx;
            mesh->faces[face_idx].vertex_count = 6;
            mesh->faces[face_idx].vertices = malloc(sizeof(int) * 6);
            mesh->faces[face_idx].neighbors = malloc(sizeof(int) * 6);
            
            if (!mesh->faces[face_idx].vertices || !mesh->faces[face_idx].neighbors) {
                printf("Failed to allocate face arrays\n");
                sylves_mesh_data_destroy(mesh);
                return NULL;
            }
            
            /* Set vertex indices */
            for (int v = 0; v < 6; v++) {
                mesh->faces[face_idx].vertices[v] = start_vertex + v;
                mesh->faces[face_idx].neighbors[v] = -1;  /* No neighbors for now */
            }
        }
    }
    
    /* Compute adjacency within chunk */
    sylves_mesh_compute_adjacency(mesh);
    
    return mesh;
}

/* Example: Generate a procedural Voronoi-like pattern */
SylvesMeshData* generate_voronoi_chunk(int chunk_x, int chunk_y, void* user_data) {
    printf("Generating Voronoi chunk (%d, %d)...\n", chunk_x, chunk_y);
    
    /* Use chunk coordinates as seed for randomization */
    unsigned int seed = (unsigned int)(chunk_x * 73856093 + chunk_y * 19349663);
    srand(seed);
    
    /* Create a simple random polygon mesh */
    int num_cells = 5 + (rand() % 3);  /* 5-7 cells per chunk */
    int max_vertices = num_cells * 8;
    
    SylvesMeshData* mesh = sylves_mesh_data_create(max_vertices, num_cells);
    if (!mesh) return NULL;
    
    int vertex_idx = 0;
    
    for (int cell = 0; cell < num_cells; cell++) {
        /* Random cell center within chunk */
        double cx = ((double)rand() / RAND_MAX) * 8.0 + 1.0;
        double cy = ((double)rand() / RAND_MAX) * 8.0 + 1.0;
        
        /* Random polygon with 4-8 vertices */
        int num_verts = 4 + (rand() % 5);
        
        int start_vertex = vertex_idx;
        for (int v = 0; v < num_verts; v++) {
            double angle = (2.0 * M_PI * v) / num_verts + ((double)rand() / RAND_MAX - 0.5) * 0.3;
            double radius = 0.8 + ((double)rand() / RAND_MAX) * 0.4;
            
            mesh->vertices[vertex_idx].x = cx + radius * cos(angle);
            mesh->vertices[vertex_idx].y = cy + radius * sin(angle);
            mesh->vertices[vertex_idx].z = 0;
            vertex_idx++;
        }
        
        /* Create face */
        mesh->faces[cell].vertex_count = num_verts;
        mesh->faces[cell].vertices = malloc(sizeof(int) * num_verts);
        mesh->faces[cell].neighbors = malloc(sizeof(int) * num_verts);
        
        if (!mesh->faces[cell].vertices || !mesh->faces[cell].neighbors) {
            sylves_mesh_data_destroy(mesh);
            return NULL;
        }
        
        for (int v = 0; v < num_verts; v++) {
            mesh->faces[cell].vertices[v] = start_vertex + v;
            mesh->faces[cell].neighbors[v] = -1;
        }
    }
    
    /* Update actual vertex count */
    mesh->vertex_count = vertex_idx;
    
    return mesh;
}

/* Render a grid to PPM for visualization */
void render_lazy_grid_to_ppm(const char* filename, SylvesGrid* grid, 
                             int width, int height, float scale) {
    uint8_t* pixels = (uint8_t*)calloc(width * height * 3, sizeof(uint8_t));
    
    /* Fill with white background */
    memset(pixels, 240, width * height * 3);
    
    float offset_x = width / 2.0f;
    float offset_y = height / 2.0f;
    
    /* Draw cells in visible region */
    int range = (int)(width / scale / 2) + 2;
    int cells_drawn = 0;
    
    for (int cy = -range; cy <= range; cy++) {
        for (int cx = -range; cx <= range; cx++) {
            SylvesCell cell = sylves_cell_create_2d(cx, cy);
            
            /* Check if cell exists in grid */
            if (grid->vtable && grid->vtable->is_cell_in_grid &&
                !grid->vtable->is_cell_in_grid(grid, cell)) {
                continue;
            }
            
            /* Get polygon vertices */
            if (grid->vtable && grid->vtable->get_polygon) {
                SylvesVector3 vertices[12];
                int vertex_count = grid->vtable->get_polygon(grid, cell, vertices, 12);
                
                if (vertex_count > 0 && vertex_count <= 12) {
                    cells_drawn++;
                    
                    /* Draw polygon edges */
                    for (int i = 0; i < vertex_count; i++) {
                        int j = (i + 1) % vertex_count;
                        
                        int x1 = (int)(vertices[i].x * scale + offset_x);
                        int y1 = (int)(height - (vertices[i].y * scale + offset_y));
                        int x2 = (int)(vertices[j].x * scale + offset_x);
                        int y2 = (int)(height - (vertices[j].y * scale + offset_y));
                        
                        /* Simple line drawing */
                        int steps = (int)fmaxf(abs(x2 - x1), abs(y2 - y1));
                        if (steps > 0) {
                            for (int s = 0; s <= steps; s++) {
                                float t = (float)s / steps;
                                int x = (int)(x1 + t * (x2 - x1));
                                int y = (int)(y1 + t * (y2 - y1));
                                
                                if (x >= 0 && x < width && y >= 0 && y < height) {
                                    int idx = (y * width + x) * 3;
                                    pixels[idx + 0] = 0;
                                    pixels[idx + 1] = 0;
                                    pixels[idx + 2] = 0;
                                }
                            }
                        }
                    }
                    
                    /* Draw cell center dot */
                    if (grid->vtable->get_cell_center) {
                        SylvesVector3 center = grid->vtable->get_cell_center(grid, cell);
                        int px = (int)(center.x * scale + offset_x);
                        int py = (int)(height - (center.y * scale + offset_y));
                        
                        /* Draw 3x3 dot */
                        for (int dy = -1; dy <= 1; dy++) {
                            for (int dx = -1; dx <= 1; dx++) {
                                int x = px + dx;
                                int y = py + dy;
                                if (x >= 0 && x < width && y >= 0 && y < height) {
                                    int idx = (y * width + x) * 3;
                                    /* Color based on chunk */
                                    int chunk_x = cx / 10;
                                    int chunk_y = cy / 10;
                                    pixels[idx + 0] = (uint8_t)(100 + (chunk_x * 50) % 156);
                                    pixels[idx + 1] = (uint8_t)(100 + (chunk_y * 50) % 156);
                                    pixels[idx + 2] = (uint8_t)(100 + ((chunk_x + chunk_y) * 50) % 156);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    printf("Drew %d cells\n", cells_drawn);
    
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
    printf("Testing PlanarLazyMeshGrid...\n\n");
    
    /* Test 1: Square chunk layout with hexagonal pattern */
    printf("1. Creating lazy grid with square chunks and hex pattern...\n");
    
    SylvesMeshGridOptions options;
    sylves_mesh_grid_options_init(&options);
    
    SylvesGrid* hex_lazy_grid = sylves_planar_lazy_mesh_grid_create_square(
        generate_hex_chunk,
        10.0,      /* chunk size */
        0.5,       /* margin */
        true,      /* translate mesh data */
        &options,
        NULL,      /* no bound */
        SYLVES_CACHE_ALWAYS,  /* cache all chunks */
        NULL       /* no user data */
    );
    
    if (hex_lazy_grid) {
        printf("   Created lazy hex grid\n");
        render_lazy_grid_to_ppm("lazy_hex_grid.ppm", hex_lazy_grid, 600, 600, 20.0f);
        
        /* Test cell access */
        SylvesCell test_cell = sylves_cell_create_2d(5, 5);
        if (hex_lazy_grid->vtable->is_cell_in_grid(hex_lazy_grid, test_cell)) {
            printf("   Cell (5,5) exists in grid\n");
            SylvesVector3 center = hex_lazy_grid->vtable->get_cell_center(hex_lazy_grid, test_cell);
            printf("   Cell center: (%.2f, %.2f, %.2f)\n", center.x, center.y, center.z);
        }
        
        /* Clean up */
        hex_lazy_grid->vtable->destroy(hex_lazy_grid);
    } else {
        printf("   Failed to create lazy hex grid\n");
    }
    
    /* Test 2: Procedural Voronoi-like pattern */
    printf("\n2. Creating lazy grid with Voronoi-like pattern...\n");
    
    SylvesGrid* voronoi_lazy_grid = sylves_planar_lazy_mesh_grid_create_square(
        generate_voronoi_chunk,
        10.0,      /* chunk size */
        0.0,       /* no margin */
        true,      /* translate mesh data */
        &options,
        NULL,      /* no bound */
        SYLVES_CACHE_LRU,  /* LRU cache */
        NULL       /* no user data */
    );
    
    if (voronoi_lazy_grid) {
        printf("   Created lazy Voronoi grid\n");
        render_lazy_grid_to_ppm("lazy_voronoi_grid.ppm", voronoi_lazy_grid, 600, 600, 20.0f);
        
        /* Clean up */
        voronoi_lazy_grid->vtable->destroy(voronoi_lazy_grid);
    } else {
        printf("   Failed to create lazy Voronoi grid\n");
    }
    
    printf("\nDone! Check the generated .ppm files\n");
    printf("Convert to PNG with: sips -s format png *.ppm --out .\n");
    
    return 0;
}
