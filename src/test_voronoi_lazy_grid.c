#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct {
    float x, y;
} Point;

typedef struct {
    Point* points;
    int count;
} VoronoiCell;

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

/* Simple random number generator for consistent results */
uint32_t xorshift32(uint32_t* state) {
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

/* Generate random float between 0 and 1 */
float randf(uint32_t* seed) {
    return (float)(xorshift32(seed) & 0xFFFFFF) / (float)0xFFFFFF;
}

/* Distance squared between two points */
float dist_sq(float x1, float y1, float x2, float y2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    return dx * dx + dy * dy;
}

/* Generate Voronoi sites for a chunk */
void generate_chunk_sites(int chunk_x, int chunk_y, float chunk_size,
                         Point* sites, int* num_sites, int max_sites) {
    /* Use chunk coordinates as seed for reproducible patterns */
    uint32_t seed = ((chunk_x + 1000) * 73856093) ^ ((chunk_y + 1000) * 19349663);
    
    /* Number of sites varies by chunk for variety */
    int base_sites = 8 + (abs(chunk_x + chunk_y) % 5);
    *num_sites = base_sites;
    
    /* Generate random sites within chunk */
    float cx = chunk_x * chunk_size;
    float cy = chunk_y * chunk_size;
    
    for (int i = 0; i < *num_sites && i < max_sites; i++) {
        /* Add some sites with jitter for more organic look */
        sites[i].x = cx + randf(&seed) * chunk_size;
        sites[i].y = cy + randf(&seed) * chunk_size;
    }
    
    /* Add sites from neighboring chunks for seamless boundaries */
    /* This ensures cells cross chunk boundaries naturally */
    int extra_idx = *num_sites;
    
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;  /* Skip current chunk */
            
            int neighbor_x = chunk_x + dx;
            int neighbor_y = chunk_y + dy;
            uint32_t neighbor_seed = ((neighbor_x + 1000) * 73856093) ^ 
                                    ((neighbor_y + 1000) * 19349663);
            
            /* Add 2-3 sites from each neighbor near the boundary */
            int neighbor_sites = 2 + (abs(neighbor_x + neighbor_y) % 2);
            
            for (int i = 0; i < neighbor_sites && extra_idx < max_sites; i++) {
                float nx = neighbor_x * chunk_size + randf(&neighbor_seed) * chunk_size;
                float ny = neighbor_y * chunk_size + randf(&neighbor_seed) * chunk_size;
                
                /* Only include if close enough to affect current chunk */
                float dist_to_chunk = 0;
                if (nx < cx) dist_to_chunk = cx - nx;
                else if (nx > cx + chunk_size) dist_to_chunk = nx - (cx + chunk_size);
                if (ny < cy) dist_to_chunk += cy - ny;
                else if (ny > cy + chunk_size) dist_to_chunk += ny - (cy + chunk_size);
                
                if (dist_to_chunk < chunk_size * 0.5) {
                    sites[extra_idx].x = nx;
                    sites[extra_idx].y = ny;
                    extra_idx++;
                }
            }
        }
    }
    
    *num_sites = extra_idx;
}

/* Draw a Voronoi cell as a polygon */
void draw_voronoi_cell(uint8_t* pixels, int width, int height,
                      float site_x, float site_y,
                      Point* all_sites, int num_sites,
                      uint8_t r, uint8_t g, uint8_t b,
                      float scale, float offset_x, float offset_y) {
    
    /* For each pixel, check if this site is the closest */
    int px_center = (int)(site_x * scale + offset_x);
    int py_center = (int)(height - (site_y * scale + offset_y));
    
    /* Estimate cell bounds (rough approximation) */
    float max_dist = 100.0f;  /* Maximum pixel distance to check */
    
    int px_min = (int)(px_center - max_dist);
    int px_max = (int)(px_center + max_dist);
    int py_min = (int)(py_center - max_dist);
    int py_max = (int)(py_center + max_dist);
    
    /* Clip to image bounds */
    if (px_min < 0) px_min = 0;
    if (px_max >= width) px_max = width - 1;
    if (py_min < 0) py_min = 0;
    if (py_max >= height) py_max = height - 1;
    
    /* Check each pixel in the region */
    for (int py = py_min; py <= py_max; py++) {
        for (int px = px_min; px <= px_max; px++) {
            /* Convert pixel to world coordinates */
            float wx = (px - offset_x) / scale;
            float wy = (height - py - offset_y) / scale;
            
            /* Check if this site is closest */
            float min_dist = dist_sq(wx, wy, site_x, site_y);
            bool is_closest = true;
            bool is_edge = false;
            
            for (int j = 0; j < num_sites; j++) {
                if (all_sites[j].x == site_x && all_sites[j].y == site_y) continue;
                
                float d = dist_sq(wx, wy, all_sites[j].x, all_sites[j].y);
                if (d < min_dist) {
                    is_closest = false;
                    break;
                } else if (fabs(d - min_dist) < 0.5) {
                    is_edge = true;  /* Very close to edge */
                }
            }
            
            if (is_closest) {
                int idx = (py * width + px) * 3;
                
                if (is_edge) {
                    /* Draw edge darker */
                    pixels[idx + 0] = 40;
                    pixels[idx + 1] = 40;
                    pixels[idx + 2] = 60;
                } else {
                    /* Fill with cell color */
                    pixels[idx + 0] = r;
                    pixels[idx + 1] = g;
                    pixels[idx + 2] = b;
                }
            }
        }
    }
}

