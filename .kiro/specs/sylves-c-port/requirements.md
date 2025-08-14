# Sylves C Library Port - Requirements Document

## Introduction

This document outlines the requirements for creating a complete C99 port of the Sylves geometry library. Sylves is a comprehensive library for working with various grid types, meshes, and geometric algorithms originally written in C#. The C port must maintain full compatibility with the original implementation while providing a clean, portable C99 interface suitable for embedded systems, game engines, and other performance-critical applications.

The port must be a complete implementation, not a subset, covering all grid types, algorithms, and functionality present in the original C# codebase. The existing foundation in `reference/sylves-c/` provides basic infrastructure for square grids, but needs to be expanded to cover the entire library.

## Requirements

### Requirement 1: Core Infrastructure and Type System

**User Story:** As a C developer, I want a complete type system that mirrors the C# implementation, so that I can work with all grid types and geometric primitives in a type-safe manner.

#### Acceptance Criteria

1. WHEN the library is included THEN all core types (Cell, Vector3, Matrix4x4, TRS, etc.) SHALL be available with complete functionality
2. WHEN working with cells THEN the Cell type SHALL support 2D and 3D coordinates with proper equality and hashing
3. WHEN working with vectors THEN Vector2, Vector3, Vector2Int, Vector3Int SHALL be available with all mathematical operations
4. WHEN working with matrices THEN Matrix4x4 SHALL support all transformation operations including TRS decomposition
5. WHEN working with rotations THEN Quaternion type SHALL be available with all rotation operations
6. WHEN working with bounds THEN Aabb type SHALL support intersection, union, and containment operations
7. WHEN working with cell directions THEN CellDir, CellCorner, CellRotation SHALL be properly typed and validated
8. WHEN working with connections THEN Connection type SHALL properly handle rotation and mirroring between cells

### Requirement 2: Grid Interface and Base Implementation

**User Story:** As a library user, I want a unified grid interface that works consistently across all grid types, so that I can write generic algorithms that work with any grid.

#### Acceptance Criteria

1. WHEN creating any grid type THEN it SHALL implement the complete IGrid interface functionality
2. WHEN querying grid properties THEN all boolean properties (Is2d, Is3d, IsPlanar, IsRepeating, IsOrientable, IsFinite, IsSingleCellType) SHALL be correctly implemented
3. WHEN working with cells THEN GetCells, IsCellInGrid, GetCellType SHALL work correctly for all grid types
4. WHEN navigating grids THEN TryMove, GetCellDirs, GetCellCorners, FindBasicPath SHALL work for all grid types
5. WHEN querying positions THEN GetCellCenter, GetCellCorner, GetTRS SHALL return correct spatial information
6. WHEN working with shapes THEN GetPolygon (2D), GetTriangleMesh (3D), GetMeshData SHALL provide correct geometry
7. WHEN querying space THEN FindCell, GetCellsIntersectsApprox, Raycast SHALL work efficiently
8. WHEN working with bounds THEN GetBound, BoundBy, GetCellsInBounds SHALL handle bounded and unbounded grids
9. WHEN working with indices THEN GetIndex, GetCellByIndex, IndexCount SHALL provide efficient cell indexing

### Requirement 3: Square Grid Implementation

**User Story:** As a developer working with rectangular layouts, I want a complete square grid implementation with all features, so that I can use it for tile-based games and regular grids.

#### Acceptance Criteria

1. WHEN creating a square grid THEN it SHALL support both bounded and unbounded variants
2. WHEN working with square cells THEN all 4 directions (Right, Up, Left, Down) SHALL be properly supported
3. WHEN working with square corners THEN all 4 corners SHALL be correctly positioned
4. WHEN querying square grid properties THEN it SHALL report as 2D, planar, repeating, orientable
5. WHEN working with square rotations THEN all 4 rotations and 4 reflections SHALL be supported
6. WHEN finding cells THEN position-to-cell lookup SHALL work correctly with configurable cell sizes
7. WHEN getting polygons THEN square cells SHALL return correct 4-vertex polygons
8. WHEN working with bounds THEN SquareBound SHALL support rectangular regions efficiently

