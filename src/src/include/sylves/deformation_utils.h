/**
 * @file deformation_utils.h
 * @brief Utility constructors for common deformations
 */
#ifndef SYLVES_DEFORMATION_UTILS_H
#define SYLVES_DEFORMATION_UTILS_H

#include "types.h"
#include "deformation.h"
#include "errors.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Create a cylindrical deformation around an axis with given radius */
SylvesDeformation* sylves_deformation_cylindrical(const SylvesVector3* axis,
                                                  float radius,
                                                  SylvesError* error_out);

/* Create a spherical deformation around a center with given radius */
SylvesDeformation* sylves_deformation_spherical(const SylvesVector3* center,
                                               float radius,
                                               SylvesError* error_out);

/* Chain two deformations: applies first, then second */
SylvesDeformation* sylves_deformation_chain(SylvesDeformation* first,
                                           SylvesDeformation* second,
                                           SylvesError* error_out);

/* Optionally optimize a deformation (may return input) */
SylvesDeformation* sylves_deformation_optimize(SylvesDeformation* deform,
                                              SylvesError* error_out);

#ifdef __cplusplus
}
#endif

#endif /* SYLVES_DEFORMATION_UTILS_H */
