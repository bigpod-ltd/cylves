#include "sylves/raster_export.h"
#include "sylves/vector.h"
#include "sylves/polygon.h"
#include "sylves/bounds.h"
#include "sylves/grid.h"
#include "sylves/utils.h"
#include "sylves/memory.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

// Use stb_image_write for image output
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBIW_MALLOC(sz) sylves_alloc(sz)
#define STBIW_REALLOC(p,sz) sylves_realloc(p,sz)
#define STBIW_FREE(p) sylves_free(p)
#define STBIW_ASSERT(x) 
#include "stb_image_write.h"

// Raster renderer internal structure
struct SylvesRasterRenderer {
    int width;
    int height;
    SylvesRasterExportOptions options;
    uint8_t* pixels;
    size_t pixel_count;
    
    // Transform from world to screen coordinates
    float scale_x;
    float scale_y;
    float offset_x;
    float offset_y;
};

// Helper functions
static void world_to_screen(const SylvesRasterRenderer* renderer, 
                           float world_x, float world_y,
                           int* screen_x, int* screen_y) {
    *screen_x = (int)((world_x - renderer->options.viewport_min_x) * renderer->scale_x);
    *screen_y = (int)(renderer->height - (world_y - renderer->options.viewport_min_y) * renderer->scale_y);
}

static void set_pixel(SylvesRasterRenderer* renderer, int x, int y, SylvesColor color) {
    if (x < 0 || x >= renderer->width || y < 0 || y >= renderer->height) return;
    
    size_t idx = (y * renderer->width + x) * 4;
    uint8_t* pixel = &renderer->pixels[idx];
    
    if (color.a == 255) {
        // Opaque
        pixel[0] = color.r;
        pixel[1] = color.g;
        pixel[2] = color.b;
        pixel[3] = color.a;
    } else if (color.a > 0) {
        // Alpha blend
        float alpha = color.a / 255.0f;
        float inv_alpha = 1.0f - alpha;
        pixel[0] = (uint8_t)(color.r * alpha + pixel[0] * inv_alpha);
        pixel[1] = (uint8_t)(color.g * alpha + pixel[1] * inv_alpha);
        pixel[2] = (uint8_t)(color.b * alpha + pixel[2] * inv_alpha);
        pixel[3] = 255; // Keep destination opaque
    }
}