/* Generate Voronoi diagram with lazy chunk loading */
void generate_voronoi_lazy_grid(uint8_t* pixels, int width, int height,
                                float chunk_size, float view_x, float view_y,
                                float scale, bool show_chunks) {
    
    /* Clear to light background */
    memset(pixels, 245, width * height * 3);
    
    float offset_x = width / 2.0f - view_x * scale;
    float offset_y = height / 2.0f + view_y * scale;
    
    /* Determine which chunks are visible */
    int min_chunk_x = (int)floor((view_x - width/(2*scale)) / chunk_size) - 1;
    int max_chunk_x = (int)ceil((view_x + width/(2*scale)) / chunk_size) + 1;
    int min_chunk_y = (int)floor((view_y - height/(2*scale)) / chunk_size) - 1;
    int max_chunk_y = (int)ceil((view_y + height/(2*scale)) / chunk_size) + 1;
    
    /* Limit chunks to render */
    if (max_chunk_x - min_chunk_x > 10) {
        max_chunk_x = min_chunk_x + 10;
    }
    if (max_chunk_y - min_chunk_y > 10) {
        max_chunk_y = min_chunk_y + 10;
    }
    
    printf("Rendering chunks from (%d,%d) to (%d,%d)\n", 
           min_chunk_x, min_chunk_y, max_chunk_x, max_chunk_y);
    
    /* Process each visible chunk */
    for (int chunk_y = min_chunk_y; chunk_y <= max_chunk_y; chunk_y++) {
        for (int chunk_x = min_chunk_x; chunk_x <= max_chunk_x; chunk_x++) {
            
            /* Generate sites for this chunk (including neighbors for seamless boundaries) */
            Point sites[100];
            int num_sites = 0;
            generate_chunk_sites(chunk_x, chunk_y, chunk_size, sites, &num_sites, 100);
            
            /* Draw Voronoi cells for sites in this chunk */
            for (int i = 0; i < num_sites; i++) {
                /* Only draw cells whose sites are in the current chunk */
                float cx = chunk_x * chunk_size;
                float cy = chunk_y * chunk_size;
                
                if (sites[i].x >= cx && sites[i].x < cx + chunk_size &&
                    sites[i].y >= cy && sites[i].y < cy + chunk_size) {
                    
                    /* Color based on chunk and site position */
                    uint32_t color_seed = (uint32_t)(sites[i].x * 1000 + sites[i].y * 2000);
                    uint8_t r = 100 + (color_seed % 100);
                    uint8_t g = 120 + ((color_seed >> 8) % 80);
                    uint8_t b = 140 + ((color_seed >> 16) % 100);
                    
                    /* Tint by chunk for visibility */
                    if (show_chunks) {
                        r = (r + (chunk_x + 10) * 20) % 256;
                        g = (g + (chunk_y + 10) * 20) % 256;
                    }
                    
                    draw_voronoi_cell(pixels, width, height,
                                     sites[i].x, sites[i].y,
                                     sites, num_sites,
                                     r, g, b,
                                     scale, offset_x, offset_y);
                }
            }
            
            /* Optionally draw chunk boundaries */
            if (show_chunks) {
                float bx1 = chunk_x * chunk_size;
                float by1 = chunk_y * chunk_size;
                float bx2 = bx1 + chunk_size;
                float by2 = by1 + chunk_size;
                
                /* Draw chunk boundary lines */
                for (float t = 0; t <= 1.0; t += 0.01) {
                    /* Top and bottom edges */
                    int px = (int)((bx1 + t * (bx2 - bx1)) * scale + offset_x);
                    int py1 = (int)(height - (by1 * scale + offset_y));
                    int py2 = (int)(height - (by2 * scale + offset_y));
                    
                    if (px >= 0 && px < width) {
                        if (py1 >= 0 && py1 < height) {
                            int idx = (py1 * width + px) * 3;
                            pixels[idx] = 255;
                            pixels[idx + 1] = 100;
                            pixels[idx + 2] = 100;
                        }
                        if (py2 >= 0 && py2 < height) {
                            int idx = (py2 * width + px) * 3;
                            pixels[idx] = 255;
                            pixels[idx + 1] = 100;
                            pixels[idx + 2] = 100;
                        }
                    }
                    
                    /* Left and right edges */
                    int py = (int)(height - ((by1 + t * (by2 - by1)) * scale + offset_y));
                    int px1 = (int)(bx1 * scale + offset_x);
                    int px2 = (int)(bx2 * scale + offset_x);
                    
                    if (py >= 0 && py < height) {
                        if (px1 >= 0 && px1 < width) {
                            int idx = (py * width + px1) * 3;
                            pixels[idx] = 255;
                            pixels[idx + 1] = 100;
                            pixels[idx + 2] = 100;
                        }
                        if (px2 >= 0 && px2 < width) {
                            int idx = (py * width + px2) * 3;
                            pixels[idx] = 255;
                            pixels[idx + 1] = 100;
                            pixels[idx + 2] = 100;
                        }
                    }
                }
            }
        }
    }
    
    /* Add title */
    const char* title = show_chunks ? 
        "PLANAR LAZY MESH GRID - VORONOI (CHUNKS VISIBLE)" :
        "PLANAR LAZY MESH GRID - VORONOI";
    int title_len = strlen(title);
    for (int i = 0; i < title_len; i++) {
        int px = 20 + i * 6;
        int py = 20;
        if (px < width - 5 && py < height - 5) {
            for (int dy = -2; dy <= 2; dy++) {
                for (int dx = -2; dx <= 2; dx++) {
                    if (abs(dx) + abs(dy) <= 3) {
                        int idx = ((py + dy) * width + (px + dx)) * 3;
                        if (idx >= 0 && idx < width * height * 3 - 2) {
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

int main() {
    printf("=== PlanarLazyMeshGrid Voronoi Visualization ===\n\n");
    
    int width = 800;
    int height = 600;
    uint8_t* pixels = (uint8_t*)malloc(width * height * 3);
    
    if (!pixels) {
        fprintf(stderr, "Failed to allocate memory for image\n");
        return 1;
    }
    
    /* Generate Voronoi with visible chunk boundaries */
    printf("Generating Voronoi diagram with visible chunks...\n");
    generate_voronoi_lazy_grid(pixels, width, height, 
                               100.0f,  /* chunk size */
                               0.0f,    /* view x */
                               0.0f,    /* view y */
                               2.0f,    /* scale */
                               true);   /* show chunks */
    write_ppm("voronoi_lazy_chunks.ppm", pixels, width, height);
    
    /* Generate seamless Voronoi without visible chunks */
    printf("\nGenerating seamless Voronoi diagram...\n");
    generate_voronoi_lazy_grid(pixels, width, height,
                               100.0f,  /* chunk size */
                               0.0f,    /* view x */
                               0.0f,    /* view y */
                               2.0f,    /* scale */
                               false);  /* hide chunks */
    write_ppm("voronoi_lazy_seamless.ppm", pixels, width, height);
    
    /* Generate zoomed in view to show detail */
    printf("\nGenerating zoomed Voronoi detail...\n");
    generate_voronoi_lazy_grid(pixels, width, height,
                               100.0f,  /* chunk size */
                               50.0f,   /* view x - shifted */
                               30.0f,   /* view y - shifted */
                               5.0f,    /* scale - zoomed in */
                               false);  /* hide chunks */
    write_ppm("voronoi_lazy_detail.ppm", pixels, width, height);
    
    free(pixels);
    
    printf("\n=== Demo Complete ===\n");
    printf("Generated PPM files:\n");
    printf("  - voronoi_lazy_chunks.ppm (shows chunk boundaries)\n");
    printf("  - voronoi_lazy_seamless.ppm (seamless Voronoi)\n");
    printf("  - voronoi_lazy_detail.ppm (zoomed detail view)\n");
    printf("\nConvert to PNG with:\n");
    printf("  sips -s format png voronoi_lazy_chunks.ppm --out voronoi_lazy_chunks.png\n");
    printf("  sips -s format png voronoi_lazy_seamless.ppm --out voronoi_lazy_seamless.png\n");
    printf("  sips -s format png voronoi_lazy_detail.ppm --out voronoi_lazy_detail.png\n");
    
    return 0;
}
