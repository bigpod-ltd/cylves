/**
 * @file connection.c
 * @brief Connection system implementation for cell adjacency
 */

#include "sylves/connection.h"

SylvesConnection sylves_connection_create(SylvesCellRotation rotation, bool is_mirror) {
    SylvesConnection conn;
    conn.rotation = rotation;
    conn.is_mirror = is_mirror;
    return conn;
}

SylvesConnection sylves_connection_identity(void) {
    return sylves_connection_create(0, false);
}

SylvesConnection sylves_connection_invert(SylvesConnection conn) {
    SylvesConnection result;
    // For simple rotations, the inverse is the negative rotation
    // For mirrors, applying twice gives identity
    result.rotation = conn.is_mirror ? conn.rotation : -conn.rotation;
    result.is_mirror = conn.is_mirror;
    return result;
}

SylvesConnection sylves_connection_compose(SylvesConnection a, SylvesConnection b) {
    SylvesConnection result;
    
    // Compose rotations
    if (a.is_mirror && b.is_mirror) {
        // Two mirrors cancel out
        result.rotation = a.rotation - b.rotation;
        result.is_mirror = false;
    } else if (a.is_mirror) {
        // Mirror then rotate
        result.rotation = a.rotation - b.rotation;
        result.is_mirror = true;
    } else if (b.is_mirror) {
        // Rotate then mirror
        result.rotation = a.rotation + b.rotation;
        result.is_mirror = true;
    } else {
        // Two rotations combine
        result.rotation = a.rotation + b.rotation;
        result.is_mirror = false;
    }
    
    return result;
}

bool sylves_connection_equals(SylvesConnection a, SylvesConnection b) {
    return a.rotation == b.rotation && a.is_mirror == b.is_mirror;
}

bool sylves_connection_is_identity(SylvesConnection conn) {
    return conn.rotation == 0 && !conn.is_mirror;
}
