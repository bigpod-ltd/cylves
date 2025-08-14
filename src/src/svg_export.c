#include "sylves/svg_export.h"
#include "sylves/vector.h"
#include "sylves/polygon.h"
#include "sylves/grid.h"
#include "sylves/bounds.h"
#include "sylves/utils.h"
#include <stdlib.h>
#include <string.h>

SylvesError sylves_svg_options_init(SylvesSvgOptions* options) {
    if (!options) return SYLVES_ERROR_INVALID_ARGUMENT;

    options->min_x = -5.0f;
    options->min_y = -5.0f;
    options->max_x = 5.0f;
    options->max_y = 5.0f;
    options->stroke_width = 0.1f;
    options->fill_color = "rgb(244, 244, 241)";
    options->stroke_color = "rgb(51, 51, 51)";
    options->get_cell_fill = NULL;
    options->get_cell_text = NULL;
    options->user_data = NULL;
    options->show_coordinates = 0;
    options->coordinate_dimensions = 3;
    options->text_scale = 1.0;
    options->include_dual = 0;
    options->trim = 0;
    options->transform = sylves_matrix4x4_identity();

    return SYLVES_SUCCESS;
}

SylvesError sylves_svg_builder_create(SylvesSvgBuilder** builder, FILE* file) {
    if (!builder || !file) return SYLVES_ERROR_INVALID_ARGUMENT;

    *builder = (SylvesSvgBuilder*)malloc(sizeof(SylvesSvgBuilder));
    if (!*builder) return SYLVES_ERROR_OUT_OF_MEMORY;

    (*builder)->file = file;
    (*builder)->flip_y = sylves_matrix4x4_scale((SylvesVector3){1.0, -1.0, 1.0});
    (*builder)->stroke_width = 0.1f;

    return SYLVES_SUCCESS;
}

void sylves_svg_builder_destroy(SylvesSvgBuilder* builder) {
    if (builder) {
        free(builder);
    }
}

SylvesError sylves_svg_begin(SylvesSvgBuilder* builder, const SylvesSvgOptions* options) {
    if (!builder || !options) return SYLVES_ERROR_INVALID_ARGUMENT;

    fprintf(builder->file, "<svg viewBox=\"%f %f %f %f\" xmlns=\"http://www.w3.org/2000/svg\">\n",
            options->min_x, -options->max_y, options->max_x - options->min_x, options->max_y - options->min_y);
    fprintf(builder->file, "<style>\n");
    fprintf(builder->file, ".cell-path { stroke-linejoin: round; fill: %s; stroke: %s; stroke-width: %f }\n",
            options->fill_color, options->stroke_color, options->stroke_width);
    fprintf(builder->file, ".dual .cell-path { fill: none; stroke: rgb(255, 0, 0); stroke-opacity: 0.5; stroke-width: %f }\n",
            options->stroke_width / 3.0f);
    fprintf(builder->file, "</style>\n");

    return SYLVES_SUCCESS;
}

SylvesError sylves_svg_end(SylvesSvgBuilder* builder) {
    if (!builder) return SYLVES_ERROR_INVALID_ARGUMENT;

    fprintf(builder->file, "</svg>\n");

    return SYLVES_SUCCESS;
}

SylvesError sylves_svg_draw_cell(SylvesSvgBuilder* builder, const SylvesGrid* grid, SylvesCell cell, const char* fill_color) {
    if (!builder || !grid) return SYLVES_ERROR_INVALID_ARGUMENT;

    const char* style = fill_color ? fill_color : "none";
    SylvesVector3 vertices[32]; // Max vertices for a cell
    int vertex_count = 0;
    
    vertex_count = sylves_grid_get_polygon(grid, cell, vertices, 32);
    
    if (vertex_count <= 0) {
        return SYLVES_ERROR_INVALID_CELL;
    }

    fprintf(builder->file, "<!-- Cell (%d, %d, %d) -->\n", cell.x, cell.y, cell.z);
    fprintf(builder->file, "<path class=\"cell-path\"");
    if (fill_color) {
        fprintf(builder->file, " style=\"fill: %s\"", style);
    }
    fprintf(builder->file, " d=\"");
    sylves_svg_write_path_commands(builder->file, vertices, vertex_count,
                                   builder->flip_y, 1);
    fprintf(builder->file, "\" />\n");

    return SYLVES_SUCCESS;
}

SylvesError sylves_svg_draw_coordinate_label(SylvesSvgBuilder* builder, const SylvesGrid* grid, SylvesCell cell, int dimensions, double text_scale, const char* text) {
    if (!builder || !grid) return SYLVES_ERROR_INVALID_ARGUMENT;

    const char* stroke_text_style = "fill: rgb(51, 51, 51); font-size: 0.3px;stroke: white; stroke-width: 0.05";
    const char* text_style = "fill: rgb(51, 51, 51); font-size: 0.3px;";
    const char* styles[2] = { stroke_text_style, text_style };

    SylvesVector3 center;
    center = sylves_grid_get_cell_center(grid, cell);

    center = sylves_matrix4x4_multiply_point(&builder->flip_y, center);

    fprintf(builder->file, "<g transform=\"translate(%f,%f) scale(%f)\">\n", center.x, center.y + 0.08, text_scale);
    for (int i = 0; i < 2; i++) {
        fprintf(builder->file, "<text text-anchor=\"middle\" alignment-baseline=\"middle\" style=\"%s\">",
                styles[i]);
        if (text) {
            fprintf(builder->file, "%s", text);
        } else {
            fprintf(builder->file, "%d, %d, %d", cell.x, cell.y, cell.z);
        }
        fprintf(builder->file, "</text>\n");
    }
    fprintf(builder->file, "</g>\n");

    return SYLVES_SUCCESS;
}

