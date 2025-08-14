# Sylves C Library Port - Design Document

## Overview

This document outlines the architectural design for the complete C99 port of the Sylves geometry library. The design maintains the core architectural patterns of the original C# implementation while adapting them to C's procedural paradigm and explicit memory management model.

The design follows a modular architecture with clear separation of concerns, using function pointers for polymorphism and consistent naming conventions throughout. The library is structured as a static library with a comprehensive public API that mirrors the functionality of the original C# implementation.

## Architecture

### High-Level Architecture

The library is organized into several key modules:

```
sylves/
├── Core Types (Vector, Matrix, Cell, etc.)
├── Grid System (IGrid interface via vtables)
├── Cell Types (ICellType interface via vtables) 
├── Grid Implementations (Square, Hex, Triangle, Cube, etc.)
├── Grid Modifiers (Transform, Mask, Wrap, etc.)
├── Algorithms (Pathfinding, Spanning Trees, etc.)
├── Mesh Operations (Generation, Conway operators, etc.)
├── Geometry (Voronoi, Delaunay, Deformation)
└── Utilities (Export, Math utilities, etc.)
```

### Memory Management Strategy

- **Ownership Model**: Clear ownership semantics with create/destroy function pairs
- **Reference Counting**: Not used - explicit ownership transfer and cleanup
- **Error Handling**: All functions return error codes or use boolean success indicators
- **Resource Management**: RAII-style patterns where possible using cleanup functions

### Polymorphism via VTables

Since C lacks native polymorphism, we implement it using function pointer tables (vtables):

```c
typedef struct SylvesGridVTable {
    void (*destroy)(SylvesGrid* grid);
    bool (*is_2d)(const SylvesGrid* grid);
    bool (*try_move)(const SylvesGrid* grid, SylvesCell cell, SylvesCellDir dir, /*...*/);
    // ... other virtual methods
} SylvesGridVTable;

struct SylvesGrid {
    const SylvesGridVTable* vtable;
    SylvesGridType type;
    SylvesBound* bound;
    void* data;  // Grid-specific data
};
```

## Components and Interfaces

### Core Type System

#### Basic Types
- `SylvesCell`: 3D integer coordinates with equality and hashing
- `SylvesVector2/3`: Double-precision vectors with full mathematical operations
- `SylvesVector2Int/3Int`: Integer vectors for discrete operations
- `SylvesMatrix4x4`: Column-major 4x4 matrices with transformation operations
- `SylvesQuaternion`: Unit quaternions for rotations
- `SylvesAabb`: Axis-aligned bounding boxes with intersection operations

#### Geometric Types
- `SylvesTRS`: Transform with separate position, rotation, scale
- `SylvesCellDir/Corner/Rotation`: Type-safe cell-relative coordinates
- `SylvesConnection`: Describes spatial relationship between adjacent cells
- `SylvesDeformation`: Maps between coordinate spaces

#### Collection Types
- `SylvesMeshData`: Vertex/index data with optional normals and UVs
- `SylvesRaycastInfo`: Ray intersection results
- `SylvesPathOptions`: Pathfinding configuration

### Grid Interface (IGrid equivalent)

The grid interface is implemented via vtables with the following key methods:

#### Property Queries
```c
bool (*is_2d)(const SylvesGrid* grid);
bool (*is_3d)(const SylvesGrid* grid);
bool (*is_planar)(const SylvesGrid* grid);
bool (*is_repeating)(const SylvesGrid* grid);
bool (*is_orientable)(const SylvesGrid* grid);
bool (*is_finite)(const SylvesGrid* grid);
int (*get_coordinate_dimension)(const SylvesGrid* grid);
```

#### Cell Operations
```c
bool (*is_cell_in_grid)(const SylvesGrid* grid, SylvesCell cell);
const SylvesCellType* (*get_cell_type)(const SylvesGrid* grid, SylvesCell cell);
int (*get_cells)(const SylvesGrid* grid, SylvesCell* cells, size_t max_cells);
```

#### Topology
```c
bool (*try_move)(const SylvesGrid* grid, SylvesCell cell, SylvesCellDir dir,
                 SylvesCell* dest, SylvesCellDir* inverse_dir, SylvesConnection* connection);
int (*get_cell_dirs)(const SylvesGrid* grid, SylvesCell cell, SylvesCellDir* dirs, size_t max_dirs);
int (*find_basic_path)(const SylvesGrid* grid, SylvesCell start, SylvesCell dest, /*...*/);
```

#### Spatial Queries
```c
SylvesVector3 (*get_cell_center)(const SylvesGrid* grid, SylvesCell cell);
SylvesVector3 (*get_cell_corner_pos)(const SylvesGrid* grid, SylvesCell cell, SylvesCellCorner corner);
bool (*find_cell)(const SylvesGrid* grid, SylvesVector3 position, SylvesCell* cell);
int (*raycast)(const SylvesGrid* grid, SylvesVector3 origin, SylvesVector3 direction, /*...*/);
```