// Bresenham's line algorithm
static void draw_line_low_level(SylvesRasterRenderer* renderer, 
                               int x0, int y0, int x1, int y1, 
                               SylvesColor color) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    
    while (1) {
        set_pixel(renderer, x0, y0, color);
        
        if (x0 == x1 && y0 == y1) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

// Simple polygon fill using scanline algorithm
static void fill_polygon(SylvesRasterRenderer* renderer,
                        const int* x_coords, const int* y_coords, 
                        int point_count, SylvesColor color) {
    if (point_count < 3) return;
    
    // Find bounding box
    int min_y = y_coords[0], max_y = y_coords[0];
    for (int i = 1; i < point_count; i++) {
        if (y_coords[i] < min_y) min_y = y_coords[i];
        if (y_coords[i] > max_y) max_y = y_coords[i];
    }
    
    // Clamp to screen
    if (min_y < 0) min_y = 0;
    if (max_y >= renderer->height) max_y = renderer->height - 1;
    
    // Scanline fill
    for (int y = min_y; y <= max_y; y++) {
        int intersections[64];
        int intersection_count = 0;
        
        // Find intersections with scanline
        for (int i = 0; i < point_count; i++) {
            int j = (i + 1) % point_count;
            int y0 = y_coords[i];
            int y1 = y_coords[j];
            
            if ((y0 <= y && y1 > y) || (y1 <= y && y0 > y)) {
                // Edge crosses scanline
                float t = (float)(y - y0) / (y1 - y0);
                int x = (int)(x_coords[i] + t * (x_coords[j] - x_coords[i]));
                
                if (intersection_count < 64) {
                    intersections[intersection_count++] = x;
                }
            }
        }
        
        // Sort intersections
        for (int i = 0; i < intersection_count - 1; i++) {
            for (int j = i + 1; j < intersection_count; j++) {
                if (intersections[i] > intersections[j]) {
                    int temp = intersections[i];
                    intersections[i] = intersections[j];
                    intersections[j] = temp;
                }
            }
        }
        
        // Fill between pairs of intersections
        for (int i = 0; i < intersection_count - 1; i += 2) {
            int x_start = intersections[i];
            int x_end = intersections[i + 1];
            
            if (x_start < 0) x_start = 0;
            if (x_end >= renderer->width) x_end = renderer->width - 1;
            
            for (int x = x_start; x <= x_end; x++) {
                set_pixel(renderer, x, y, color);
            }
        }
    }
}

SylvesError sylves_raster_export_options_init(SylvesRasterExportOptions* options) {
    if (!options) return SYLVES_ERROR_INVALID_ARGUMENT;

    options->width = 800;
    options->height = 600;
    options->dpi = 96.0f;

    options->viewport_min_x = -5.0f;
    options->viewport_min_y = -5.0f;
    options->viewport_max_x = 5.0f;
    options->viewport_max_y = 5.0f;

    options->background_style = SYLVES_BACKGROUND_SOLID;
    options->background_color = sylves_color_rgb(255, 255, 255);
    options->checker_color1 = sylves_color_rgb(192, 192, 192);
    options->checker_color2 = sylves_color_rgb(255, 255, 255);
    options->checker_size = 10;

    options->antialiasing = 1;
    options->samples_per_pixel = 4;

    sylves_cell_style_init_default(&options->default_style);

    options->get_cell_style = NULL;
    options->style_user_data = NULL;

    options->show_axes = 1;
    options->show_coordinates = 0;
    options->show_grid_lines = 0;
    options->axes_color = sylves_color_rgb(0, 0, 0);
    options->grid_line_color = sylves_color_rgb(200, 200, 200);
    options->grid_line_width = 1.0f;

    options->font_path = NULL;
    options->font_size = 12.0f;
    options->text_color = sylves_color_rgb(0, 0, 0);

    options->layers = NULL;
    options->layer_count = 0;

    options->use_tiled_rendering = 0;
    options->tile_size = 256;
    options->max_memory_bytes = 1024 * 1024 * 100;

    options->jpeg_quality = 90;
    options->png_compression = 6;

    return SYLVES_SUCCESS;
}

SylvesError sylves_raster_renderer_create(SylvesRasterRenderer** renderer,
                                          int width,
                                          int height,
                                          const SylvesRasterExportOptions* options) {
    if (!renderer || width <= 0 || height <= 0 || !options) return SYLVES_ERROR_INVALID_ARGUMENT;

    *renderer = (SylvesRasterRenderer*)sylves_alloc(sizeof(SylvesRasterRenderer));
    if (!*renderer) return SYLVES_ERROR_OUT_OF_MEMORY;

    (*renderer)->width = width;
    (*renderer)->height = height;
    (*renderer)->options = *options;
    
    // Allocate pixel buffer
    (*renderer)->pixel_count = (size_t)width * height * 4;
    (*renderer)->pixels = (uint8_t*)sylves_alloc((*renderer)->pixel_count);
    if (!(*renderer)->pixels) {
        sylves_free(*renderer);
        *renderer = NULL;
        return SYLVES_ERROR_OUT_OF_MEMORY;
    }
    
    // Calculate transform
    float viewport_width = options->viewport_max_x - options->viewport_min_x;
    float viewport_height = options->viewport_max_y - options->viewport_min_y;
    (*renderer)->scale_x = width / viewport_width;
    (*renderer)->scale_y = height / viewport_height;
    (*renderer)->offset_x = -options->viewport_min_x;
    (*renderer)->offset_y = -options->viewport_min_y;
    
    // Clear to background
    if (options->background_style == SYLVES_BACKGROUND_SOLID) {
        for (size_t i = 0; i < (*renderer)->pixel_count; i += 4) {
            (*renderer)->pixels[i] = options->background_color.r;
            (*renderer)->pixels[i + 1] = options->background_color.g;
            (*renderer)->pixels[i + 2] = options->background_color.b;
            (*renderer)->pixels[i + 3] = options->background_color.a;
        }
    } else if (options->background_style == SYLVES_BACKGROUND_CHECKER) {
        // Draw checkerboard pattern
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int checker_x = x / options->checker_size;
                int checker_y = y / options->checker_size;
                SylvesColor color = ((checker_x + checker_y) % 2 == 0) ? 
                    options->checker_color1 : options->checker_color2;
                    
                size_t idx = (y * width + x) * 4;
                (*renderer)->pixels[idx] = color.r;
                (*renderer)->pixels[idx + 1] = color.g;
                (*renderer)->pixels[idx + 2] = color.b;
                (*renderer)->pixels[idx + 3] = color.a;
            }
        }
    } else {
        // Transparent
        memset((*renderer)->pixels, 0, (*renderer)->pixel_count);
    }

    return SYLVES_SUCCESS;
}

