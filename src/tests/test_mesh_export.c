#include "test_common.h"
#include "sylves/mesh_export.h"
#include "sylves/mesh_data.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static SylvesMeshDataEx* create_test_cube_mesh(void) {
    SylvesMeshDataEx* mesh = sylves_mesh_data_ex_create(8, 1);
    if (!mesh) return NULL;
    
    // Define cube vertices
    float verts[][3] = {
        {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},  // Bottom
        {-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}       // Top
    };
    
    for (int i = 0; i < 8; i++) {
        mesh->vertices[i] = (SylvesVector3){verts[i][0], verts[i][1], verts[i][2]};
    }
    
    // Add normals
    mesh->normals = (SylvesVector3*)malloc(8 * sizeof(SylvesVector3));
    for (int i = 0; i < 8; i++) {
        mesh->normals[i] = sylves_vector3_normalize(mesh->vertices[i]);
    }
    
    // Add UVs
    mesh->uvs = (SylvesVector2*)malloc(8 * sizeof(SylvesVector2));
    for (int i = 0; i < 8; i++) {
        mesh->uvs[i] = (SylvesVector2){(i % 4) / 3.0f, (i / 4) * 1.0f};
    }
    
    // Create triangles submesh
    mesh->submeshes[0].topology = SYLVES_MESH_TOPOLOGY_TRIANGLES;
    mesh->submeshes[0].index_count = 36;
    mesh->submeshes[0].indices = (int*)malloc(36 * sizeof(int));
    
    // Define cube faces (as triangles)
    int faces[][6] = {
        {0, 1, 2, 0, 2, 3}, // Bottom
        {4, 7, 6, 4, 6, 5}, // Top
        {0, 4, 5, 0, 5, 1}, // Front
        {2, 6, 7, 2, 7, 3}, // Back
        {0, 3, 7, 0, 7, 4}, // Left
        {1, 5, 6, 1, 6, 2}  // Right
    };
    
    int idx = 0;
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 6; j++) {
            mesh->submeshes[0].indices[idx++] = faces[i][j];
        }
    }
    
    return mesh;
}

static void test_mesh_export_options_init(void) {
    SylvesMeshExportOptions options;
    SylvesError err = sylves_mesh_export_options_init(&options);
    
    ASSERT_EQ(err, SYLVES_SUCCESS, "Options init should succeed");
    ASSERT_EQ(options.include_normals, 1, "Default include normals");
    ASSERT_EQ(options.include_uvs, 1, "Default include UVs");
    ASSERT_EQ(options.include_colors, 0, "Default include colors");
    ASSERT_EQ(options.binary_format, 0, "Default binary format");
    ASSERT_STR_EQ(options.material_name, "default", "Default material name");
    ASSERT_NULL(options.material_file, "Default material file");
    ASSERT_EQ(options.float_precision, 6, "Default float precision");
}

static void test_export_obj(void) {
    SylvesMeshDataEx* mesh = create_test_cube_mesh();
    ASSERT_NOT_NULL(mesh, "Create test mesh");
    
    SylvesMeshExportOptions options;
    sylves_mesh_export_options_init(&options);
    
    // Export to temp file
    char temp_filename[] = "/tmp/test_cube_XXXXXX.obj";
    int fd = mkstemps(temp_filename, 4);
    ASSERT_TRUE(fd >= 0, "Create temp file");
    close(fd);
    
    SylvesError err = sylves_export_mesh_data(mesh, temp_filename, SYLVES_MESH_FORMAT_OBJ, &options);
    ASSERT_EQ(err, SYLVES_SUCCESS, "Export should succeed");
    
    // Verify file content
    FILE* file = fopen(temp_filename, "r");
    ASSERT_NOT_NULL(file, "Open exported file");
    
    char line[256];
    int vertex_count = 0, normal_count = 0, uv_count = 0, face_count = 0;
    
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "v ", 2) == 0) vertex_count++;
        else if (strncmp(line, "vn ", 3) == 0) normal_count++;
        else if (strncmp(line, "vt ", 3) == 0) uv_count++;
        else if (strncmp(line, "f ", 2) == 0) face_count++;
    }
    
    ASSERT_EQ(vertex_count, 8, "Should have 8 vertices");
    ASSERT_EQ(normal_count, 8, "Should have 8 normals");
    ASSERT_EQ(uv_count, 8, "Should have 8 UVs");
    ASSERT_EQ(face_count, 12, "Should have 12 faces (2 per cube face)");
    
    fclose(file);
    remove(temp_filename);
    sylves_mesh_data_ex_destroy(mesh);
}