### Requirement 4: Hexagonal Grid Implementation

**User Story:** As a developer working with hexagonal layouts, I want complete hexagonal grid support with both flat-top and pointy-top orientations, so that I can create hex-based games and simulations.

#### Acceptance Criteria

1. WHEN creating hex grids THEN both flat-top and pointy-top orientations SHALL be supported
2. WHEN working with hex cells THEN all 6 directions SHALL be properly implemented with correct neighbor relationships
3. WHEN working with hex coordinates THEN both axial and cube coordinate systems SHALL be supported
4. WHEN working with hex rotations THEN all 6 rotations and 6 reflections SHALL be correctly implemented
5. WHEN querying hex properties THEN it SHALL report as 2D, planar, repeating, orientable
6. WHEN finding cells THEN position-to-cell lookup SHALL work correctly for both orientations
7. WHEN getting polygons THEN hex cells SHALL return correct 6-vertex polygons
8. WHEN working with bounds THEN HexBound SHALL support hexagonal and parallelogram regions

### Requirement 5: Triangle Grid Implementation

**User Story:** As a developer working with triangular tessellations, I want complete triangle grid support, so that I can work with triangular meshes and dual relationships.

#### Acceptance Criteria

1. WHEN creating triangle grids THEN both up-pointing and down-pointing triangles SHALL be supported
2. WHEN working with triangle cells THEN all 3 directions SHALL be properly implemented
3. WHEN working with triangle coordinates THEN the coordinate system SHALL correctly handle alternating orientations
4. WHEN working with triangle rotations THEN all 3 rotations and 3 reflections SHALL be supported
5. WHEN querying triangle properties THEN it SHALL report as 2D, planar, repeating, orientable
6. WHEN finding cells THEN position-to-cell lookup SHALL correctly identify triangle containing point
7. WHEN getting polygons THEN triangle cells SHALL return correct 3-vertex triangles
8. WHEN working with bounds THEN TriangleBound SHALL support triangular regions

### Requirement 6: Cube Grid Implementation

**User Story:** As a developer working with 3D voxel grids, I want complete cube grid support, so that I can create 3D games and simulations with cubic cells.

#### Acceptance Criteria

1. WHEN creating cube grids THEN it SHALL support 3D cubic tessellation
2. WHEN working with cube cells THEN all 6 face directions SHALL be properly implemented
3. WHEN working with cube coordinates THEN 3D integer coordinates SHALL work correctly
4. WHEN working with cube rotations THEN all 24 rotations SHALL be supported (no reflections for orientable grid)
5. WHEN querying cube properties THEN it SHALL report as 3D, not planar, repeating, orientable
6. WHEN finding cells THEN 3D position-to-cell lookup SHALL work correctly
7. WHEN getting mesh data THEN cube cells SHALL return correct 6-face cube meshes
8. WHEN working with bounds THEN CubeBound SHALL support 3D rectangular regions

### Requirement 7: Advanced Grid Types

**User Story:** As a developer working with complex tessellations, I want support for advanced grid types including mesh grids, substitution tilings, and Voronoi diagrams, so that I can create sophisticated geometric applications.

#### Acceptance Criteria

1. WHEN creating mesh grids THEN arbitrary triangle/quad meshes SHALL be supported as grids
2. WHEN working with Voronoi grids THEN point-based Voronoi tessellation SHALL be supported
3. WHEN working with substitution tilings THEN Penrose, Ammann-Beenker, and other aperiodic tilings SHALL be supported
4. WHEN working with periodic planar grids THEN Cairo, Rhombille, TriHex, and other Archimedean tilings SHALL be supported
5. WHEN working with prism grids THEN 2D grids extruded to 3D SHALL be supported (HexPrism, TrianglePrism)
6. WHEN working with jittered grids THEN perturbed regular grids SHALL be supported
7. WHEN working with wrapped grids THEN toroidal and other topological variants SHALL be supported

### Requirement 8: Grid Modifiers

