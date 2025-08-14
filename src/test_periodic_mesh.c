#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sylves/sylves.h"
#include "sylves/periodic_planar_mesh_grid.h"
#include "sylves/mesh.h"
#include "sylves/cell.h"

// Simple function to render a grid to a PPM image
void render_grid_to_ppm(const char* filename, SylvesGrid* grid, int width, int height, float scale) {
    uint8_t* pixels = (uint8_t*)calloc(width * height * 3, sizeof(uint8_t));
    
    // Fill with white background
    memset(pixels, 240, width * height * 3);
    
    float offset_x = width / 2.0f;
    float offset_y = height / 2.0f;
    
    // Draw cells in a region around the origin
    int range = (int)(width / scale / 2) + 2;
    for (int cy = -range; cy <= range; cy++) {
        for (int cx = -range; cx <= range; cx++) {
            SylvesCell cell = sylves_cell_create_2d(cx, cy);
            
            // Check if cell is in grid
            bool in_grid = true;
            if (grid->vtable && grid->vtable->is_cell_in_grid) {
                in_grid = grid->vtable->is_cell_in_grid(grid, cell);
            }
            
            if (in_grid) {
                // Get cell center
                SylvesVector3 center = {0, 0, 0};
                if (grid->vtable && grid->vtable->get_cell_center) {
                    center = grid->vtable->get_cell_center(grid, cell);
                } else {
                    center.x = (float)cx;
                    center.y = (float)cy;
                }
                
                // Try to draw cell edges using polygon
                if (grid->vtable && grid->vtable->get_polygon) {
                    SylvesVector3 vertices[12];  // Support up to 12 vertices
                    int vertex_count = grid->vtable->get_polygon(grid, cell, vertices, 12);
                    
                    if (vertex_count > 0 && vertex_count <= 12) {
                        // Fill the polygon (simple scanline algorithm)
                        float min_y = vertices[0].y, max_y = vertices[0].y;
                        for (int i = 1; i < vertex_count; i++) {
                            if (vertices[i].y < min_y) min_y = vertices[i].y;
                            if (vertices[i].y > max_y) max_y = vertices[i].y;
                        }
                        
                        int y_start = (int)((height - (max_y * scale + offset_y)));
                        int y_end = (int)((height - (min_y * scale + offset_y)));
                        
                        for (int py = y_start; py <= y_end && py >= 0 && py < height; py++) {
                            float scan_y = (height - py - offset_y) / scale;
                            
                            // Find intersections with scanline
                            float intersections[12];
                            int num_intersections = 0;
                            
                            for (int i = 0; i < vertex_count; i++) {
                                int j = (i + 1) % vertex_count;
                                float y1 = vertices[i].y;
                                float y2 = vertices[j].y;
                                
                                if ((y1 <= scan_y && y2 > scan_y) || (y2 <= scan_y && y1 > scan_y)) {
                                    float x1 = vertices[i].x;
                                    float x2 = vertices[j].x;
                                    float t = (scan_y - y1) / (y2 - y1);
                                    float x = x1 + t * (x2 - x1);
                                    intersections[num_intersections++] = x;
                                }
                            }
                            
                            // Sort intersections
                            for (int i = 0; i < num_intersections - 1; i++) {
                                for (int j = i + 1; j < num_intersections; j++) {
                                    if (intersections[i] > intersections[j]) {
                                        float temp = intersections[i];
                                        intersections[i] = intersections[j];
                                        intersections[j] = temp;
                                    }
                                }
                            }
                            
                            // Fill between pairs of intersections
                            for (int i = 0; i < num_intersections - 1; i += 2) {
                                int x_start = (int)(intersections[i] * scale + offset_x);
                                int x_end = (int)(intersections[i + 1] * scale + offset_x);
                                
                                for (int px = x_start; px <= x_end && px >= 0 && px < width; px++) {
                                    int idx = (py * width + px) * 3;
                                    // Color based on cell type/position
                                    int color_hash = (cx * 7 + cy * 13) & 0x7;
                                    pixels[idx + 0] = (uint8_t)(100 + color_hash * 20);
                                    pixels[idx + 1] = (uint8_t)(120 + ((color_hash >> 1) & 3) * 40);
                                    pixels[idx + 2] = (uint8_t)(140 + ((color_hash >> 2) & 1) * 60);
                                }
                            }
                        }
                        
                        // Draw edges
                        for (int i = 0; i < vertex_count; i++) {
                            int j = (i + 1) % vertex_count;
                            
                            int x1 = (int)(vertices[i].x * scale + offset_x);
                            int y1 = (int)(height - (vertices[i].y * scale + offset_y));
                            int x2 = (int)(vertices[j].x * scale + offset_x);
                            int y2 = (int)(height - (vertices[j].y * scale + offset_y));
                            
                            int steps = (int)fmaxf(abs(x2 - x1), abs(y2 - y1));
                            if (steps > 0) {
                                for (int s = 0; s <= steps; s++) {
                                    float t = (float)s / steps;
                                    int x = (int)(x1 + t * (x2 - x1));
                                    int y = (int)(y1 + t * (y2 - y1));
                                    
                                    if (x >= 0 && x < width && y >= 0 && y < height) {
                                        int idx = (y * width + x) * 3;
                                        // Black edge
                                        pixels[idx + 0] = 0;
                                        pixels[idx + 1] = 0;
                                        pixels[idx + 2] = 0;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    // Write PPM file
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
    printf("Testing periodic planar mesh grids...\n\n");
    
    // Test Cairo pentagonal tiling
    printf("1. Creating Cairo pentagonal tiling...\n");
    SylvesGrid* cairo_grid = sylves_cairo_grid_create(4.0, 4.0);
    if (cairo_grid) {
        printf("   Created Cairo grid\n");
        render_grid_to_ppm("cairo_tiling.ppm", cairo_grid, 600, 600, 30.0f);
    } else {
        printf("   Failed to create Cairo grid\n");
    }
    
    // Test Rhombille tiling
    printf("2. Creating Rhombille tiling...\n");
    SylvesGrid* rhombille_grid = sylves_rhombille_grid_create(4.0, 4.0);
    if (rhombille_grid) {
        printf("   Created Rhombille grid\n");
        render_grid_to_ppm("rhombille_tiling.ppm", rhombille_grid, 600, 600, 30.0f);
    } else {
        printf("   Failed to create Rhombille grid\n");
    }
    
    // Test Trihexagonal tiling
    printf("3. Creating Trihexagonal tiling...\n");
    SylvesGrid* trihex_grid = sylves_trihex_grid_create(4.0, 4.0);
    if (trihex_grid) {
        printf("   Created Trihexagonal grid\n");
        render_grid_to_ppm("trihex_tiling.ppm", trihex_grid, 600, 600, 30.0f);
    } else {
        printf("   Failed to create Trihexagonal grid\n");
    }
    
    // Test generic periodic planar mesh grid creation
    printf("4. Creating generic periodic mesh (Tetrakis Square)...\n");
    SylvesGrid* tetrakis_grid = sylves_periodic_planar_mesh_grid_create(
        SYLVES_PERIODIC_TETRAKIS_SQUARE, 4.0, 4.0);
    if (tetrakis_grid) {
        printf("   Created Tetrakis Square grid\n");
        render_grid_to_ppm("tetrakis_square.ppm", tetrakis_grid, 600, 600, 30.0f);
    } else {
        printf("   Failed to create Tetrakis Square grid\n");
    }
    
    printf("\nDone! Check the generated .ppm files\n");
    printf("Convert to PNG with: sips -s format png *.ppm --out .\n");
    
    return 0;
}