static void test_export_ply(void) {
    SylvesMeshDataEx* mesh = create_test_cube_mesh();
    ASSERT_NOT_NULL(mesh, "Create test mesh");
    
    SylvesMeshExportOptions options;
    sylves_mesh_export_options_init(&options);
    
    // Export to temp file
    char temp_filename[] = "/tmp/test_cube_XXXXXX.ply";
    int fd = mkstemps(temp_filename, 4);
    ASSERT_TRUE(fd >= 0, "Create temp file");
    close(fd);
    
    SylvesError err = sylves_export_mesh_data(mesh, temp_filename, SYLVES_MESH_FORMAT_PLY, &options);
    ASSERT_EQ(err, SYLVES_SUCCESS, "Export should succeed");
    
    // Verify file content
    FILE* file = fopen(temp_filename, "r");
    ASSERT_NOT_NULL(file, "Open exported file");
    
    char line[256];
    fgets(line, sizeof(line), file);
    ASSERT_STR_EQ(line, "ply\n", "Should start with ply header");
    
    fgets(line, sizeof(line), file);
    ASSERT_TRUE(strstr(line, "format ascii") != NULL, "Should be ASCII format");
    
    fclose(file);
    remove(temp_filename);
    sylves_mesh_data_ex_destroy(mesh);
}

static void test_export_stl(void) {
    SylvesMeshDataEx* mesh = create_test_cube_mesh();
    ASSERT_NOT_NULL(mesh, "Create test mesh");
    
    SylvesMeshExportOptions options;
    sylves_mesh_export_options_init(&options);
    
    // Export to temp file
    char temp_filename[] = "/tmp/test_cube_XXXXXX.stl";
    int fd = mkstemps(temp_filename, 4);
    ASSERT_TRUE(fd >= 0, "Create temp file");
    close(fd);
    
    SylvesError err = sylves_export_mesh_data(mesh, temp_filename, SYLVES_MESH_FORMAT_STL, &options);
    ASSERT_EQ(err, SYLVES_SUCCESS, "Export should succeed");
    
    // Verify file content
    FILE* file = fopen(temp_filename, "r");
    ASSERT_NOT_NULL(file, "Open exported file");
    
    char line[256];
    fgets(line, sizeof(line), file);
    ASSERT_TRUE(strstr(line, "solid") != NULL, "Should start with solid");
    
    // Count facets
    int facet_count = 0;
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "facet normal")) facet_count++;
    }
    
    ASSERT_EQ(facet_count, 12, "Should have 12 facets");
    
    fclose(file);
    remove(temp_filename);
    sylves_mesh_data_ex_destroy(mesh);
}

static void test_export_off(void) {
    SylvesMeshDataEx* mesh = create_test_cube_mesh();
    ASSERT_NOT_NULL(mesh, "Create test mesh");
    
    SylvesMeshExportOptions options;
    sylves_mesh_export_options_init(&options);
    
    // Export to temp file
    char temp_filename[] = "/tmp/test_cube_XXXXXX.off";
    int fd = mkstemps(temp_filename, 4);
    ASSERT_TRUE(fd >= 0, "Create temp file");
    close(fd);
    
    SylvesError err = sylves_export_mesh_data(mesh, temp_filename, SYLVES_MESH_FORMAT_OFF, &options);
    ASSERT_EQ(err, SYLVES_SUCCESS, "Export should succeed");
    
    // Verify file content
    FILE* file = fopen(temp_filename, "r");
    ASSERT_NOT_NULL(file, "Open exported file");
    
    char line[256];
    fgets(line, sizeof(line), file);
    ASSERT_STR_EQ(line, "OFF\n", "Should start with OFF header");
    
    // Read counts
    int v_count, f_count, e_count;
    fscanf(file, "%d %d %d", &v_count, &f_count, &e_count);
    ASSERT_EQ(v_count, 8, "Should have 8 vertices");
    ASSERT_EQ(f_count, 12, "Should have 12 faces");
    
    fclose(file);
    remove(temp_filename);
    sylves_mesh_data_ex_destroy(mesh);
}

