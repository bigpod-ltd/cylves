#include "test_common.h"
#include "sylves/svg_export.h"
#include "sylves/square_grid.h"
#include "sylves/hex_grid.h"
#include "sylves/bounds.h"
#include <string.h>
#include <stdio.h>

static void test_svg_options_init(void) {
    SylvesSvgOptions options;
    SylvesError err = sylves_svg_options_init(&options);
    ASSERT_EQ(err, SYLVES_SUCCESS, "SVG options init should succeed");
    
    ASSERT_FLOAT_EQ(options.min_x, -5.0f, "Default min_x");
    ASSERT_FLOAT_EQ(options.min_y, -5.0f, "Default min_y");
    ASSERT_FLOAT_EQ(options.max_x, 5.0f, "Default max_x");
    ASSERT_FLOAT_EQ(options.max_y, 5.0f, "Default max_y");
    ASSERT_FLOAT_EQ(options.stroke_width, 0.1f, "Default stroke width");
    ASSERT_STR_EQ(options.fill_color, "rgb(244, 244, 241)", "Default fill color");
    ASSERT_STR_EQ(options.stroke_color, "rgb(51, 51, 51)", "Default stroke color");
    ASSERT_EQ(options.show_coordinates, 0, "Default show coordinates");
    ASSERT_EQ(options.coordinate_dimensions, 3, "Default coordinate dimensions");
    ASSERT_FLOAT_EQ(options.text_scale, 1.0, "Default text scale");
}

static void test_svg_builder_create_destroy(void) {
    FILE* file = tmpfile();
    ASSERT_NOT_NULL(file, "tmpfile should succeed");
    
    SylvesSvgBuilder* builder = NULL;
    SylvesError err = sylves_svg_builder_create(&builder, file);
    ASSERT_EQ(err, SYLVES_SUCCESS, "SVG builder create should succeed");
    ASSERT_NOT_NULL(builder, "Builder should be allocated");
    
    sylves_svg_builder_destroy(builder);
    fclose(file);
}

static void test_svg_write_path_commands(void) {
    FILE* file = tmpfile();
    ASSERT_NOT_NULL(file, "tmpfile should succeed");
    
    SylvesVector3 vertices[] = {
        {0, 0, 0},
        {1, 0, 0},
        {1, 1, 0},
        {0, 1, 0}
    };
    
    SylvesMatrix4x4 identity;
    sylves_matrix4x4_identity(&identity);
    
    SylvesError err = sylves_svg_write_path_commands(file, vertices, 4, identity, 1);
    ASSERT_EQ(err, SYLVES_SUCCESS, "Write path commands should succeed");
    
    // Check file content
    rewind(file);
    char buffer[256];
    fgets(buffer, sizeof(buffer), file);
    ASSERT_TRUE(strstr(buffer, "M0.000000 0.000000") != NULL, "Should contain move command");
    ASSERT_TRUE(strstr(buffer, "Z") != NULL, "Should contain close path");
    
    fclose(file);
}

static void test_svg_export_square_grid(void) {
    // Create a square grid
    SylvesGrid* grid = sylves_square_grid_create(1.0f);
    ASSERT_NOT_NULL(grid, "Square grid creation should succeed");
    
    // Bind it
    SylvesBound* bound = sylves_bound_create_rectangle(-2, -2, 2, 2);
    grid = sylves_grid_bound_by(grid, bound);
    
    // Set up options
    SylvesSvgOptions options;
    sylves_svg_options_init(&options);
    options.min_x = -3;
    options.min_y = -3;
    options.max_x = 3;
    options.max_y = 3;
    options.show_coordinates = 1;
    
    // Export to temp file
    char temp_filename[] = "/tmp/test_square_grid_XXXXXX.svg";
    int fd = mkstemps(temp_filename, 4);
    ASSERT_TRUE(fd >= 0, "Create temp file");
    close(fd);
    
    SylvesError err = sylves_export_grid_svg(grid, temp_filename, &options);
    ASSERT_EQ(err, SYLVES_SUCCESS, "Export should succeed");
    
    // Verify file exists and has content
    FILE* file = fopen(temp_filename, "r");
    ASSERT_NOT_NULL(file, "Should be able to open exported file");
    
    char buffer[256];
    fgets(buffer, sizeof(buffer), file);
    ASSERT_TRUE(strstr(buffer, "<svg") != NULL, "Should contain SVG tag");
    
    fclose(file);
    remove(temp_filename);
    
    sylves_grid_destroy(grid);
}

static const char* custom_fill_callback(Cell cell, void* user_data) {
    (void)user_data;
    if ((cell.x + cell.y) % 2 == 0) {
        return "rgb(255, 0, 0)";
    }
    return "rgb(0, 255, 0)";
}

static void test_svg_export_with_callbacks(void) {
    // Create a hex grid
    SylvesGrid* grid = sylves_hex_grid_create(1.0f, HEX_FLAT_TOPPED);
    ASSERT_NOT_NULL(grid, "Hex grid creation should succeed");
    
    // Bind it
    SylvesBound* bound = sylves_bound_create_hex_parallelogram(-2, -2, 2, 2);
    grid = sylves_grid_bound_by(grid, bound);
    
    // Set up options with callback
    SylvesSvgOptions options;
    sylves_svg_options_init(&options);
    options.get_cell_fill = custom_fill_callback;
    options.user_data = NULL;
    
    // Export to temp file
    char temp_filename[] = "/tmp/test_hex_grid_XXXXXX.svg";
    int fd = mkstemps(temp_filename, 4);
    ASSERT_TRUE(fd >= 0, "Create temp file");
    close(fd);
    
    SylvesError err = sylves_export_grid_svg(grid, temp_filename, &options);
    ASSERT_EQ(err, SYLVES_SUCCESS, "Export should succeed");
    
    // Verify file contains custom colors
    FILE* file = fopen(temp_filename, "r");
    ASSERT_NOT_NULL(file, "Should be able to open exported file");
    
    char buffer[4096];
    size_t total_read = fread(buffer, 1, sizeof(buffer) - 1, file);
    buffer[total_read] = '\0';
    
    ASSERT_TRUE(strstr(buffer, "rgb(255, 0, 0)") != NULL, "Should contain red color");
    ASSERT_TRUE(strstr(buffer, "rgb(0, 255, 0)") != NULL, "Should contain green color");
    
    fclose(file);
    remove(temp_filename);
    
    sylves_grid_destroy(grid);
}

int main(void) {
    TEST_RUN(test_svg_options_init);
    TEST_RUN(test_svg_builder_create_destroy);
    TEST_RUN(test_svg_write_path_commands);
    TEST_RUN(test_svg_export_square_grid);
    TEST_RUN(test_svg_export_with_callbacks);
    
    TEST_SUMMARY();
    return TEST_FAILED_COUNT > 0 ? 1 : 0;
}
