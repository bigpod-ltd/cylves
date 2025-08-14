#ifndef SYLVES_EXPORT_H
#define SYLVES_EXPORT_H

/*
 * Symbol visibility/export macros
 * Define SYLVES_EXPORT appropriately for shared library builds.
 */
#if defined(_WIN32) || defined(_WIN64)
  #if defined(SYLVES_BUILD_SHARED)
    #define SYLVES_EXPORT __declspec(dllexport)
  #elif defined(SYLVES_USE_SHARED)
    #define SYLVES_EXPORT __declspec(dllimport)
  #else
    #define SYLVES_EXPORT
  #endif
#else
  #if defined(SYLVES_BUILD_SHARED)
    #if defined(__GNUC__) || defined(__clang__)
      #define SYLVES_EXPORT __attribute__((visibility("default")))
    #else
      #define SYLVES_EXPORT
    #endif
  #else
    #define SYLVES_EXPORT
  #endif
#endif

#endif /* SYLVES_EXPORT_H */