### Cell Type Interface (ICellType equivalent)

Cell types define the local structure and symmetries of individual cells:

```c
typedef struct SylvesCellTypeVTable {
    int (*get_cell_dirs)(const SylvesCellType* cell_type, SylvesCellDir* dirs, size_t max_dirs);
    SylvesCellDir (*invert_dir)(const SylvesCellType* cell_type, SylvesCellDir dir);
    int (*get_cell_corners)(const SylvesCellType* cell_type, SylvesCellCorner* corners, size_t max_corners);
    int (*get_rotations)(const SylvesCellType* cell_type, SylvesCellRotation* rotations, 
                        size_t max_rotations, bool include_reflections);
    SylvesCellRotation (*multiply_rotations)(const SylvesCellType* cell_type, 
                                           SylvesCellRotation a, SylvesCellRotation b);
    SylvesCellDir (*rotate_dir)(const SylvesCellType* cell_type, SylvesCellDir dir, 
                               SylvesCellRotation rotation);
    SylvesMatrix4x4 (*get_rotation_matrix)(const SylvesCellType* cell_type, SylvesCellRotation rotation);
    SylvesVector3 (*get_corner_position)(const SylvesCellType* cell_type, SylvesCellCorner corner);
} SylvesCellTypeVTable;
```

### Bounds System

Bounds are implemented as an abstract interface with concrete implementations:

```c
typedef struct SylvesBoundVTable {
    void (*destroy)(SylvesBound* bound);
    bool (*contains)(const SylvesBound* bound, SylvesCell cell);
    int (*get_cells)(const SylvesBound* bound, SylvesCell* cells, size_t max_cells);
    SylvesBound* (*intersect)(const SylvesBound* a, const SylvesBound* b);
    SylvesBound* (*union_bounds)(const SylvesBound* a, const SylvesBound* b);
    SylvesAabb (*get_aabb)(const SylvesBound* bound);
} SylvesBoundVTable;
```

Concrete bound types:
- `SylvesSquareBound`: Rectangular regions for square grids
- `SylvesHexBound`: Hexagonal and parallelogram regions for hex grids
- `SylvesCubeBound`: 3D rectangular regions for cube grids
- `SylvesMaskBound`: Arbitrary cell sets

## Data Models

### Grid Implementations

#### Square Grid
```c
typedef struct {
    double cell_size_x, cell_size_y;  // Non-uniform cell sizes
    SylvesSquareBound* bound;          // Optional bounds
} SquareGridData;
```

#### Hexagonal Grid
```c
typedef enum {
    SYLVES_HEX_ORIENTATION_FLAT_TOP,
    SYLVES_HEX_ORIENTATION_POINTY_TOP
} SylvesHexOrientation;

typedef struct {
    double cell_size;
    SylvesHexOrientation orientation;
    SylvesHexBound* bound;
} HexGridData;
```

#### Triangle Grid
```c
typedef struct {
    double cell_size;
    SylvesTriangleBound* bound;
} TriangleGridData;
```

#### Cube Grid
```c
typedef struct {
    double cell_size_x, cell_size_y, cell_size_z;
    SylvesCubeBound* bound;
} CubeGridData;
```

#### Mesh Grid
```c
typedef struct {
    SylvesMeshData* mesh;
    SylvesVector3* face_centers;
    SylvesVector3* face_normals;
    int* adjacency;  // Face adjacency information
    size_t face_count;
} MeshGridData;
```

### Grid Modifiers

Grid modifiers wrap existing grids to provide transformed behavior:

#### Transform Modifier
```c
typedef struct {
    SylvesGrid* base_grid;
    SylvesMatrix4x4 transform;
    SylvesMatrix4x4 inverse_transform;
} TransformModifierData;
```

#### Mask Modifier
```c
typedef struct {
    SylvesGrid* base_grid;
    SylvesBound* mask;
} MaskModifierData;
```

#### Wrap Modifier
```c
typedef struct {
    SylvesGrid* base_grid;
    SylvesVector3Int wrap_size;
} WrapModifierData;
```

### Algorithm Data Structures

#### Pathfinding
```c
typedef struct {
    SylvesCell* path;
    SylvesCellDir* directions;
    double* distances;
    size_t length;
    double total_distance;
} SylvesPath;

typedef struct {
    double (*heuristic)(SylvesCell from, SylvesCell to, void* user_data);
    double (*edge_weight)(SylvesCell from, SylvesCell to, SylvesCellDir dir, void* user_data);
    bool (*is_passable)(SylvesCell cell, void* user_data);
    void* user_data;
} SylvesPathfindingCallbacks;
```

