# Cylves - C Grid & Tessellation Library

[![CI](https://github.com/yourusername/cylves/actions/workflows/ci.yml/badge.svg)](https://github.com/yourusername/cylves/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C99](https://img.shields.io/badge/C-99-blue.svg)](https://en.wikipedia.org/wiki/C99)

Cylves is a comprehensive C99 port of the [Sylves](https://github.com/BorisTheBrave/sylves) geometry library, providing a unified interface for working with various grid types, meshes, and geometric algorithms.

## Features

### Current Implementation (~5% Complete)
- ✅ Basic square grid with bounded/unbounded variants
- ✅ Initial CI/CD pipeline
- ✅ Coding standards and documentation framework

### Planned Features (See [GAP_ANALYSIS.md](GAP_ANALYSIS.md))
- 🚧 Complete mathematical type system (vectors, matrices, quaternions)
- 🚧 Multiple grid types (hexagonal, triangular, cubic, Voronoi, etc.)
- 🚧 Grid modifiers (transform, mask, wrap, etc.)
- 🚧 Pathfinding algorithms (A*, Dijkstra, BFS)
- 🚧 Mesh operations and Conway operators
- 🚧 Computational geometry (Delaunay, Voronoi)
- 🚧 Export to SVG and 3D mesh formats

## Quick Start

### Prerequisites
- C99-compliant compiler (GCC, Clang, or MSVC)
- CMake 3.14 or higher
- Standard C math library

### Building

```bash
# Clone the repository
git clone https://github.com/yourusername/cylves.git
cd cylves

# Build the library
mkdir build
cd build
cmake ../reference/sylves-c
make

# Run tests
ctest --output-on-failure
```

### Basic Usage

```c
#include <sylves/sylves.h>
#include <stdio.h>

int main() {
    // Create a 10x10 bounded square grid with cell size 1.0
    SylvesGrid* grid = sylves_square_grid_create_bounded(
        1.0,    // cell size
        0, 0,   // min x, y
        9, 9    // max x, y
    );
    
    // Get the center of a cell
    SylvesCell cell = sylves_cell_create_2d(5, 5);
    SylvesVector3 center = sylves_grid_get_cell_center(grid, cell);
    printf("Cell center: (%.2f, %.2f, %.2f)\n", 
           center.x, center.y, center.z);
    
    // Navigate to neighboring cells
    SylvesCell neighbor;
    if (sylves_grid_try_move(grid, cell, SYLVES_SQUARE_DIR_RIGHT, 
                             &neighbor, NULL, NULL)) {
        printf("Right neighbor: (%d, %d)\n", neighbor.x, neighbor.y);
    }
    
    // Clean up
    sylves_grid_destroy(grid);
    return 0;
}
```

## Project Structure

```
cylves/
├── .github/workflows/     # CI/CD pipelines
├── .kiro/specs/           # Original specifications
├── reference/sylves-c/    # C library implementation
│   ├── src/              
│   │   ├── include/sylves/  # Public headers
│   │   ├── internal/        # Internal headers
│   │   └── *.c             # Implementation files
│   ├── tests/              # Unit tests
│   ├── examples/           # Example programs
│   └── CMakeLists.txt      # Build configuration
├── CODING_STANDARDS.md     # Coding guidelines
├── GAP_ANALYSIS.md        # Feature gap analysis & backlog
└── README.md              # This file
```

## Development

### Coding Standards

All contributions must follow the [Coding Standards](CODING_STANDARDS.md). Key points:
- C99 compliant code
- 4-space indentation
- Clear naming conventions
- Comprehensive documentation
- Memory safety first

### Building with Different Configurations

```bash
# Debug build with assertions
cmake -DCMAKE_BUILD_TYPE=Debug ../reference/sylves-c

# Release build with optimizations
cmake -DCMAKE_BUILD_TYPE=Release ../reference/sylves-c

# Build with specific compiler
CC=clang cmake ../reference/sylves-c
```

### Running Tests

```bash
# Run all tests
ctest

# Run with verbose output
ctest -V

# Run specific test
ctest -R test_square_grid

# Run with memory checking (Linux)
ctest -T memcheck
```

### Code Formatting

The project uses clang-format for consistent code style:

```bash
# Format all source files
find reference/sylves-c -name "*.c" -o -name "*.h" | xargs clang-format -i

# Check formatting without modifying
find reference/sylves-c -name "*.c" -o -name "*.h" | xargs clang-format --dry-run --Werror
```

### Documentation

API documentation is generated using Doxygen:

```bash
cd reference/sylves-c
doxygen Doxyfile
# Open docs/html/index.html in a browser
```

## Roadmap

See [GAP_ANALYSIS.md](GAP_ANALYSIS.md) for detailed implementation status and roadmap.

### Timeline Overview
- **Phase 1 (Current - 2 months)**: Core infrastructure, mathematical types, testing framework
- **Phase 2 (2-4 months)**: Basic grid implementations (hex, triangle, cube)
- **Phase 3 (4-6 months)**: Advanced features (modifiers, pathfinding, mesh operations)
- **Phase 4 (6-8 months)**: Polish, optimization, production readiness

## Contributing

We welcome contributions! Please:

1. Check the [GAP_ANALYSIS.md](GAP_ANALYSIS.md) for open tasks
2. Follow the [CODING_STANDARDS.md](CODING_STANDARDS.md)
3. Write tests for new features
4. Update documentation
5. Submit a pull request

### Development Workflow

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## Performance

The library is designed for performance-critical applications:
- Zero-allocation pathfinding options
- Cache-friendly data structures
- Optional memory pooling
- SIMD-ready vector operations

## Platform Support

- **Operating Systems**: Linux, macOS, Windows
- **Compilers**: GCC 4.9+, Clang 3.4+, MSVC 2015+
- **Architectures**: x86, x86_64, ARM, ARM64

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- Original [Sylves](https://github.com/BorisTheBrave/sylves) library by Boris The Brave
- Inspired by the need for a performant C implementation for embedded systems and game engines

## Resources

- [API Documentation](https://yourdomain.com/cylves/docs) (when deployed)
- [Original Sylves Documentation](https://www.boristhebrave.com/docs/sylves/1/)
- [Gap Analysis & Backlog](GAP_ANALYSIS.md)
- [Coding Standards](CODING_STANDARDS.md)

## Contact

- Issue Tracker: [GitHub Issues](https://github.com/yourusername/cylves/issues)
- Discussions: [GitHub Discussions](https://github.com/yourusername/cylves/discussions)

---

*Note: This is a work in progress. The library is currently in early development (~5% complete). See [GAP_ANALYSIS.md](GAP_ANALYSIS.md) for implementation status.*
