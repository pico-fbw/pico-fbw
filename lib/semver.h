#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SEMVER_VERSION
#define SEMVER_VERSION "0.2.0"
#endif

/**
 * semver_t struct
 */

typedef struct semver_version_s {
  i32 major;
  i32 minor;
  i32 patch;
  char * metadata;
  char * prerelease;
} semver_t;

/**
 * Set prototypes
 */

i32
semver_satisfies (semver_t x, semver_t y, const char *op);

i32
semver_satisfies_caret (semver_t x, semver_t y);

i32
semver_satisfies_patch (semver_t x, semver_t y);

i32
semver_compare (semver_t x, semver_t y);

i32
semver_compare_version (semver_t x, semver_t y);

i32
semver_compare_prerelease (semver_t x, semver_t y);

i32
semver_gt (semver_t x, semver_t y);

i32
semver_gte (semver_t x, semver_t y);

i32
semver_lt (semver_t x, semver_t y);

i32
semver_lte (semver_t x, semver_t y);

i32
semver_eq (semver_t x, semver_t y);

i32
semver_neq (semver_t x, semver_t y);

i32
semver_parse (const char *str, semver_t *ver);

i32
semver_parse_version (const char *str, semver_t *ver);

void
semver_render (semver_t *x, char *dest);

i32
semver_numeric (semver_t *x);

void
semver_bump (semver_t *x);

void
semver_bump_minor (semver_t *x);

void
semver_bump_patch (semver_t *x);

void
semver_free (semver_t *x);

i32
semver_is_valid (const char *s);

i32
semver_clean (char *s);

#ifdef __cplusplus
}
#endif
