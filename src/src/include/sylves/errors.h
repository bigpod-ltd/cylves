/**
 * @file errors.h
 * @brief Error codes and error handling for the Sylves library
 */

#ifndef SYLVES_ERRORS_H
#define SYLVES_ERRORS_H

#include <stdbool.h>


/**
 * @brief Error codes used throughout the Sylves library
 */
typedef enum {
    /** Operation completed successfully */
    SYLVES_SUCCESS = 0,
    
    /** A null pointer was passed where non-null was required */
    SYLVES_ERROR_NULL_POINTER = -1,
    
    /** Index or coordinate out of valid bounds */
    SYLVES_ERROR_OUT_OF_BOUNDS = -2,
    
    /** Memory allocation failed */
    SYLVES_ERROR_OUT_OF_MEMORY = -3,
    
    /** Invalid argument passed to function */
    SYLVES_ERROR_INVALID_ARGUMENT = -4,
    
    /** Feature not yet implemented */
    SYLVES_ERROR_NOT_IMPLEMENTED = -5,
    
    /** Cell not found in grid */
    SYLVES_ERROR_CELL_NOT_IN_GRID = -6,
    
    /** Operation not supported for this grid type */
    SYLVES_ERROR_NOT_SUPPORTED = -7,
    
    /** Path not found between cells */
    SYLVES_ERROR_PATH_NOT_FOUND = -8,
    
    /** Division by zero or other math error */
    SYLVES_ERROR_MATH = -9,
    
    /** Buffer too small for operation */
    SYLVES_ERROR_BUFFER_TOO_SMALL = -10,
    
    /** Grid is infinite and operation requires finite grid */
    SYLVES_ERROR_INFINITE_GRID = -11,
    
    /** Operation would create invalid state */
    SYLVES_ERROR_INVALID_STATE = -12,
    
    /** Operation on unbounded grid not allowed */
    SYLVES_ERROR_UNBOUNDED = -13,
    
    /** Invalid cell coordinates */
    SYLVES_ERROR_INVALID_CELL = -14,
    
    /** Invalid direction */
    SYLVES_ERROR_INVALID_DIR = -15,
    
    /** No neighbor cell in requested direction */
    SYLVES_ERROR_NO_NEIGHBOR = -16,
    
    /** Invalid corner index */
    SYLVES_ERROR_INVALID_CORNER = -17,
    
    /** Cell not found during search */
    SYLVES_ERROR_CELL_NOT_FOUND = -18,

    /** I/O error (file or stream) */
SYLVES_ERROR_IO = -19,
    
    /** Not found */
    SYLVES_ERROR_NOT_FOUND = -20
} SylvesError;

/**
 * @brief Get a human-readable error message for an error code
 * @param error The error code
 * @return String describing the error (do not free)
 */
const char* sylves_error_string(SylvesError error);

/**
 * @brief Check if an error code indicates success
 * @param error The error code to check
 * @return true if success, false if error
 */
static inline bool sylves_is_success(SylvesError error) {
    return error == SYLVES_SUCCESS;
}

/**
 * @brief Check if an error code indicates failure
 * @param error The error code to check
 * @return true if error, false if success
 */
static inline bool sylves_is_error(SylvesError error) {
    return error != SYLVES_SUCCESS;
}


#endif /* SYLVES_ERRORS_H */