**User Story:** As a developer, I want to apply transformations and constraints to existing grids, so that I can create bounded, transformed, and modified versions without reimplementing grid logic.

#### Acceptance Criteria

1. WHEN applying bounds THEN MaskModifier SHALL restrict grids to arbitrary cell sets
2. WHEN applying transforms THEN TransformModifier SHALL apply 3D transformations to grids
3. WHEN applying bijections THEN BijectModifier SHALL remap cell coordinates
4. WHEN applying wrapping THEN WrapModifier SHALL create toroidal topologies
5. WHEN applying relaxation THEN RelaxModifier SHALL smooth grid vertex positions
6. WHEN applying nesting THEN NestedModifier SHALL create hierarchical grids
7. WHEN applying raveling THEN RavelModifier SHALL flatten multi-dimensional grids
8. WHEN applying planar prisms THEN PlanarPrismModifier SHALL extrude 2D grids to 3D

### Requirement 9: Cell Types and Rotations

**User Story:** As a developer, I want complete cell type support with rotation and reflection operations, so that I can work with cell symmetries and transformations.

#### Acceptance Criteria

1. WHEN working with any cell type THEN GetCellDirs, GetCellCorners SHALL return all valid directions and corners
2. WHEN working with rotations THEN GetRotations SHALL return all valid rotations and reflections
3. WHEN composing rotations THEN Multiply, Invert SHALL work correctly for all cell types
4. WHEN rotating directions THEN Rotate SHALL correctly transform directions and corners
5. WHEN working with matrices THEN GetMatrix SHALL return correct transformation matrices
6. WHEN working with canonical shapes THEN GetCornerPosition SHALL return correct positions
7. WHEN working with connections THEN TryGetRotation SHALL correctly determine rotations between cells

### Requirement 10: Pathfinding and Algorithms

**User Story:** As a developer, I want comprehensive pathfinding and geometric algorithms, so that I can implement navigation, spanning trees, and other graph algorithms on grids.

#### Acceptance Criteria

1. WHEN finding paths THEN A* pathfinding SHALL work on all grid types with customizable heuristics
2. WHEN finding paths THEN Dijkstra pathfinding SHALL work with custom edge weights
3. WHEN finding paths THEN basic breadth-first search SHALL be available for simple connectivity
4. WHEN finding spanning trees THEN Kruskal's algorithm SHALL work on grid graphs
5. WHEN outlining regions THEN cell outlining algorithms SHALL trace boundaries correctly
6. WHEN working with chiseled paths THEN pathfinding SHALL handle blocked cells and custom constraints

### Requirement 11: Mesh Operations and Geometry

**User Story:** As a developer working with 3D geometry, I want complete mesh generation and manipulation capabilities, so that I can create and process 3D models from grids.

#### Acceptance Criteria

1. WHEN generating meshes THEN MeshData SHALL support vertices, indices, normals, and UVs
2. WHEN working with dual meshes THEN DualMeshBuilder SHALL create dual polyhedra
3. WHEN applying Conway operators THEN mesh subdivision and modification SHALL be supported
4. WHEN working with mesh topology THEN edge and face relationships SHALL be maintained
5. WHEN raycasting meshes THEN efficient ray-mesh intersection SHALL be available
6. WHEN working with mesh utilities THEN common operations (merge, split, etc.) SHALL be supported
7. WHEN working with prism info THEN 2D-to-3D extrusion data SHALL be available

### Requirement 12: Voronoi and Delaunay Triangulation

**User Story:** As a developer working with computational geometry, I want robust Voronoi and Delaunay algorithms, so that I can create natural-looking tessellations and spatial structures.

#### Acceptance Criteria

1. WHEN computing Delaunay triangulation THEN robust triangulation of point sets SHALL be supported
2. WHEN computing Voronoi diagrams THEN dual Voronoi cells SHALL be correctly generated
3. WHEN working with Voronoi grids THEN point-based grid generation SHALL work efficiently
4. WHEN working with spherical Voronoi THEN spherical surface tessellation SHALL be supported
5. WHEN handling degenerate cases THEN algorithms SHALL handle collinear and cocircular points correctly