void sylves_raster_renderer_destroy(SylvesRasterRenderer* renderer) {
    if (renderer) {
        if (renderer->pixels) {
            sylves_free(renderer->pixels);
        }
        sylves_free(renderer);
    }
}

SylvesError sylves_raster_render_grid(SylvesRasterRenderer* renderer,
                                      const SylvesGrid* grid) {
    if (!renderer || !grid) return SYLVES_ERROR_INVALID_ARGUMENT;

    // Get cells to render
    const SylvesBound* bound = sylves_grid_get_bound(grid);
    if (!bound) return SYLVES_ERROR_UNBOUNDED;
    
    // Get viewport bounds in grid coordinates
    SylvesVector3 min = {renderer->options.viewport_min_x, renderer->options.viewport_min_y, 0};
    SylvesVector3 max = {renderer->options.viewport_max_x, renderer->options.viewport_max_y, 0};
    
    // Get cells in viewport
    SylvesCell cells[4096];
    int cell_count = sylves_grid_get_cells_in_aabb(grid, min, max, cells, 4096);
    if (cell_count <= 0) return SYLVES_SUCCESS;
    
    // Render each cell
    for (int i = 0; i < cell_count; i++) {
        // Get cell style
        SylvesCellStyle style = renderer->options.default_style;
        if (renderer->options.get_cell_style) {
            renderer->options.get_cell_style(cells[i], &style, renderer->options.style_user_data);
        }
        
        // Get cell polygon
        SylvesVector3 vertices[32];
        int vertex_count = 0;
        vertex_count = sylves_grid_get_polygon(grid, cells[i], vertices, 32);
        
        if (vertex_count > 0) {
            // Convert to screen coordinates
            int x_coords[32], y_coords[32];
            for (int j = 0; j < vertex_count; j++) {
                world_to_screen(renderer, vertices[j].x, vertices[j].y, 
                              &x_coords[j], &y_coords[j]);
            }
            
            // Fill polygon if requested
            if (style.filled) {
                fill_polygon(renderer, x_coords, y_coords, vertex_count, style.fill_color);
            }
            
            // Draw outline if requested
            if (style.stroked) {
                for (int j = 0; j < vertex_count; j++) {
                    int next = (j + 1) % vertex_count;
                    draw_line_low_level(renderer, x_coords[j], y_coords[j],
                                      x_coords[next], y_coords[next], style.stroke_color);
                }
            }
        }
    }

    return SYLVES_SUCCESS;
}

