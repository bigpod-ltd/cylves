/**
 * @file connection.h
 * @brief Connection system for cell adjacency
 */

#ifndef SYLVES_CONNECTION_H
#define SYLVES_CONNECTION_H

#include "types.h"
#include <stdbool.h>


/**
 * @brief Create a connection with specified rotation and mirror
 */
SylvesConnection sylves_connection_create(SylvesCellRotation rotation, bool is_mirror);

/**
 * @brief Create an identity connection (no rotation or mirror)
 */
SylvesConnection sylves_connection_identity(void);

/**
 * @brief Get the inverse of a connection
 */
SylvesConnection sylves_connection_invert(SylvesConnection conn);

/**
 * @brief Compose two connections (apply b then a)
 */
SylvesConnection sylves_connection_compose(SylvesConnection a, SylvesConnection b);

/**
 * @brief Check if two connections are equal
 */
bool sylves_connection_equals(SylvesConnection a, SylvesConnection b);

/**
 * @brief Check if a connection is the identity
 */
bool sylves_connection_is_identity(SylvesConnection conn);


#endif /* SYLVES_CONNECTION_H */
