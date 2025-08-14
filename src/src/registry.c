/**
 * @file registry.c
 * @brief Simple in-process registries for grid, cell type, and bound implementations
 */

#include "sylves/registry.h"
#include <stdlib.h>
#include <string.h>

#define MAX_REG 32

static SylvesGridDescriptor grid_descs[MAX_REG];
static size_t grid_desc_count = 0;

static SylvesCellTypeDescriptor cell_type_descs[MAX_REG];
static size_t cell_type_desc_count = 0;

static SylvesBoundDescriptor bound_descs[MAX_REG];
static size_t bound_desc_count = 0;

int sylves_registry_add_grid(const SylvesGridDescriptor* desc) {
    if (!desc || !desc->name || !desc->factory) return -1;
    if (grid_desc_count >= MAX_REG) return -2;
    grid_descs[grid_desc_count++] = *desc;
    return 0;
}

const SylvesGridDescriptor* sylves_registry_get_grid_desc(const char* name) {
    if (!name) return NULL;
    for (size_t i = 0; i < grid_desc_count; ++i) {
        if (strcmp(grid_descs[i].name, name) == 0) return &grid_descs[i];
    }
    return NULL;
}

int sylves_registry_add_cell_type(const SylvesCellTypeDescriptor* desc) {
    if (!desc || !desc->name || !desc->factory) return -1;
    if (cell_type_desc_count >= MAX_REG) return -2;
    cell_type_descs[cell_type_desc_count++] = *desc;
    return 0;
}

const SylvesCellTypeDescriptor* sylves_registry_get_cell_type_desc(const char* name) {
    if (!name) return NULL;
    for (size_t i = 0; i < cell_type_desc_count; ++i) {
        if (strcmp(cell_type_descs[i].name, name) == 0) return &cell_type_descs[i];
    }
    return NULL;
}

int sylves_registry_add_bound(const SylvesBoundDescriptor* desc) {
    if (!desc || !desc->name || !desc->factory) return -1;
    if (bound_desc_count >= MAX_REG) return -2;
    bound_descs[bound_desc_count++] = *desc;
    return 0;
}

const SylvesBoundDescriptor* sylves_registry_get_bound_desc(const char* name) {
    if (!name) return NULL;
    for (size_t i = 0; i < bound_desc_count; ++i) {
        if (strcmp(bound_descs[i].name, name) == 0) return &bound_descs[i];
    }
    return NULL;
}

int sylves_registry_init(void) {
    grid_desc_count = 0; cell_type_desc_count = 0; bound_desc_count = 0;
    return 0;
}

void sylves_registry_cleanup(void) {
    grid_desc_count = 0; cell_type_desc_count = 0; bound_desc_count = 0;
}