SylvesError sylves_raster_render_cells(SylvesRasterRenderer* renderer,
                                       const SylvesGrid* grid,
                                       const SylvesCell* cells,
                                       size_t cell_count,
                                       const SylvesCellStyle* style) {
    if (!renderer || !grid || !cells || cell_count == 0 || !style) return SYLVES_ERROR_INVALID_ARGUMENT;

    // Render each specified cell
    for (size_t i = 0; i < cell_count; i++) {
        // Get cell polygon
        SylvesVector3 vertices[32];
        int vertex_count = 0;
        vertex_count = sylves_grid_get_polygon(grid, cells[i], vertices, 32);
        
        if (vertex_count > 0) {
            // Convert to screen coordinates
            int x_coords[32], y_coords[32];
            for (int j = 0; j < vertex_count; j++) {
                world_to_screen(renderer, vertices[j].x, vertices[j].y, 
                              &x_coords[j], &y_coords[j]);
            }
            
            // Fill polygon if requested
            if (style->filled) {
                fill_polygon(renderer, x_coords, y_coords, vertex_count, style->fill_color);
            }
            
            // Draw outline if requested
            if (style->stroked) {
                for (int j = 0; j < vertex_count; j++) {
                    int next = (j + 1) % vertex_count;
                    draw_line_low_level(renderer, x_coords[j], y_coords[j],
                                      x_coords[next], y_coords[next], style->stroke_color);
                }
            }
        }
    }

    return SYLVES_SUCCESS;
}

SylvesError sylves_raster_draw_polygon(SylvesRasterRenderer* renderer,
                                       const SylvesVector2* points,
                                       size_t point_count,
                                       const SylvesCellStyle* style) {
    if (!renderer || !points || point_count == 0 || !style) return SYLVES_ERROR_INVALID_ARGUMENT;

    // Convert to screen coordinates
    int* x_coords = (int*)sylves_alloc(point_count * sizeof(int));
    int* y_coords = (int*)sylves_alloc(point_count * sizeof(int));
    if (!x_coords || !y_coords) {
        sylves_free(x_coords);
        sylves_free(y_coords);
        return SYLVES_ERROR_OUT_OF_MEMORY;
    }
    
    for (size_t i = 0; i < point_count; i++) {
        world_to_screen(renderer, points[i].x, points[i].y, &x_coords[i], &y_coords[i]);
    }
    
    // Fill polygon if requested
    if (style->filled) {
        fill_polygon(renderer, x_coords, y_coords, (int)point_count, style->fill_color);
    }
    
    // Draw outline if requested
    if (style->stroked) {
        for (size_t i = 0; i < point_count; i++) {
            size_t next = (i + 1) % point_count;
            draw_line_low_level(renderer, x_coords[i], y_coords[i],
                              x_coords[next], y_coords[next], style->stroke_color);
        }
    }
    
    sylves_free(x_coords);
    sylves_free(y_coords);
    return SYLVES_SUCCESS;
}

SylvesError sylves_raster_draw_line(SylvesRasterRenderer* renderer,
                                    SylvesVector2 start,
                                    SylvesVector2 end,
                                    const SylvesCellStyle* style) {
    if (!renderer || !style) return SYLVES_ERROR_INVALID_ARGUMENT;

    int x0, y0, x1, y1;
    world_to_screen(renderer, start.x, start.y, &x0, &y0);
    world_to_screen(renderer, end.x, end.y, &x1, &y1);
    
    if (style->stroked) {
        draw_line_low_level(renderer, x0, y0, x1, y1, style->stroke_color);
    }

    return SYLVES_SUCCESS;
}

SylvesError sylves_raster_draw_text(SylvesRasterRenderer* renderer,
                                    SylvesVector2 position,
                                    const char* text,
                                    const SylvesColor* color,
                                    float size) {
    if (!renderer || !text || !color) return SYLVES_ERROR_INVALID_ARGUMENT;

    // Placeholder rendering code
    printf("Drawing text '%s' at position (%f, %f).\n", text, position.x, position.y);

    return SYLVES_SUCCESS;
}