### Requirement 13: Deformation and Interpolation

**User Story:** As a developer working with curved surfaces, I want deformation and interpolation capabilities, so that I can map flat grids onto curved surfaces and create smooth transitions.

#### Acceptance Criteria

1. WHEN applying deformations THEN grid cells SHALL be mapped to curved surfaces correctly
2. WHEN interpolating triangles THEN barycentric interpolation SHALL work correctly
3. WHEN interpolating quads THEN bilinear interpolation SHALL work correctly
4. WHEN working with deformation utilities THEN common deformation operations SHALL be available

### Requirement 14: Export and Visualization

**User Story:** As a developer, I want to export grid data in standard formats, so that I can visualize and process grids in external tools.

#### Acceptance Criteria

1. WHEN exporting to SVG THEN 2D grids SHALL be correctly rendered as vector graphics
2. WHEN exporting mesh data THEN 3D grids SHALL be exportable as standard mesh formats
3. WHEN working with visualization THEN grid structure SHALL be clearly representable

### Requirement 15: Memory Management and Performance

**User Story:** As a C developer, I want explicit and efficient memory management, so that I can use the library in resource-constrained environments without memory leaks.

#### Acceptance Criteria

1. WHEN creating any object THEN corresponding destroy functions SHALL be available
2. WHEN allocating memory THEN all allocations SHALL be properly freed by library functions
3. WHEN working with large grids THEN memory usage SHALL be efficient and predictable
4. WHEN performing operations THEN algorithms SHALL have reasonable time complexity
5. WHEN handling errors THEN all error conditions SHALL be properly reported without memory leaks

### Requirement 16: Error Handling and Robustness

**User Story:** As a developer, I want comprehensive error handling, so that I can build robust applications that gracefully handle edge cases and invalid inputs.

#### Acceptance Criteria

1. WHEN calling any function THEN error conditions SHALL be clearly indicated via return codes
2. WHEN providing invalid inputs THEN functions SHALL return appropriate error codes without crashing
3. WHEN working with out-of-bounds data THEN bounds checking SHALL be performed consistently
4. WHEN encountering memory allocation failures THEN functions SHALL fail gracefully
5. WHEN working with infinite grids THEN operations requiring finite grids SHALL return appropriate errors

### Requirement 17: Platform Compatibility and Standards

**User Story:** As a developer targeting multiple platforms, I want the library to work consistently across different systems, so that I can deploy to embedded systems, mobile devices, and desktop platforms.

#### Acceptance Criteria

1. WHEN compiling THEN the library SHALL use only C99 standard features
2. WHEN building THEN CMake SHALL work on Linux, macOS, Windows, and embedded systems
3. WHEN linking THEN only standard C library and math library SHALL be required
4. WHEN using THEN the library SHALL work with both static and dynamic linking
5. WHEN integrating THEN CMake package config SHALL enable easy integration

### Requirement 18: Testing and Validation

**User Story:** As a developer, I want comprehensive tests that validate correctness against the original C# implementation, so that I can trust the library's behavior.

#### Acceptance Criteria

1. WHEN running tests THEN all grid types SHALL have comprehensive test coverage
2. WHEN testing algorithms THEN results SHALL match the original C# implementation
3. WHEN testing edge cases THEN boundary conditions and error cases SHALL be covered
4. WHEN testing performance THEN benchmarks SHALL validate acceptable performance characteristics
5. WHEN testing memory THEN no memory leaks SHALL be detected in test runs

### Requirement 19: Documentation and Examples

**User Story:** As a developer learning the library, I want complete documentation and examples, so that I can quickly understand how to use all features effectively.

#### Acceptance Criteria

1. WHEN reading documentation THEN all public functions SHALL have complete Doxygen documentation
2. WHEN following examples THEN working code samples SHALL be provided for all major features
3. WHEN migrating from C# THEN mapping between C# and C APIs SHALL be clearly documented
4. WHEN building THEN clear build instructions SHALL be provided for all supported platforms
5. WHEN integrating THEN integration examples SHALL be provided for common use cases