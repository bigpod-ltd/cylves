# Sylves C Library

A pure C99 port of the Sylves geometry library, providing a comprehensive toolkit for working with various grid types, meshes, and geometric algorithms.

## Overview

Sylves is a powerful library for working with:
- Various grid types (square, hexagonal, cube, triangular, etc.)
- Mesh generation and manipulation
- Voronoi diagrams and Delaunay triangulation
- Pathfinding algorithms (A*, Dijkstra)
- Grid transformations and deformations
- Geometric utilities

This C port maintains full compatibility with the original C# implementation while providing a clean, portable C99 interface.

## Features

- **Pure C99**: No dependencies on C++ or platform-specific APIs
- **Static Library**: Builds as `libsylves.a` for easy integration
- **Memory Safe**: Explicit memory management with clear ownership semantics
- **Cross-Platform**: Works on Linux, macOS, Windows, and embedded systems
- **CMake Integration**: Modern CMake build system with package config

## Building

### Requirements

- C99-compliant compiler (GCC, Clang, MSVC)
- CMake 3.14 or higher
- Standard C library with math support

### Basic Build

```bash
mkdir build
cd build
cmake ..
make
```

### Build Options

```bash
# Release build with optimizations
cmake -DCMAKE_BUILD_TYPE=Release ..

# Debug build
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Disable tests
cmake -DBUILD_TESTS=OFF ..
```

### Installation

```bash
sudo make install
```

This installs:
- Static library to `/usr/local/lib/libsylves.a`
- Headers to `/usr/local/include/sylves/`
- CMake package config to `/usr/local/lib/cmake/sylves/`

## Integration

### Using CMake

In your project's `CMakeLists.txt`:

```cmake
find_package(sylves REQUIRED)
target_link_libraries(your_target PRIVATE sylves::sylves)
```

### Manual Integration

```c
#include <sylves/sylves.h>

// Link with libsylves.a and -lm
```

## Usage Example

```c
#include <sylves/sylves.h>
#include <stdio.h>

int main() {
    // Initialize a square grid
    SylvesGrid* grid = sylves_square_grid_create(1.0);
    
    // Create a cell at position (0, 0)
    SylvesCell cell = {0, 0, 0};
    
    // Get cell center
    SylvesVector3 center = sylves_grid_get_cell_center(grid, cell);
    printf("Cell center: (%f, %f, %f)\n", center.x, center.y, center.z);
    
    // Find neighbors
    SylvesCellDir dirs[4];
    size_t dir_count = sylves_grid_get_cell_dirs(grid, cell, dirs, 4);
    
    for (size_t i = 0; i < dir_count; i++) {
        SylvesCell neighbor;
        SylvesCellDir inverse_dir;
        SylvesConnection connection;
        
        if (sylves_grid_try_move(grid, cell, dirs[i], 
                                 &neighbor, &inverse_dir, &connection)) {
            printf("Neighbor at direction %d: (%d, %d, %d)\n", 
                   dirs[i], neighbor.x, neighbor.y, neighbor.z);
        }
    }
    
    // Clean up
    sylves_grid_destroy(grid);
    
    return 0;
}
```

## API Documentation

The library provides a comprehensive C API organized into modules:

### Core Types
- `SylvesCell` - Cell coordinates
- `SylvesVector3` - 3D vectors
- `SylvesMatrix4x4` - 4x4 transformation matrices
- `SylvesBounds` - Bounding boxes

### Grid System
- Grid creation and manipulation
- Cell navigation and neighbors
- Coordinate transformations
- Bounds and intersections

### Geometry
- Mesh generation
- Polygon operations
- Voronoi/Delaunay algorithms
- Shape deformations

### Algorithms
- Pathfinding (A*, Dijkstra)
- Minimum spanning trees
- Cell outlining
- Grid symmetries

## Memory Management

The library follows these conventions:

- Functions returning pointers allocate memory that must be freed by the caller
- Destruction functions are provided for all allocated structures (`sylves_*_destroy`)
- Output parameters use pointer arguments
- Arrays include size parameters or use null-termination

## Error Handling

All functions return error codes or use boolean success indicators:

```c
typedef enum {
    SYLVES_SUCCESS = 0,
    SYLVES_ERROR_NULL_POINTER = -1,
    SYLVES_ERROR_OUT_OF_BOUNDS = -2,
    SYLVES_ERROR_OUT_OF_MEMORY = -3,
    SYLVES_ERROR_INVALID_ARGUMENT = -4,
    SYLVES_ERROR_NOT_IMPLEMENTED = -5
} SylvesError;
```

## Testing

Run the test suite:

```bash
cd build
make test
# or
ctest --verbose
```

## License

This C port maintains compatibility with the original Sylves library license.
See LICENSE file for details.

## Contributing

Contributions are welcome! Please ensure:
- Code follows C99 standards
- Functions include Doxygen documentation
- Memory management is explicit and safe
- Tests are provided for new features

## Support

For issues, questions, or contributions, please refer to the project repository.