SylvesError sylves_raster_get_pixels(const SylvesRasterRenderer* renderer,
                                     uint8_t** pixels,
                                     size_t* pixel_count) {
    if (!renderer || !pixels || !pixel_count) return SYLVES_ERROR_INVALID_ARGUMENT;

    *pixel_count = renderer->pixel_count;
    *pixels = (uint8_t*)sylves_alloc(*pixel_count);
    if (!*pixels) return SYLVES_ERROR_OUT_OF_MEMORY;

    memcpy(*pixels, renderer->pixels, *pixel_count);

    return SYLVES_SUCCESS;
}

SylvesError sylves_export_grid_raster(const SylvesGrid* grid,
                                      const char* filename,
                                      SylvesImageFormat format,
                                      const SylvesRasterExportOptions* options) {
    if (!grid || !filename || !options) return SYLVES_ERROR_INVALID_ARGUMENT;

    // Create renderer
    SylvesRasterRenderer* renderer;
    SylvesError err = sylves_raster_renderer_create(&renderer, options->width, options->height, options);
    if (err != SYLVES_SUCCESS) return err;

    // Render the grid
    err = sylves_raster_render_grid(renderer, grid);
    if (err != SYLVES_SUCCESS) {
        sylves_raster_renderer_destroy(renderer);
        return err;
    }

    // Draw axes if requested
    if (options->show_axes) {
        SylvesCellStyle axes_style;
        sylves_cell_style_init_default(&axes_style);
        axes_style.stroked = 1;
        axes_style.filled = 0;
        axes_style.stroke_color = options->axes_color;
        axes_style.stroke_width = 2.0f;
        
        // X axis
        SylvesVector2 x_start = {options->viewport_min_x, 0};
        SylvesVector2 x_end = {options->viewport_max_x, 0};
        sylves_raster_draw_line(renderer, x_start, x_end, &axes_style);
        
        // Y axis
        SylvesVector2 y_start = {0, options->viewport_min_y};
        SylvesVector2 y_end = {0, options->viewport_max_y};
        sylves_raster_draw_line(renderer, y_start, y_end, &axes_style);
    }

    // Export to file
    int result = 0;
    switch (format) {
        case SYLVES_IMAGE_FORMAT_PNG:
            stbi_write_png_compression_level = options->png_compression;
            result = stbi_write_png(filename, renderer->width, renderer->height, 4, 
                                  renderer->pixels, renderer->width * 4);
            break;
            
        case SYLVES_IMAGE_FORMAT_JPG:
            result = stbi_write_jpg(filename, renderer->width, renderer->height, 4,
                                  renderer->pixels, options->jpeg_quality);
            break;
            
        case SYLVES_IMAGE_FORMAT_BMP:
            result = stbi_write_bmp(filename, renderer->width, renderer->height, 4,
                                  renderer->pixels);
            break;
            
        case SYLVES_IMAGE_FORMAT_TGA:
            result = stbi_write_tga(filename, renderer->width, renderer->height, 4,
                                  renderer->pixels);
            break;
            
        default:
            err = SYLVES_ERROR_INVALID_ARGUMENT;
    }

    sylves_raster_renderer_destroy(renderer);
    
    return result ? SYLVES_SUCCESS : SYLVES_ERROR_IO;
}