static void test_export_obj_material(void) {
    char temp_filename[] = "/tmp/test_material_XXXXXX.mtl";
    int fd = mkstemps(temp_filename, 4);
    ASSERT_TRUE(fd >= 0, "Create temp file");
    close(fd);
    
    SylvesVector3 diffuse = {0.8f, 0.2f, 0.2f};
    SylvesVector3 specular = {1.0f, 1.0f, 1.0f};
    
    SylvesError err = sylves_export_obj_material(temp_filename, "red_plastic", &diffuse, &specular, 32.0f);
    ASSERT_EQ(err, SYLVES_SUCCESS, "Material export should succeed");
    
    // Verify content
    FILE* file = fopen(temp_filename, "r");
    ASSERT_NOT_NULL(file, "Open material file");
    
    char buffer[1024];
    size_t total_read = fread(buffer, 1, sizeof(buffer) - 1, file);
    buffer[total_read] = '\0';
    
    ASSERT_TRUE(strstr(buffer, "newmtl red_plastic") != NULL, "Should contain material name");
    ASSERT_TRUE(strstr(buffer, "Kd 0.800000 0.200000 0.200000") != NULL, "Should contain diffuse color");
    ASSERT_TRUE(strstr(buffer, "Ks 1.000000 1.000000 1.000000") != NULL, "Should contain specular color");
    ASSERT_TRUE(strstr(buffer, "Ns 32.0") != NULL, "Should contain shininess");
    
    fclose(file);
    remove(temp_filename);
}

static void test_export_with_transform(void) {
    SylvesMeshDataEx* mesh = create_test_cube_mesh();
    ASSERT_NOT_NULL(mesh, "Create test mesh");
    
    SylvesMeshExportOptions options;
    sylves_mesh_export_options_init(&options);
    
    // Apply scaling transform
    sylves_matrix4x4_scale(&options.transform, 2.0f, 2.0f, 2.0f);
    
    // Export to stream
    FILE* stream = tmpfile();
    ASSERT_NOT_NULL(stream, "Create temp stream");
    
    SylvesError err = sylves_export_mesh_data_to_stream(mesh, stream, SYLVES_MESH_FORMAT_OBJ, &options);
    ASSERT_EQ(err, SYLVES_SUCCESS, "Export to stream should succeed");
    
    // Check that vertices are scaled
    rewind(stream);
    char line[256];
    int found_scaled_vertex = 0;
    
    while (fgets(line, sizeof(line), stream)) {
        if (strncmp(line, "v ", 2) == 0) {
            float x, y, z;
            sscanf(line, "v %f %f %f", &x, &y, &z);
            if (fabsf(x) > 1.5f || fabsf(y) > 1.5f || fabsf(z) > 1.5f) {
                found_scaled_vertex = 1;
                break;
            }
        }
    }
    
    ASSERT_TRUE(found_scaled_vertex, "Should find scaled vertices");
    
    fclose(stream);
    sylves_mesh_data_ex_destroy(mesh);
}

int main(void) {
    TEST_RUN(test_mesh_export_options_init);
    TEST_RUN(test_export_obj);
    TEST_RUN(test_export_ply);
    TEST_RUN(test_export_stl);
    TEST_RUN(test_export_off);
    TEST_RUN(test_export_obj_material);
    TEST_RUN(test_export_with_transform);
    
    TEST_SUMMARY();
    return TEST_FAILED_COUNT > 0 ? 1 : 0;
}
