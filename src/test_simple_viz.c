#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sylves/sylves.h"
#include "sylves/square_grid.h"
#include "sylves/hex_grid.h"
#include "sylves/cell.h"

// Simple function to render a bounded grid to a PPM image
void render_grid_to_ppm(const char* filename, SylvesGrid* grid, int width, int height) {
    uint8_t* pixels = (uint8_t*)calloc(width * height * 3, sizeof(uint8_t));
    
    // Fill with white background
    memset(pixels, 240, width * height * 3);
    
    // Scale factor to map grid coordinates to image coordinates
    float scale = 20.0f;  // 20 pixels per grid unit
    float offset_x = width / 2.0f;
    float offset_y = height / 2.0f;
    
    // Draw cells in a small region around the origin
    for (int cy = -5; cy <= 5; cy++) {
        for (int cx = -5; cx <= 5; cx++) {
            SylvesCell cell = sylves_cell_create_2d(cx, cy);
            
            // Check if cell is in grid
            bool in_grid = false;
            if (grid->vtable && grid->vtable->is_cell_in_grid) {
                in_grid = grid->vtable->is_cell_in_grid(grid, cell);
            } else {
                in_grid = true; // Assume it's in grid if no check available
            }
            
            if (in_grid) {
                // Get cell center
                SylvesVector3 center = {0, 0, 0};
                if (grid->vtable && grid->vtable->get_cell_center) {
                    center = grid->vtable->get_cell_center(grid, cell);
                } else {
                    // Fallback: use cell coordinates directly
                    center.x = (float)cx;
                    center.y = (float)cy;
                }
                
                // Convert to image coordinates
                int px = (int)(center.x * scale + offset_x);
                int py = (int)(height - (center.y * scale + offset_y)); // Flip Y axis
                
                // Draw a small square for the cell center
                int size = 3;
                for (int dy = -size; dy <= size; dy++) {
                    for (int dx = -size; dx <= size; dx++) {
                        int x = px + dx;
                        int y = py + dy;
                        if (x >= 0 && x < width && y >= 0 && y < height) {
                            int idx = (y * width + x) * 3;
                            // Color based on cell coordinates
                            pixels[idx + 0] = (uint8_t)((cx + 5) * 25);     // R
                            pixels[idx + 1] = (uint8_t)((cy + 5) * 25);     // G
                            pixels[idx + 2] = 100;                          // B
                        }
                    }
                }
                
                // Try to draw cell edges using polygon
                if (grid->vtable && grid->vtable->get_polygon) {
                    SylvesVector3 vertices[8];
                    int vertex_count = grid->vtable->get_polygon(grid, cell, vertices, 8);
                    
                    if (vertex_count > 0 && vertex_count <= 8) {
                        // Draw lines between consecutive vertices
                        for (int i = 0; i < vertex_count; i++) {
                            int j = (i + 1) % vertex_count;
                            
                            int x1 = (int)(vertices[i].x * scale + offset_x);
                            int y1 = (int)(height - (vertices[i].y * scale + offset_y));
                            int x2 = (int)(vertices[j].x * scale + offset_x);
                            int y2 = (int)(height - (vertices[j].y * scale + offset_y));
                            
                            // Simple line drawing (Bresenham would be better)
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
    } else {
        printf("Failed to open %s for writing\n", filename);
    }
    
    free(pixels);
}

int main() {
    printf("Creating simple grid visualizations...\n");
    
    // Create a bounded square grid
    SylvesGrid* square_grid = sylves_square_grid_create_bounded(1.0, -5, -5, 5, 5);
    if (square_grid) {
        printf("Created bounded square grid\n");
        render_grid_to_ppm("square_grid.ppm", square_grid, 400, 400);
    }
    
    // Create a bounded hex grid
    SylvesGrid* hex_grid = sylves_hex_grid_create_bounded(SYLVES_HEX_ORIENTATION_FLAT_TOP, 1.0, -5, -5, 5, 5);
    if (hex_grid) {
        printf("Created bounded hex grid\n");
        render_grid_to_ppm("hex_grid.ppm", hex_grid, 400, 400);
    }
    
    // For comparison, also render unbounded grids (showing just a region)
    SylvesGrid* unbounded_square = sylves_square_grid_create(1.0);
    if (unbounded_square) {
        printf("Created unbounded square grid\n");
        render_grid_to_ppm("square_grid_unbounded.ppm", unbounded_square, 400, 400);
    }
    
    printf("\nDone! Created PPM images:\n");
    printf("  - square_grid.ppm (bounded)\n");
    printf("  - hex_grid.ppm (bounded)\n");
    printf("  - square_grid_unbounded.ppm (unbounded)\n");
    printf("\nConvert to PNG with: convert *.ppm *.png\n");
    
    return 0;
}
