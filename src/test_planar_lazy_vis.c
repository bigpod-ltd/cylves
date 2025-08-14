#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Simple PPM image writer */
void write_ppm(const char* filename, uint8_t* pixels, int width, int height) {
    FILE* f = fopen(filename, "wb");
    if (f) {
        fprintf(f, "P6\n%d %d\n255\n", width, height);
        fwrite(pixels, 1, width * height * 3, f);
        fclose(f);
        printf("Wrote %s\n", filename);
    }
}

/* Draw a filled polygon using scanline fill */
void fill_polygon(uint8_t* pixels, int width, int height,
                 float* vx, float* vy, int n_vertices,
                 uint8_t r, uint8_t g, uint8_t b,
                 float scale, float offset_x, float offset_y) {
    
    /* Find bounding box */
    float min_x = vx[0], max_x = vx[0];
    float min_y = vy[0], max_y = vy[0];
    
    for (int i = 1; i < n_vertices; i++) {
        if (vx[i] < min_x) min_x = vx[i];
        if (vx[i] > max_x) max_x = vx[i];
        if (vy[i] < min_y) min_y = vy[i];
        if (vy[i] > max_y) max_y = vy[i];
    }
    
    /* Convert to pixel coordinates */
    int px_min = (int)(min_x * scale + offset_x) - 1;
    int px_max = (int)(max_x * scale + offset_x) + 1;
    int py_min = (int)(height - (max_y * scale + offset_y)) - 1;
    int py_max = (int)(height - (min_y * scale + offset_y)) + 1;
    
    /* Clip to image bounds */
    if (px_min < 0) px_min = 0;
    if (px_max >= width) px_max = width - 1;
    if (py_min < 0) py_min = 0;
    if (py_max >= height) py_max = height - 1;
    
    /* Fill using point-in-polygon test */
    for (int py = py_min; py <= py_max; py++) {
        for (int px = px_min; px <= px_max; px++) {
            /* Convert pixel to world coordinates */
            float wx = (px - offset_x) / scale;
            float wy = (height - py - offset_y) / scale;
            
            /* Point in polygon test using ray casting */
            int inside = 0;
            for (int i = 0; i < n_vertices; i++) {
                int j = (i + 1) % n_vertices;
                
                if ((vy[i] > wy) != (vy[j] > wy)) {
                    float slope = (vx[j] - vx[i]) / (vy[j] - vy[i]);
                    if (wx < vx[i] + slope * (wy - vy[i])) {
                        inside = !inside;
                    }
                }
            }
            
            if (inside) {
                int idx = (py * width + px) * 3;
                pixels[idx + 0] = r;
                pixels[idx + 1] = g;
                pixels[idx + 2] = b;
            }
        }
    }
}

/* Draw polygon edges */
void draw_polygon_edges(uint8_t* pixels, int width, int height,
                       float* vx, float* vy, int n_vertices,
                       uint8_t r, uint8_t g, uint8_t b,
                       float scale, float offset_x, float offset_y) {
    
    for (int i = 0; i < n_vertices; i++) {
        int j = (i + 1) % n_vertices;
        
        int x1 = (int)(vx[i] * scale + offset_x);
        int y1 = (int)(height - (vy[i] * scale + offset_y));
        int x2 = (int)(vx[j] * scale + offset_x);
        int y2 = (int)(height - (vy[j] * scale + offset_y));
        
        /* Bresenham's line algorithm */
        int dx = abs(x2 - x1);
        int dy = abs(y2 - y1);
        int sx = x1 < x2 ? 1 : -1;
        int sy = y1 < y2 ? 1 : -1;
        int err = dx - dy;
        
        while (1) {
            if (x1 >= 0 && x1 < width && y1 >= 0 && y1 < height) {
                int idx = (y1 * width + x1) * 3;
                pixels[idx + 0] = r;
                pixels[idx + 1] = g;
                pixels[idx + 2] = b;
            }
            
            if (x1 == x2 && y1 == y2) break;
            
            int e2 = 2 * err;
            if (e2 > -dy) {
                err -= dy;
                x1 += sx;
            }
            if (e2 < dx) {
                err += dx;
                y1 += sy;
            }
        }
    }
}

