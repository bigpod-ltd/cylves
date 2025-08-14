#ifndef SYLVES_SVG_EXPORT_H
#define SYLVES_SVG_EXPORT_H

#include "sylves/types.h"
#include "sylves/grid.h"
#include "sylves/matrix.h"
#include "sylves/errors.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// SVG export options structure
typedef struct SylvesSvgOptions {
    // Viewport settings
    float min_x;
    float min_y; 
    float max_x;
    float max_y;
    
    // Styling options
    float stroke_width;
    const char* fill_color;      // Default fill color
    const char* stroke_color;    // Default stroke color
    
    // Cell styling callbacks
    const char* (*get_cell_fill)(SylvesCell cell, void* user_data);
    const char* (*get_cell_text)(SylvesCell cell, void* user_data);
    void* user_data;
    
    // Display options
    int show_coordinates;        // Show cell coordinates
    int coordinate_dimensions;   // Number of dimensions to display (2 or 3)
    double text_scale;          // Scale for coordinate text
    int include_dual;           // Include dual grid
    int trim;                   // Trim cells outside viewport
    
    // Transform
    SylvesMatrix4x4 transform;  // Additional transform to apply
} SylvesSvgOptions;

// SVG builder structure
typedef struct SylvesSvgBuilder {
    FILE* file;
    SylvesMatrix4x4 flip_y;
    float stroke_width;
} SylvesSvgBuilder;

// Initialize default SVG options
SylvesError sylves_svg_options_init(SylvesSvgOptions* options);

// Create SVG builder
SylvesError sylves_svg_builder_create(SylvesSvgBuilder** builder, FILE* file);

// Destroy SVG builder
void sylves_svg_builder_destroy(SylvesSvgBuilder* builder);

// Begin SVG document
SylvesError sylves_svg_begin(SylvesSvgBuilder* builder, const SylvesSvgOptions* options);

// End SVG document
SylvesError sylves_svg_end(SylvesSvgBuilder* builder);

// Draw a single cell
SylvesError sylves_svg_draw_cell(
    SylvesSvgBuilder* builder,
    const SylvesGrid* grid,
    SylvesCell cell,
    const char* fill_color
);

// Draw coordinate label for a cell
SylvesError sylves_svg_draw_coordinate_label(
    SylvesSvgBuilder* builder,
    const SylvesGrid* grid,
    SylvesCell cell,
    int dimensions,
    double text_scale,
    const char* text
);

// Write path commands for vertices
SylvesError sylves_svg_write_path_commands(
    FILE* file,
    const SylvesVector3* vertices,
    size_t vertex_count,
    SylvesMatrix4x4 transform,
    int close_path
);

// Export grid to SVG file
SylvesError sylves_export_grid_svg(
    const SylvesGrid* grid,
    const char* filename,
    const SylvesSvgOptions* options
);

// Export multiple grids to SVG file
SylvesError sylves_export_grids_svg(
    const SylvesGrid** grids,
    size_t grid_count,
    const char* filename,
    const SylvesSvgOptions* options
);

#ifdef __cplusplus
}
#endif

#endif // SYLVES_SVG_EXPORT_H
