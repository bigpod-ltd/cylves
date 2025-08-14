# Cylves C Library Coding Standards

## Table of Contents
1. [General Principles](#general-principles)
2. [File Organization](#file-organization)
3. [Naming Conventions](#naming-conventions)
4. [Code Formatting](#code-formatting)
5. [Type Definitions](#type-definitions)
6. [Functions](#functions)
7. [Error Handling](#error-handling)
8. [Memory Management](#memory-management)
9. [Documentation](#documentation)
10. [Testing](#testing)
11. [Platform Compatibility](#platform-compatibility)

## General Principles

- **C99 Standard**: All code must be compliant with the C99 standard
- **Clarity over Cleverness**: Write clear, maintainable code that is easy to understand
- **Consistency**: Follow these standards consistently throughout the codebase
- **Safety First**: Prioritize memory safety and robustness over performance optimizations
- **No Warnings**: Code must compile without warnings on all supported platforms

## File Organization

### Directory Structure
```
reference/sylves-c/
├── src/
│   ├── include/sylves/    # Public headers
│   ├── internal/          # Internal headers
│   └── *.c               # Implementation files
├── tests/                 # Unit tests
├── examples/             # Example programs
└── docs/                 # Documentation
```

### Source Files
- One module per file (e.g., `square_grid.c` for square grid implementation)
- Private functions declared `static` in implementation files
- Internal headers in `internal/` directory for shared private functions

### Header Files
- Public API headers in `include/sylves/`
- Include guards using `SYLVES_MODULE_H` pattern
- Minimal includes in headers (prefer forward declarations)

Example header structure:
```c
#ifndef SYLVES_SQUARE_GRID_H
#define SYLVES_SQUARE_GRID_H

#include "sylves/types.h"

/* Public API declarations */

#endif /* SYLVES_SQUARE_GRID_H */
```

## Naming Conventions

### General Rules
- Use descriptive names that clearly indicate purpose
- Avoid abbreviations except for well-known terms (e.g., `max`, `min`, `ptr`)

### Specific Conventions

| Element | Convention | Example |
|---------|-----------|---------|
| Functions (public) | `sylves_module_action` | `sylves_grid_create()` |
| Functions (private) | `module_action` or `static` | `static validate_input()` |
| Types | `SylvesPascalCase` | `SylvesGrid`, `SylvesVector3` |
| Enums | `SYLVES_CONSTANT_CASE` | `SYLVES_ERROR_NULL_POINTER` |
| Enum Types | `SylvesPascalCase` | `SylvesError`, `SylvesGridType` |
| Struct Members | `snake_case` | `grid->cell_size` |
| Local Variables | `snake_case` | `cell_count` |
| Constants | `SYLVES_CONSTANT_CASE` | `SYLVES_MAX_DIMENSIONS` |
| Macros | `SYLVES_MACRO_NAME` | `SYLVES_ASSERT()` |

## Code Formatting

### Indentation and Spacing
- Use 4 spaces for indentation (no tabs)
- Maximum line length: 100 characters
- One blank line between functions
- Space after keywords: `if (`, `while (`, `for (`
- Space around operators: `a + b`, `x = y`
- No space before semicolon or comma
- Opening brace on same line for control structures

### Example
```c
static bool square_is_cell_in_grid(const SylvesGrid* grid, SylvesCell cell) {
    if (cell.z != 0) {
        return false;
    }
    
    SquareGridData* data = (SquareGridData*)grid->data;
    if (!data->is_bounded) {
        return true;
    }
    
    return cell.x >= data->min_x && cell.x <= data->max_x &&
           cell.y >= data->min_y && cell.y <= data->max_y;
}
```

### Alignment
- Align similar declarations and assignments for readability:
```c
int          x        = 10;
double       velocity = 25.5;
const char*  name     = "grid";
```

## Type Definitions

### Structures
```c
typedef struct SylvesGrid {
    const SylvesGridVTable* vtable;  /* Virtual function table */
    SylvesGridType          type;    /* Grid type identifier */
    SylvesBound*           bound;    /* Optional bounds */
    void*                  data;     /* Grid-specific data */
} SylvesGrid;
```

### Enumerations
```c
typedef enum {
    SYLVES_SUCCESS = 0,
    SYLVES_ERROR_NULL_POINTER = -1,
    SYLVES_ERROR_OUT_OF_BOUNDS = -2,
    /* ... */
} SylvesError;
```

### Function Pointers
```c
typedef bool (*SylvesGridPredicate)(const SylvesGrid* grid);
typedef void (*SylvesGridCallback)(SylvesGrid* grid, void* user_data);
```

## Functions

### Function Design
- Single responsibility principle: one function, one task
- Keep functions short (typically < 50 lines)
- Validate parameters at public API boundaries
- Use early returns for error conditions

### Parameter Order
1. Output parameters (if any)
2. Primary input object
3. Secondary input parameters
4. Size/count parameters
5. Optional parameters/callbacks
6. User data (for callbacks)

Example:
```c
int sylves_grid_get_cells(
    SylvesCell* cells,          /* Output buffer */
    const SylvesGrid* grid,     /* Primary input */
    const SylvesBound* bound,   /* Secondary input */
    size_t max_cells            /* Buffer size */
);
```

### Return Values
- Use `SylvesError` for operations that can fail
- Use `bool` for simple success/failure
- Use specific types for getters
- Return count/size as `int` (negative for errors)

## Error Handling

### Error Codes
All error codes defined in `sylves/errors.h`:
```c
#define SYLVES_CHECK(expr) \
    do { \
        SylvesError err = (expr); \
        if (err != SYLVES_SUCCESS) { \
            return err; \
        } \
    } while(0)
```

### Parameter Validation
```c
SylvesError sylves_grid_operation(SylvesGrid* grid, /* ... */) {
    if (!grid) {
        return SYLVES_ERROR_NULL_POINTER;
    }
    
    if (!sylves_grid_is_valid(grid)) {
        return SYLVES_ERROR_INVALID_STATE;
    }
    
    /* Actual implementation */
}
```

### Assertions
Use assertions for internal consistency checks (debug builds only):
```c
#ifdef DEBUG
    #define SYLVES_ASSERT(cond) assert(cond)
#else
    #define SYLVES_ASSERT(cond) ((void)0)
#endif
```

## Memory Management

### Allocation and Deallocation
- Always check allocation success
- Provide matching create/destroy functions
- Zero-initialize allocated memory with `calloc` or `memset`
- Set pointers to NULL after freeing

```c
SylvesGrid* sylves_square_grid_create(double cell_size) {
    SylvesGrid* grid = calloc(1, sizeof(SylvesGrid));
    if (!grid) {
        return NULL;
    }
    
    SquareGridData* data = calloc(1, sizeof(SquareGridData));
    if (!data) {
        free(grid);
        return NULL;
    }
    
    /* Initialize */
    grid->data = data;
    return grid;
}

void sylves_grid_destroy(SylvesGrid* grid) {
    if (grid) {
        if (grid->vtable && grid->vtable->destroy) {
            grid->vtable->destroy(grid);
        } else {
            free(grid->data);
            free(grid);
        }
    }
}
```

### Ownership Rules
- Functions that create objects transfer ownership to caller
- Functions named `*_create` allocate and return new objects
- Functions named `*_destroy` deallocate objects
- Document ownership transfer in function comments

## Documentation

### File Headers
```c
/**
 * @file square_grid.c
 * @brief Square grid implementation for the Sylves library
 * @author Your Name
 * @date 2024
 * @copyright MIT License
 */
```

### Function Documentation
```c
/**
 * @brief Creates a new unbounded square grid
 * 
 * @param cell_size Size of each square cell (must be positive)
 * @return Pointer to new grid, or NULL on allocation failure
 * 
 * @note Caller is responsible for destroying the grid with sylves_grid_destroy()
 * 
 * Example:
 * @code
 * SylvesGrid* grid = sylves_square_grid_create(1.0);
 * if (grid) {
 *     // Use grid
 *     sylves_grid_destroy(grid);
 * }
 * @endcode
 */
SylvesGrid* sylves_square_grid_create(double cell_size);
```

### Inline Comments
- Use comments to explain "why", not "what"
- Place comments above the code they describe
- Use `/* */` for multi-line comments
- Use `//` for single-line comments (C99 feature)

## Testing

### Unit Test Structure
```c
static void test_square_grid_creation(void) {
    SylvesGrid* grid = sylves_square_grid_create(1.0);
    TEST_ASSERT_NOT_NULL(grid);
    TEST_ASSERT_EQUAL(SYLVES_GRID_TYPE_SQUARE, grid->type);
    
    sylves_grid_destroy(grid);
}

static void test_square_grid_invalid_size(void) {
    SylvesGrid* grid = sylves_square_grid_create(-1.0);
    TEST_ASSERT_NULL(grid);
}
```

### Test Coverage Requirements
- Minimum 80% code coverage
- Test all public API functions
- Test error conditions and edge cases
- Test memory management (use Valgrind on Linux)

## Platform Compatibility

### Standard Headers
Only use standard C99 headers:
```c
#include <stdbool.h>   /* bool type */
#include <stdint.h>    /* Fixed-width integers */
#include <stddef.h>    /* size_t, NULL */
#include <stdlib.h>    /* Memory allocation */
#include <string.h>    /* String operations */
#include <math.h>      /* Mathematical functions */
#include <assert.h>    /* Assertions */
```

### Platform-Specific Code
Isolate platform-specific code using preprocessor:
```c
#ifdef _WIN32
    /* Windows-specific code */
#elif defined(__linux__)
    /* Linux-specific code */
#elif defined(__APPLE__)
    /* macOS-specific code */
#else
    /* Generic fallback */
#endif
```

### Compiler Attributes
Use macros for compiler-specific attributes:
```c
#ifdef __GNUC__
    #define SYLVES_UNUSED __attribute__((unused))
    #define SYLVES_PACKED __attribute__((packed))
#else
    #define SYLVES_UNUSED
    #define SYLVES_PACKED
#endif
```

## Code Review Checklist

Before submitting code for review, ensure:

- [ ] Code compiles without warnings on all target platforms
- [ ] All tests pass
- [ ] Code follows naming conventions
- [ ] Functions have proper documentation
- [ ] Error conditions are handled
- [ ] Memory is properly managed (no leaks)
- [ ] Code is formatted according to standards
- [ ] Complex logic is commented
- [ ] No commented-out code
- [ ] No debug prints in production code
- [ ] No C++ code, including header files

## Enforcement

These standards are enforced through:
1. Automated formatting checks (clang-format)
2. Static analysis (cppcheck, clang-tidy)
3. Code review process
4. CI pipeline validation

Configuration files for these tools are provided in the repository.