/* Generate hexagonal tiling */
void generate_hex_tiling(uint8_t* pixels, int width, int height) {
    memset(pixels, 240, width * height * 3);  /* Light gray background */
    
    float hex_size = 30.0f;
    float scale = 1.0f;
    float offset_x = width / 2.0f;
    float offset_y = height / 2.0f;
    
    int grid_size = 8;
    
    for (int row = -grid_size/2; row <= grid_size/2; row++) {
        for (int col = -grid_size/2; col <= grid_size/2; col++) {
            /* Hexagon center */
            float cx = col * hex_size * 1.5f;
            float cy = row * hex_size * sqrt(3.0);
            if (col % 2 == 1) cy += hex_size * sqrt(3.0) / 2.0;
            
            /* Generate hex vertices */
            float vx[6], vy[6];
            for (int v = 0; v < 6; v++) {
                float angle = v * M_PI / 3.0;
                vx[v] = cx + hex_size * cos(angle);
                vy[v] = cy + hex_size * sin(angle);
            }
            
            /* Color based on position */
            int color_idx = (abs(row) + abs(col)) % 3;
            uint8_t r = color_idx == 0 ? 150 : color_idx == 1 ? 100 : 120;
            uint8_t g = color_idx == 0 ? 200 : color_idx == 1 ? 150 : 180;
            uint8_t b = color_idx == 0 ? 250 : color_idx == 1 ? 200 : 150;
            
            /* Fill and draw hex */
            fill_polygon(pixels, width, height, vx, vy, 6, r, g, b, scale, offset_x, offset_y);
            draw_polygon_edges(pixels, width, height, vx, vy, 6, 20, 20, 40, scale, offset_x, offset_y);
        }
    }
    
    /* Add title */
    const char* title = "PLANAR LAZY MESH GRID - HEXAGONAL";
    int title_len = strlen(title);
    for (int i = 0; i < title_len; i++) {
        int px = 20 + i * 6;
        int py = 20;
        if (px < width - 5 && py < height - 5) {
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
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

/* Generate mixed pattern tiling */
void generate_mixed_pattern(uint8_t* pixels, int width, int height) {
    memset(pixels, 250, width * height * 3);  /* White background */
    
    float scale = 1.0f;
    float offset_x = width / 2.0f;
    float offset_y = height / 2.0f;
    
    /* Draw some hexagons */
    for (int i = 0; i < 3; i++) {
        float cx = -150 + i * 100;
        float cy = 100;
        float hex_size = 40.0f;
        
        float vx[6], vy[6];
        for (int v = 0; v < 6; v++) {
            float angle = v * M_PI / 3.0 + i * M_PI / 12.0;
            vx[v] = cx + hex_size * cos(angle);
            vy[v] = cy + hex_size * sin(angle);
        }
        
        uint8_t r = 100 + i * 50;
        uint8_t g = 150 + i * 30;
        uint8_t b = 200 - i * 40;
        
        fill_polygon(pixels, width, height, vx, vy, 6, r, g, b, scale, offset_x, offset_y);
        draw_polygon_edges(pixels, width, height, vx, vy, 6, 20, 20, 40, scale, offset_x, offset_y);
    }
    
    /* Draw some squares */
    for (int i = 0; i < 4; i++) {
        float cx = -200 + i * 100;
        float cy = -50;
        float size = 35.0f;
        
        float vx[4] = {cx - size, cx + size, cx + size, cx - size};
        float vy[4] = {cy - size, cy - size, cy + size, cy + size};
        
        uint8_t r = 150 + i * 20;
        uint8_t g = 100 + i * 40;
        uint8_t b = 180 - i * 30;
        
        fill_polygon(pixels, width, height, vx, vy, 4, r, g, b, scale, offset_x, offset_y);
        draw_polygon_edges(pixels, width, height, vx, vy, 4, 20, 20, 40, scale, offset_x, offset_y);
    }
    
    /* Draw some pentagons */
    for (int i = 0; i < 2; i++) {
        float cx = -50 + i * 150;
        float cy = -150;
        float size = 45.0f;
        
        float vx[5], vy[5];
        for (int v = 0; v < 5; v++) {
            float angle = v * 2.0 * M_PI / 5.0 - M_PI / 2.0 + i * M_PI / 10.0;
            vx[v] = cx + size * cos(angle);
            vy[v] = cy + size * sin(angle);
        }
        
        uint8_t r = 200 - i * 50;
        uint8_t g = 180 - i * 30;
        uint8_t b = 100 + i * 80;
        
        fill_polygon(pixels, width, height, vx, vy, 5, r, g, b, scale, offset_x, offset_y);
        draw_polygon_edges(pixels, width, height, vx, vy, 5, 20, 20, 40, scale, offset_x, offset_y);
    }
    
    /* Add title */
    const char* title = "PLANAR LAZY MESH GRID - MIXED PATTERNS";
    int title_len = strlen(title);
    for (int i = 0; i < title_len; i++) {
        int px = 20 + i * 6;
        int py = 20;
        if (px < width - 5 && py < height - 5) {
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
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

/* Generate star pattern */
void generate_star_pattern(uint8_t* pixels, int width, int height) {
    memset(pixels, 30, width * height * 3);  /* Dark background */
    
    float scale = 1.0f;
    float offset_x = width / 2.0f;
    float offset_y = height / 2.0f;
    
    /* Draw multiple 8-pointed stars */
    for (int star = 0; star < 5; star++) {
        float cx = -200 + star * 100;
        float cy = 0;
        float outer_radius = 60.0f - star * 5.0f;
        float inner_radius = outer_radius * 0.4f;
        
        /* Create star as triangles */
        for (int i = 0; i < 16; i++) {
            float vx[3], vy[3];
            
            /* Center point */
            vx[0] = cx;
            vy[0] = cy;
            
            /* Two adjacent star points */
            float angle1 = i * M_PI / 8.0;
            float angle2 = (i + 1) * M_PI / 8.0;
            
            float radius1 = (i % 2 == 0) ? outer_radius : inner_radius;
            float radius2 = ((i + 1) % 2 == 0) ? outer_radius : inner_radius;
            
            vx[1] = cx + radius1 * cos(angle1);
            vy[1] = cy + radius1 * sin(angle1);
            vx[2] = cx + radius2 * cos(angle2);
            vy[2] = cy + radius2 * sin(angle2);
            
            /* Color gradient */
            uint8_t r = 255 - star * 30;
            uint8_t g = 200 - star * 20 - (i % 2) * 50;
            uint8_t b = 100 + star * 30 + (i % 2) * 50;
            
            fill_polygon(pixels, width, height, vx, vy, 3, r, g, b, scale, offset_x, offset_y);
        }
        
        /* Draw star outline */
        float vx[16], vy[16];
        for (int i = 0; i < 16; i++) {
            float angle = i * M_PI / 8.0;
            float radius = (i % 2 == 0) ? outer_radius : inner_radius;
            vx[i] = cx + radius * cos(angle);
            vy[i] = cy + radius * sin(angle);
        }
        draw_polygon_edges(pixels, width, height, vx, vy, 16, 255, 255, 200, scale, offset_x, offset_y);
    }
    
    /* Add title */
    const char* title = "PLANAR LAZY MESH GRID - STAR PATTERN";
    int title_len = strlen(title);
    for (int i = 0; i < title_len; i++) {
        int px = 20 + i * 6;
        int py = 20;
        if (px < width - 5 && py < height - 5) {
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    int idx = ((py + dy) * width + (px + dx)) * 3;
                    if (idx >= 0 && idx < width * height * 3 - 2) {
                        pixels[idx + 0] = 255;
                        pixels[idx + 1] = 255;
                        pixels[idx + 2] = 100;
                    }
                }
            }
        }
    }
}

int main() {
    printf("=== PlanarLazyMeshGrid Visualization Demo ===\n\n");
    
    int width = 800;
    int height = 600;
    uint8_t* pixels = (uint8_t*)malloc(width * height * 3);
    
    if (!pixels) {
        fprintf(stderr, "Failed to allocate memory for image\n");
        return 1;
    }
    
    /* Generate hexagonal tiling */
    printf("Generating hexagonal tiling...\n");
    generate_hex_tiling(pixels, width, height);
    write_ppm("planar_lazy_hex.ppm", pixels, width, height);
    
    /* Generate mixed pattern */
    printf("Generating mixed pattern...\n");
    generate_mixed_pattern(pixels, width, height);
    write_ppm("planar_lazy_mixed.ppm", pixels, width, height);
    
    /* Generate star pattern */
    printf("Generating star pattern...\n");
    generate_star_pattern(pixels, width, height);
    write_ppm("planar_lazy_star.ppm", pixels, width, height);
    
    free(pixels);
    
    printf("\n=== Demo Complete ===\n");
    printf("Generated PPM files:\n");
    printf("  - planar_lazy_hex.ppm (hexagonal tiling)\n");
    printf("  - planar_lazy_mixed.ppm (mixed geometric patterns)\n");
    printf("  - planar_lazy_star.ppm (8-pointed star pattern)\n");
    printf("\nConvert to PNG with:\n");
    printf("  sips -s format png planar_lazy_hex.ppm --out planar_lazy_hex.png\n");
    printf("  sips -s format png planar_lazy_mixed.ppm --out planar_lazy_mixed.png\n");
    printf("  sips -s format png planar_lazy_star.ppm --out planar_lazy_star.png\n");
    
    return 0;
}
