#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sylves/sylves.h"
#include "sylves/raster_export.h"
#include "sylves/square_grid.h"
#include "sylves/hex_grid.h"
#include "sylves/triangle_grid.h"

// Simple callback to color cells based on their coordinates
void color_cells(SylvesCell cell, SylvesCellStyle* style, void* user_data) {
    // Create a gradient based on cell coordinates
    int index = cell.x + cell.y * 10;  // Simple hash from coordinates
    
    // Use HSV to RGB conversion for nice colors
    float h = (index * 30) % 360;  // Hue varies by cell
    float s = 0.7f;  // Saturation
    float v = 0.9f;  // Value
    
    // Convert HSV to RGB
    float c = v * s;
    float x = c * (1 - fabsf(fmodf(h / 60.0f, 2) - 1));
    float m = v - c;
    
    float r, g, b;
    if (h < 60) {
        r = c; g = x; b = 0;
    } else if (h < 120) {
        r = x; g = c; b = 0;
    } else if (h < 180) {
        r = 0; g = c; b = x;
    } else if (h < 240) {
        r = 0; g = x; b = c;
    } else if (h < 300) {
        r = x; g = 0; b = c;
    } else {
        r = c; g = 0; b = x;
    }
    
    // Convert to 0-255 range
    style->fill_color.r = (uint8_t)((r + m) * 255);
    style->fill_color.g = (uint8_t)((g + m) * 255);
    style->fill_color.b = (uint8_t)((b + m) * 255);
    style->fill_color.a = 200;  // Slightly transparent
    
    // Black outline
    style->stroke_color = sylves_color_rgb(0, 0, 0);
    style->stroke_width = 2.0f;
    style->filled = 1;
    style->stroked = 1;
}

int main(int argc, char* argv[]) {
    SylvesError error;
    
    printf("Creating grid visualization...\n");
    
    // Create different types of grids to test
    SylvesGrid* grid = NULL;
    const char* grid_type = "hex";  // Can be "square", "hex", or "triangle"
    
    if (strcmp(grid_type, "square") == 0) {
        // Create a square grid
        grid = sylves_square_grid_create(1.0);  // cell size of 1.0
        if (grid) {
            printf("Created square grid\n");
        }
    } else if (strcmp(grid_type, "hex") == 0) {
        // Create a hexagonal grid
        grid = sylves_hex_grid_create(SYLVES_HEX_ORIENTATION_FLAT_TOP, 1.0);
        if (grid) {
            printf("Created hexagonal grid\n");
        }
    } else if (strcmp(grid_type, "triangle") == 0) {
        // Create a triangle grid
        grid = sylves_triangle_grid_create(1.0, SYLVES_TRIANGLE_ORIENTATION_FLAT_TOPPED);
        if (grid) {
            printf("Created triangle grid\n");
        }
    }
    
    if (!grid) {
        printf("Failed to create grid\n");
        return 1;
    }
    
    // Setup raster export options
    SylvesRasterExportOptions options;
    sylves_raster_export_options_init(&options);
    
    // Image settings
    options.width = 800;
    options.height = 600;
    options.antialiasing = 1;
    options.samples_per_pixel = 4;
    
    // Viewport - adjust to fit the mesh
    options.viewport_min_x = -4.0f;
    options.viewport_min_y = -3.0f;
    options.viewport_max_x = 4.0f;
    options.viewport_max_y = 3.0f;
    
    // Background
    options.background_style = SYLVES_BACKGROUND_SOLID;
    options.background_color = sylves_color_rgb(240, 240, 240);
    
    // Default cell style
    sylves_cell_style_init_default(&options.default_style);
    options.default_style.fill_color = sylves_color_rgba(100, 150, 200, 200);
    options.default_style.stroke_color = sylves_color_rgb(0, 0, 0);
    options.default_style.stroke_width = 2.0f;
    
    // Use custom coloring callback
    options.get_cell_style = color_cells;
    options.style_user_data = NULL;
    
    // Show grid aids
    options.show_axes = 1;
    options.show_grid_lines = 0;
    options.axes_color = sylves_color_rgb(100, 100, 100);
    
    // Export to PNG
    const char* output_filename = "mesh_grid_visualization.png";
    error = sylves_export_grid_raster(
        grid,
        output_filename,
        SYLVES_IMAGE_FORMAT_PNG,
        &options
    );
    
    if (error != SYLVES_SUCCESS) {
        printf("Failed to export grid to PNG. Error: %d\n", error);
        
        // Try a simpler approach - just save raw pixel data
        printf("Trying alternative approach...\n");
        
        // Create a simple image manually
        int width = 400;
        int height = 300;
        uint8_t* pixels = (uint8_t*)calloc(width * height * 4, sizeof(uint8_t));
        
        // Fill with a gradient pattern as a test
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int idx = (y * width + x) * 4;
                pixels[idx + 0] = (uint8_t)(x * 255 / width);     // R
                pixels[idx + 1] = (uint8_t)(y * 255 / height);    // G
                pixels[idx + 2] = 128;                            // B
                pixels[idx + 3] = 255;                            // A
            }
        }
        
        // Try to write using stb_image_write directly
        printf("Writing test image...\n");
        FILE* f = fopen("test_gradient.png", "wb");
        if (f) {
            // Write a simple PPM file instead (easier format)
            fprintf(f, "P6\n%d %d\n255\n", width, height);
            for (int i = 0; i < width * height * 4; i += 4) {
                fputc(pixels[i], f);     // R
                fputc(pixels[i + 1], f); // G
                fputc(pixels[i + 2], f); // B
            }
            fclose(f);
            printf("Wrote test_gradient.ppm\n");
        }
        
        free(pixels);
    } else {
        printf("Successfully exported mesh grid visualization to %s\n", output_filename);
    }
    
    // Cleanup
    if (grid) {
        // Grid cleanup handled by specific grid type destructor
        // which should be called through the grid's vtable
    }
    
    printf("Done!\n");
    return 0;
}