#### Mesh Operations
```c
typedef struct {
    SylvesVector3* vertices;
    int* faces;  // Variable-length faces with count prefix
    size_t vertex_count;
    size_t face_count;
    int* vertex_face_adjacency;
    int* edge_adjacency;
} SylvesPolyhedralMesh;
```

## Error Handling

### Error Code System

All functions use a consistent error reporting system:

```c
typedef enum {
    SYLVES_SUCCESS = 0,
    SYLVES_ERROR_NULL_POINTER = -1,
    SYLVES_ERROR_OUT_OF_BOUNDS = -2,
    SYLVES_ERROR_OUT_OF_MEMORY = -3,
    SYLVES_ERROR_INVALID_ARGUMENT = -4,
    SYLVES_ERROR_NOT_IMPLEMENTED = -5,
    SYLVES_ERROR_CELL_NOT_IN_GRID = -6,
    SYLVES_ERROR_NOT_SUPPORTED = -7,
    SYLVES_ERROR_PATH_NOT_FOUND = -8,
    SYLVES_ERROR_MATH = -9,
    SYLVES_ERROR_BUFFER_TOO_SMALL = -10,
    SYLVES_ERROR_INFINITE_GRID = -11,
    SYLVES_ERROR_INVALID_STATE = -12
} SylvesError;
```

### Error Handling Patterns

1. **Return Codes**: Functions return `SylvesError` or negative values for errors
2. **Boolean Success**: Simple operations return `bool` for success/failure
3. **Output Parameters**: Complex results use output parameters with error returns
4. **Null Checks**: All public functions validate input parameters
5. **Graceful Degradation**: Operations fail safely without corrupting state

## Testing Strategy

### Unit Testing Framework

- **Framework**: Custom lightweight testing framework suitable for C99
- **Coverage**: All public API functions with edge cases
- **Memory Testing**: Valgrind integration for leak detection
- **Cross-Platform**: Tests run on Linux, macOS, Windows

### Test Categories

#### Core Type Tests
- Vector/matrix mathematical operations
- Cell equality and hashing
- Transformation composition and decomposition
- Bounds intersection and union operations

#### Grid Implementation Tests
- All grid types with property validation
- Cell navigation and topology
- Spatial queries and position mapping
- Bounds handling and finite/infinite behavior

#### Algorithm Tests
- Pathfinding correctness and performance
- Mesh generation and manipulation
- Voronoi/Delaunay triangulation accuracy
- Geometric algorithm robustness

#### Integration Tests
- Grid modifier combinations
- Complex pathfinding scenarios
- Large-scale performance tests
- Memory usage validation

### Validation Against C# Implementation

- **Reference Tests**: Key algorithms validated against C# results
- **Numerical Precision**: Floating-point comparisons with appropriate tolerances
- **Behavioral Equivalence**: Same inputs produce equivalent outputs
- **Performance Benchmarks**: C implementation performance compared to C#

## Performance Considerations

### Memory Layout

- **Structure Packing**: Careful attention to cache-friendly data layout
- **Memory Pools**: Optional memory pool allocators for high-frequency operations
- **Lazy Evaluation**: Expensive computations deferred until needed
- **Reference Sharing**: Immutable data shared between instances where safe

### Algorithmic Efficiency

- **Spatial Indexing**: Efficient spatial queries using appropriate data structures
- **Path Caching**: Common paths cached for repeated queries
- **Incremental Updates**: Grid modifications update only affected regions
- **SIMD Opportunities**: Vector operations designed for potential SIMD optimization

### Scalability

- **Large Grids**: Algorithms designed to handle millions of cells
- **Streaming**: Support for processing grids larger than memory
- **Parallel Processing**: Thread-safe read operations where applicable
- **Memory Bounds**: Configurable limits to prevent excessive memory usage

## Platform Compatibility

### C99 Compliance

- **Standard Library Only**: No platform-specific dependencies
- **Portable Types**: Use of `stdint.h` and `stdbool.h` for consistency
- **Math Library**: Only standard `math.h` functions used
- **Compiler Compatibility**: Works with GCC, Clang, MSVC

### Build System

- **CMake**: Modern CMake (3.14+) with proper target exports
- **Package Config**: CMake package configuration for easy integration
- **Static/Dynamic**: Support for both static and dynamic linking
- **Cross-Compilation**: Support for embedded and mobile targets

### Integration Support

- **C++ Compatibility**: Headers work in C++ with `extern "C"`
- **Binding Friendly**: API designed for easy language binding generation
- **Documentation**: Doxygen-compatible documentation for all public APIs
- **Examples**: Comprehensive examples for all major features

This design provides a solid foundation for implementing the complete Sylves C port while maintaining the flexibility and power of the original C# implementation.