#ifndef SYLVES_RASTER_EXPORT_H
#define SYLVES_RASTER_EXPORT_H

#include "sylves/types.h"
#include "sylves/grid.h"
#include "sylves/errors.h"

#ifdef __cplusplus
extern "C" {
#endif

// Image format types
typedef enum {
    SYLVES_IMAGE_FORMAT_PNG,
    SYLVES_IMAGE_FORMAT_JPG,
    SYLVES_IMAGE_FORMAT_BMP,
    SYLVES_IMAGE_FORMAT_TGA
} SylvesImageFormat;

// Blend mode for transparency
typedef enum {
    SYLVES_BLEND_MODE_OPAQUE,
    SYLVES_BLEND_MODE_ALPHA,
    SYLVES_BLEND_MODE_PREMULTIPLIED
} SylvesBlendMode;

// Background style
typedef enum {
    SYLVES_BACKGROUND_SOLID,
    SYLVES_BACKGROUND_TRANSPARENT,
    SYLVES_BACKGROUND_CHECKER
} SylvesBackgroundStyle;

// Line cap style
typedef enum {
    SYLVES_LINE_CAP_BUTT,
    SYLVES_LINE_CAP_ROUND,
    SYLVES_LINE_CAP_SQUARE
} SylvesLineCap;

// Line join style
typedef enum {
    SYLVES_LINE_JOIN_MITER,
    SYLVES_LINE_JOIN_ROUND,
    SYLVES_LINE_JOIN_BEVEL
} SylvesLineJoin;

// RGBA color
typedef struct SylvesColor {
    uint8_t r, g, b, a;
} SylvesColor;

// Style for cell rendering
typedef struct SylvesCellStyle {
    SylvesColor fill_color;
    SylvesColor stroke_color;
    float stroke_width;
    int filled;
    int stroked;
    
    // Advanced stroke options
    SylvesLineCap line_cap;
    SylvesLineJoin line_join;
    float miter_limit;
    float* dash_pattern;
    size_t dash_count;
    float dash_offset;
} SylvesCellStyle;

// Layer information
typedef struct SylvesRenderLayer {
    const char* name;
    int visible;
    float opacity;
    SylvesBlendMode blend_mode;
    int z_order;
} SylvesRenderLayer;

// Raster export options
typedef struct SylvesRasterExportOptions {
    // Image dimensions
    int width;
    int height;
    float dpi;
    
    // Viewport (world coordinates)
    float viewport_min_x;
    float viewport_min_y;
    float viewport_max_x;
    float viewport_max_y;
    
    // Background
    SylvesBackgroundStyle background_style;
    SylvesColor background_color;
    SylvesColor checker_color1;
    SylvesColor checker_color2;
    int checker_size;
    
    // Anti-aliasing
    int antialiasing;
    int samples_per_pixel;  // For supersampling
    
    // Default style
    SylvesCellStyle default_style;
    
    // Callbacks for custom styling
    void (*get_cell_style)(SylvesCell cell, SylvesCellStyle* style, void* user_data);
    void* style_user_data;
    
    // Grid aids
    int show_axes;
    int show_coordinates;
    int show_grid_lines;
    SylvesColor axes_color;
    SylvesColor grid_line_color;
    float grid_line_width;
    
    // Font settings (if text rendering enabled)
    const char* font_path;
    float font_size;
    SylvesColor text_color;
    
    // Layers
    SylvesRenderLayer* layers;
    size_t layer_count;
    
    // Performance options
    int use_tiled_rendering;
    int tile_size;
    size_t max_memory_bytes;
    
    // Output options
    int jpeg_quality;  // For JPEG format (0-100)
    int png_compression;  // For PNG format (0-9)
} SylvesRasterExportOptions;

// Raster renderer context
typedef struct SylvesRasterRenderer SylvesRasterRenderer;

// Initialize default raster export options
SylvesError sylves_raster_export_options_init(SylvesRasterExportOptions* options);

// Create raster renderer
SylvesError sylves_raster_renderer_create(
    SylvesRasterRenderer** renderer,
    int width,
    int height,
    const SylvesRasterExportOptions* options
);

// Destroy raster renderer
void sylves_raster_renderer_destroy(SylvesRasterRenderer* renderer);

// Render grid to raster
SylvesError sylves_raster_render_grid(
    SylvesRasterRenderer* renderer,
    const SylvesGrid* grid
);

// Render specific cells
SylvesError sylves_raster_render_cells(
    SylvesRasterRenderer* renderer,
    const SylvesGrid* grid,
    const SylvesCell* cells,
    size_t cell_count,
    const SylvesCellStyle* style
);

// Draw primitives
SylvesError sylves_raster_draw_polygon(
    SylvesRasterRenderer* renderer,
    const SylvesVector2* points,
    size_t point_count,
    const SylvesCellStyle* style
);

SylvesError sylves_raster_draw_line(
    SylvesRasterRenderer* renderer,
    SylvesVector2 start,
    SylvesVector2 end,
    const SylvesCellStyle* style
);

SylvesError sylves_raster_draw_text(
    SylvesRasterRenderer* renderer,
    SylvesVector2 position,
    const char* text,
    const SylvesColor* color,
    float size
);

// Get rendered image data
SylvesError sylves_raster_get_pixels(
    const SylvesRasterRenderer* renderer,
    uint8_t** pixels,
    size_t* pixel_count
);

// Export grid to raster file
SylvesError sylves_export_grid_raster(
    const SylvesGrid* grid,
    const char* filename,
    SylvesImageFormat format,
    const SylvesRasterExportOptions* options
);

// Export to memory buffer
SylvesError sylves_export_grid_raster_to_memory(
    const SylvesGrid* grid,
    uint8_t** data,
    size_t* data_size,
    SylvesImageFormat format,
    const SylvesRasterExportOptions* options
);

// Utility functions
SylvesColor sylves_color_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
SylvesColor sylves_color_rgb(uint8_t r, uint8_t g, uint8_t b);
SylvesColor sylves_color_from_hex(uint32_t hex);
SylvesColor sylves_color_blend(SylvesColor src, SylvesColor dst, SylvesBlendMode mode);

// Default style presets
void sylves_cell_style_init_default(SylvesCellStyle* style);
void sylves_cell_style_init_wireframe(SylvesCellStyle* style);
void sylves_cell_style_init_solid(SylvesCellStyle* style, SylvesColor color);

#ifdef __cplusplus
}
#endif

#endif // SYLVES_RASTER_EXPORT_H
