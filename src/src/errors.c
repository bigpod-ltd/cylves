/**
 * @file errors.c
 * @brief Error handling implementation
 */

#include "sylves/errors.h"

const char* sylves_error_string(SylvesError error) {
    switch (error) {
        case SYLVES_SUCCESS:
            return "Success";
        case SYLVES_ERROR_NULL_POINTER:
            return "Null pointer passed where non-null required";
        case SYLVES_ERROR_OUT_OF_BOUNDS:
            return "Index or coordinate out of bounds";
        case SYLVES_ERROR_OUT_OF_MEMORY:
            return "Memory allocation failed";
        case SYLVES_ERROR_INVALID_ARGUMENT:
            return "Invalid argument";
        case SYLVES_ERROR_NOT_IMPLEMENTED:
            return "Feature not implemented";
        case SYLVES_ERROR_CELL_NOT_IN_GRID:
            return "Cell not found in grid";
        case SYLVES_ERROR_NOT_SUPPORTED:
            return "Operation not supported for this grid type";
        case SYLVES_ERROR_PATH_NOT_FOUND:
            return "Path not found between cells";
        case SYLVES_ERROR_MATH:
            return "Mathematical error";
        case SYLVES_ERROR_BUFFER_TOO_SMALL:
            return "Buffer too small for operation";
        case SYLVES_ERROR_INFINITE_GRID:
            return "Operation requires finite grid";
        case SYLVES_ERROR_INVALID_STATE:
            return "Invalid state";
        default:
            return "Unknown error";
    }
}
