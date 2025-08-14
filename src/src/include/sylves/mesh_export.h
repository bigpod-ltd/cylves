#ifndef SYLVES_MESH_EXPORT_H
#define SYLVES_MESH_EXPORT_H

#include "sylves/types.h"
#include "sylves/mesh_data.h"
#include "sylves/grid.h"
#include "sylves/errors.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// Mesh export format
typedef enum {
    SYLVES_MESH_FORMAT_OBJ,
    SYLVES_MESH_FORMAT_PLY,
    SYLVES_MESH_FORMAT_STL,
    SYLVES_MESH_FORMAT_OFF
} SylvesMeshFormat;

// Mesh export options
typedef struct SylvesMeshExportOptions {
    // Format-specific options
    int include_normals;        // Include vertex normals
    int include_uvs;           // Include texture coordinates
    int include_colors;        // Include vertex colors (PLY)
    int binary_format;         // Use binary format (PLY, STL)
    
    // Material settings (OBJ)
    const char* material_name;
    const char* material_file;
    
    // Coordinate system transform
    SylvesMatrix4x4 transform;
    
    // Precision
    int float_precision;       // Number of decimal places
} SylvesMeshExportOptions;

// Initialize default export options
SylvesError sylves_mesh_export_options_init(SylvesMeshExportOptions* options);

// Export mesh data to file
SylvesError sylves_export_mesh_data(
    const SylvesMeshDataEx* mesh,
    const char* filename,
    SylvesMeshFormat format,
    const SylvesMeshExportOptions* options
);

// Export mesh data to stream
SylvesError sylves_export_mesh_data_to_stream(
    const SylvesMeshDataEx* mesh,
    FILE* file,
    SylvesMeshFormat format,
    const SylvesMeshExportOptions* options
);

// Export grid as mesh
SylvesError sylves_export_grid_mesh(
    const SylvesGrid* grid,
    const char* filename,
    SylvesMeshFormat format,
    const SylvesMeshExportOptions* options
);

// Format-specific export functions
SylvesError sylves_export_obj(
    const SylvesMeshDataEx* mesh,
    FILE* file,
    const SylvesMeshExportOptions* options
);

SylvesError sylves_export_ply(
    const SylvesMeshDataEx* mesh,
    FILE* file,
    const SylvesMeshExportOptions* options
);

SylvesError sylves_export_stl(
    const SylvesMeshDataEx* mesh,
    FILE* file,
    const SylvesMeshExportOptions* options
);

SylvesError sylves_export_off(
    const SylvesMeshDataEx* mesh,
    FILE* file,
    const SylvesMeshExportOptions* options
);

// Helper to write material file for OBJ
SylvesError sylves_export_obj_material(
    const char* filename,
    const char* material_name,
    const SylvesVector3* diffuse_color,
    const SylvesVector3* specular_color,
    float shininess
);

#ifdef __cplusplus
}
#endif

#endif // SYLVES_MESH_EXPORT_H