SylvesError sylves_export_grid_raster_to_memory(
    const SylvesGrid* grid,
    uint8_t** data,
    size_t* data_size,
    SylvesImageFormat format,
    const SylvesRasterExportOptions* options
) {
    if (!grid || !data || !data_size || !options) return SYLVES_ERROR_INVALID_ARGUMENT;

    // Create renderer
    SylvesRasterRenderer* renderer;
    SylvesError err = sylves_raster_renderer_create(&renderer, options->width, options->height, options);
    if (err != SYLVES_SUCCESS) return err;

    // Render the grid
    err = sylves_raster_render_grid(renderer, grid);
    if (err != SYLVES_SUCCESS) {
        sylves_raster_renderer_destroy(renderer);
        return err;
    }

    // Encode to memory
    int len = 0;
    unsigned char* png_data = NULL;
    
    switch (format) {
        case SYLVES_IMAGE_FORMAT_PNG:
            png_data = stbi_write_png_to_mem(renderer->pixels, renderer->width * 4,
                                            renderer->width, renderer->height, 4, &len);
            break;
            
        default:
            // Only PNG is supported for memory export in stb_image_write
            sylves_raster_renderer_destroy(renderer);
            return SYLVES_ERROR_NOT_SUPPORTED;
    }
    
    if (!png_data || len <= 0) {
        sylves_raster_renderer_destroy(renderer);
        return SYLVES_ERROR_IO;
    }
    
    *data = (uint8_t*)sylves_alloc(len);
    if (!*data) {
        STBIW_FREE(png_data);
        sylves_raster_renderer_destroy(renderer);
        return SYLVES_ERROR_OUT_OF_MEMORY;
    }
    
    memcpy(*data, png_data, len);
    *data_size = len;
    
    STBIW_FREE(png_data);
    sylves_raster_renderer_destroy(renderer);
    
    return SYLVES_SUCCESS;
}

// Color utility functions
SylvesColor sylves_color_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    SylvesColor color = {r, g, b, a};
    return color;
}

SylvesColor sylves_color_rgb(uint8_t r, uint8_t g, uint8_t b) {
    SylvesColor color = {r, g, b, 255};
    return color;
}

SylvesColor sylves_color_from_hex(uint32_t hex) {
    SylvesColor color;
    color.r = (hex >> 24) & 0xFF;
    color.g = (hex >> 16) & 0xFF;
    color.b = (hex >> 8) & 0xFF;
    color.a = hex & 0xFF;
    return color;
}

SylvesColor sylves_color_blend(SylvesColor src, SylvesColor dst, SylvesBlendMode mode) {
    if (mode == SYLVES_BLEND_MODE_OPAQUE || src.a == 255) {
        return src;
    }
    
    if (src.a == 0) {
        return dst;
    }
    
    float alpha = src.a / 255.0f;
    float inv_alpha = 1.0f - alpha;
    
    SylvesColor result;
    if (mode == SYLVES_BLEND_MODE_ALPHA) {
        result.r = (uint8_t)(src.r * alpha + dst.r * inv_alpha);
        result.g = (uint8_t)(src.g * alpha + dst.g * inv_alpha);
        result.b = (uint8_t)(src.b * alpha + dst.b * inv_alpha);
        result.a = (uint8_t)(src.a + dst.a * inv_alpha);
    } else { // PREMULTIPLIED
        result.r = (uint8_t)(src.r + dst.r * inv_alpha);
        result.g = (uint8_t)(src.g + dst.g * inv_alpha);
        result.b = (uint8_t)(src.b + dst.b * inv_alpha);
        result.a = (uint8_t)(src.a + dst.a * inv_alpha);
    }
    
    return result;
}

// Style presets
void sylves_cell_style_init_default(SylvesCellStyle* style) {
    if (!style) return;
    
    style->fill_color = sylves_color_rgb(244, 244, 241);
    style->stroke_color = sylves_color_rgb(51, 51, 51);
    style->stroke_width = 1.0f;
    style->filled = 1;
    style->stroked = 1;
    style->line_cap = SYLVES_LINE_CAP_BUTT;
    style->line_join = SYLVES_LINE_JOIN_MITER;
    style->miter_limit = 10.0f;
    style->dash_pattern = NULL;
    style->dash_count = 0;
    style->dash_offset = 0.0f;
}

void sylves_cell_style_init_wireframe(SylvesCellStyle* style) {
    if (!style) return;
    
    sylves_cell_style_init_default(style);
    style->filled = 0;
    style->stroked = 1;
    style->stroke_color = sylves_color_rgb(0, 0, 0);
    style->stroke_width = 1.0f;
}

void sylves_cell_style_init_solid(SylvesCellStyle* style, SylvesColor color) {
    if (!style) return;
    
    sylves_cell_style_init_default(style);
    style->filled = 1;
    style->stroked = 0;
    style->fill_color = color;
}