SylvesError sylves_svg_write_path_commands(FILE* file, const SylvesVector3* vertices, size_t vertex_count, SylvesMatrix4x4 transform, int close_path) {
    if (!vertices || vertex_count == 0) return SYLVES_ERROR_INVALID_ARGUMENT;

    int first = 1;
    for (size_t i = 0; i < vertex_count; i++) {
        SylvesVector3 v2;
        v2 = sylves_matrix4x4_multiply_point(&transform, vertices[i]);
        fprintf(file, "%c%f %f ", first ? 'M' : 'L', v2.x, v2.y);
        first = 0;
    }

    if (close_path) {
        fprintf(file, "Z");
    }

    return SYLVES_SUCCESS;
}

SylvesError sylves_export_grid_svg(const SylvesGrid* grid, const char* filename, const SylvesSvgOptions* options) {
    if (!grid || !filename || !options) return SYLVES_ERROR_INVALID_ARGUMENT;

    FILE* file = fopen(filename, "w");
    if (!file) return SYLVES_ERROR_IO;

    SylvesSvgBuilder* builder;
    SylvesError err = sylves_svg_builder_create(&builder, file);
    if (err != SYLVES_SUCCESS) {
        fclose(file);
        return err;
    }

    err = sylves_svg_begin(builder, options);
    if (err != SYLVES_SUCCESS) goto cleanup;

    // Get cells from grid or bounds
    SylvesCell cells[1024]; // Reasonable max for SVG export
    int cell_count = 0;
    
    // Get cells directly from the grid (works for finite bounded grids)
    cell_count = sylves_grid_get_cells(grid, cells, 1024);
    
    if (cell_count <= 0) {
        err = SYLVES_ERROR_UNBOUNDED;
        goto cleanup;
    }

    for (size_t i = 0; i < cell_count; i++) {
        const char* fill_color = options->get_cell_fill ? options->get_cell_fill(cells[i], options->user_data) : options->fill_color;
        err = sylves_svg_draw_cell(builder, grid, cells[i], fill_color);
        if (err != SYLVES_SUCCESS) goto cleanup;

        if (options->show_coordinates) {
            const char* text = NULL;
            if (options->get_cell_text) text = options->get_cell_text(cells[i], options->user_data);
            err = sylves_svg_draw_coordinate_label(builder, grid, cells[i], options->coordinate_dimensions, options->text_scale, text);
            if (err != SYLVES_SUCCESS) goto cleanup;
        }
    }

    err = sylves_svg_end(builder);

cleanup:
    sylves_svg_builder_destroy(builder);
    fclose(file);

    return err;
}

SylvesError sylves_export_grids_svg(
    const SylvesGrid** grids,
    size_t grid_count,
    const char* filename,
    const SylvesSvgOptions* options
) {
    if (!grids || grid_count == 0 || !filename || !options) return SYLVES_ERROR_INVALID_ARGUMENT;

    FILE* file = fopen(filename, "w");
    if (!file) return SYLVES_ERROR_IO;

    SylvesSvgBuilder* builder;
    SylvesError err = sylves_svg_builder_create(&builder, file);
    if (err != SYLVES_SUCCESS) {
        fclose(file);
        return err;
    }

    err = sylves_svg_begin(builder, options);
    if (err != SYLVES_SUCCESS) goto cleanup;

    for (size_t g = 0; g < grid_count; g++) {
        const SylvesGrid* grid = grids[g];
        if (!grid) continue;

        // Retrieve cells from bound if present; otherwise, attempt finite grid extraction
        SylvesCell cells[2048];
        int cell_count = 0;
        // Fetch cells directly from grid
        cell_count = sylves_grid_get_cells(grid, cells, 2048);
        if (cell_count <= 0) continue;

        for (int i = 0; i < cell_count; i++) {
            const char* fill_color = options->get_cell_fill ? options->get_cell_fill(cells[i], options->user_data) : options->fill_color;
            SylvesError derr = sylves_svg_draw_cell(builder, grid, cells[i], fill_color);
            if (derr != SYLVES_SUCCESS) { err = derr; goto cleanup; }

            if (options->show_coordinates) {
                const char* text = NULL;
                if (options->get_cell_text) text = options->get_cell_text(cells[i], options->user_data);
                derr = sylves_svg_draw_coordinate_label(builder, grid, cells[i], options->coordinate_dimensions, options->text_scale, text);
                if (derr != SYLVES_SUCCESS) { err = derr; goto cleanup; }
            }
        }
    }

    err = sylves_svg_end(builder);

cleanup:
    sylves_svg_builder_destroy(builder);
    fclose(file);
    return err;
}
