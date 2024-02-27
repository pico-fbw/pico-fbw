/**
 * This file utilizes code under the MIT License. See "LICENSE" for details.
 * 
 * Copyright (c) 2015-2017 Tomas Aparicio
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform/int.h"

#include "semver.h"

#define SLICE_SIZE   50
#define DELIMITER    "."
#define PR_DELIMITER "-"
#define MT_DELIMITER "+"
#define NUMBERS      "0123456789"
#define ALPHA        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define DELIMITERS   DELIMITER PR_DELIMITER MT_DELIMITER
#define VALID_CHARS  NUMBERS ALPHA DELIMITERS

static const size_t MAX_SIZE     = sizeof(char) * 255;
static const i32 MAX_SAFE_INT = (unsigned int) -1 >> 1;

/**
 * Define comparison operators, storing the
 * ASCII code per each symbol in hexadecimal notation.
 */

enum operators {
  SYMBOL_GT = 0x3e,
  SYMBOL_LT = 0x3c,
  SYMBOL_EQ = 0x3d,
  SYMBOL_TF = 0x7e,
  SYMBOL_CF = 0x5e
};

/**
 * Private helpers
 */

/*
 * Remove [begin:len-begin] from str by moving len data from begin+len to begin.
 * If len is negative cut out to the end of the string.
 */
static i32
strcut (char *str, i32 begin, i32 len) {
  size_t l;
  l = strlen(str);

  if((i32)l < 0 || (i32)l > MAX_SAFE_INT) return -1;

  if (len < 0) len = l - begin + 1;
  if (begin + len > (i32)l) len = l - begin;
  memmove(str + begin, str + begin + len, l - len + 1 - begin);

  return len;
}

static i32
contains (const char c, const char *matrix, size_t len) {
  size_t x;
  for (x = 0; x < len; x++)
    if ((char) matrix[x] == c) return 1;
  return 0;
}

static i32
has_valid_chars (const char *str, const char *matrix) {
  size_t i, len, mlen;
  len = strlen(str);
  mlen = strlen(matrix);

  for (i = 0; i < len; i++)
    if (contains(str[i], matrix, mlen) == 0)
      return 0;

  return 1;
}

static i32
binary_comparison (i32 x, i32 y) {
  if (x == y) return 0;
  if (x > y) return 1;
  return -1;
}

static i32
parse_int (const char *s) {
  i32 valid, num;
  valid = has_valid_chars(s, NUMBERS);
  if (valid == 0) return -1;

  num = strtol(s, NULL, 10);
  if (num > MAX_SAFE_INT) return -1;

  return num;
}

/*
 * Return a string allocated on the heap with the content from sep to end and
 * terminate buf at sep.
 */
static char *
parse_slice (char *buf, char sep) {
  char *pr, *part;
  i32 plen;

  /* Find separator in buf */
  pr = strchr(buf, sep);
  if (pr == NULL) return NULL;
  /* Length from separator to end of buf */
  plen = strlen(pr);

  /* Copy from buf into new string */
  part = (char*)calloc(plen + 1, sizeof(*part));
  if (part == NULL) return NULL;
  memcpy(part, pr + 1, plen);
  /* Null terminate new string */
  part[plen] = '\0';

  /* Terminate buf where separator was */
  *pr = '\0';

  return part;
}

/**
 * Parses a string as semver expression.
 *
 * Returns:
 *
 * `0` - Parsed successfully
 * `-1` - In case of error
 */

i32
semver_parse (const char *str, semver_t *ver) {
  i32 valid, res;
  size_t len;
  char *buf;
  valid = semver_is_valid(str);
  if (!valid) return -1;

  len = strlen(str);
  buf = (char*)calloc(len + 1, sizeof(*buf));
  if (buf == NULL) return -1;
  strcpy(buf, str);

  ver->metadata = parse_slice(buf, MT_DELIMITER[0]);
  ver->prerelease = parse_slice(buf, PR_DELIMITER[0]);

  res = semver_parse_version(buf, ver);
  free(buf);
#if DEBUG > 0
  printf("[debug] semver.c %s = %d.%d.%d, %s %s\n", str, ver->major, ver->minor, ver->patch, ver->prerelease, ver->metadata);
#endif
  return res;
}

/**
 * Parses a given string as semver expression.
 *
 * Returns:
 *
 * `0` - Parsed successfully
 * `-1` - Parse error or invalid
 */

i32
semver_parse_version (const char *str, semver_t *ver) {
  size_t len;
  i32 index, value;
  char *slice, *next, *endptr;
  slice = (char *) str;
  index = 0;

  while (slice != NULL && index++ < 4) {
    next = strchr(slice, DELIMITER[0]);
    if (next == NULL)
      len = strlen(slice);
    else
      len = next - slice;
    if (len > SLICE_SIZE) return -1;

    /* Cast to integer and store */
    value = strtol(slice, &endptr, 10);
    if (endptr != next && *endptr != '\0') return -1;

    switch (index) {
      case 1: ver->major = value; break;
      case 2: ver->minor = value; break;
      case 3: ver->patch = value; break;
    }

    /* Continue with the next slice */
    if (next == NULL)
      slice = NULL;
    else
      slice = next + 1;
  }

  return 0;
}

static i32
compare_prerelease (char *x, char *y) {
  char *lastx, *lasty, *xptr, *yptr, *endptr;
  i32 xlen, ylen, xisnum, yisnum, xnum, ynum;
  i32 xn, yn, min, res;
  if (x == NULL && y == NULL) return 0;
  if (y == NULL && x) return -1;
  if (x == NULL && y) return 1;

  lastx = x;
  lasty = y;
  xlen = strlen(x);
  ylen = strlen(y);

  while (1) {
    if ((xptr = strchr(lastx, DELIMITER[0])) == NULL)
      xptr = x + xlen;
    if ((yptr = strchr(lasty, DELIMITER[0])) == NULL)
      yptr = y + ylen;

    xnum = strtol(lastx, &endptr, 10);
    xisnum = endptr == xptr ? 1 : 0;
    ynum = strtol(lasty, &endptr, 10);
    yisnum = endptr == yptr ? 1 : 0;

    if (xisnum && !yisnum) return -1;
    if (!xisnum && yisnum) return 1;

    if (xisnum && yisnum) {
      /* Numerical comparison */
      if (xnum != ynum) return xnum < ynum ? -1 : 1;
    } else {
      /* String comparison */
      xn = xptr - lastx;
      yn = yptr - lasty;
      min = xn < yn ? xn : yn;
      if ((res = strncmp(lastx, lasty, min))) return res < 0 ? -1 : 1;
      if (xn != yn) return xn < yn ? -1 : 1;
    }

    lastx = xptr + 1;
    lasty = yptr + 1;
    if (lastx == x + xlen + 1 && lasty == y + ylen + 1) break;
    if (lastx == x + xlen + 1) return -1;
    if (lasty == y + ylen + 1) return 1;
  }

  return 0;
}

i32
semver_compare_prerelease (semver_t x, semver_t y) {
  return compare_prerelease(x.prerelease, y.prerelease);
}

/**
 * Performs a major, minor and patch binary comparison (x, y).
 * This function is mostly used internally
 *
 * Returns:
 *
 * `0` - If versiona are equal
 * `1` - If x is higher than y
 * `-1` - If x is lower than y
 */

i32
semver_compare_version (semver_t x, semver_t y) {
  i32 res;

  if ((res = binary_comparison(x.major, y.major)) == 0) {
    if ((res = binary_comparison(x.minor, y.minor)) == 0) {
      return binary_comparison(x.patch, y.patch);
    }
  }

  return res;
}

/**
 * Compare two semantic versions (x, y).
 *
 * Returns:
 * - `1` if x is higher than y
 * - `0` if x is equal to y
 * - `-1` if x is lower than y
 */

i32
semver_compare (semver_t x, semver_t y) {
  i32 res;

  if ((res = semver_compare_version(x, y)) == 0) {
    return semver_compare_prerelease(x, y);
  }

  return res;
}

/**
 * Performs a `greater than` comparison
 */

i32
semver_gt (semver_t x, semver_t y) {
  return semver_compare(x, y) == 1;
}

/**
 * Performs a `lower than` comparison
 */

i32
semver_lt (semver_t x, semver_t y) {
  return semver_compare(x, y) == -1;
}

/**
 * Performs a `equality` comparison
 */

i32
semver_eq (semver_t x, semver_t y) {
  return semver_compare(x, y) == 0;
}

/**
 * Performs a `non equal to` comparison
 */

i32
semver_neq (semver_t x, semver_t y) {
  return semver_compare(x, y) != 0;
}

/**
 * Performs a `greater than or equal` comparison
 */

i32
semver_gte (semver_t x, semver_t y) {
  return semver_compare(x, y) >= 0;
}

/**
 * Performs a `lower than or equal` comparison
 */

i32
semver_lte (semver_t x, semver_t y) {
  return semver_compare(x, y) <= 0;
}

/**
 * Checks if version `x` can be satisfied by `y`
 * performing a comparison with caret operator.
 *
 * See: https://docs.npmjs.com/misc/semver#caret-ranges-1-2-3-0-2-5-0-0-4
 *
 * Returns:
 *
 * `1` - Can be satisfied
 * `0` - Cannot be satisfied
 */

i32
semver_satisfies_caret (semver_t x, semver_t y) {
  /* Major versions must always match. */
  if (x.major == y.major) {
    /* If major version is 0, minor versions must match */
    if (x.major == 0) {
        /* If minor version is 0, patch must match */
        if (x.minor == 0){
          return (x.minor == y.minor) && (x.patch == y.patch);
        }
        /* If minor version is not 0, patch must be >= */
        else if (x.minor == y.minor){
          return x.patch >= y.patch;
        }
        else{
          return 0;
        }
      }
    else if (x.minor > y.minor){
      return 1;
    }
    else if (x.minor == y.minor)
    {
      return x.patch >= y.patch;
    }
    else {
      return 0;
    }
  }
  return 0;
}

/**
 * Checks if version `x` can be satisfied by `y`
 * performing a comparison with tilde operator.
 *
 * See: https://docs.npmjs.com/misc/semver#tilde-ranges-1-2-3-1-2-1
 *
 * Returns:
 *
 * `1` - Can be satisfied
 * `0` - Cannot be satisfied
 */

i32
semver_satisfies_patch (semver_t x, semver_t y) {
  return x.major == y.major
      && x.minor == y.minor;
}

/**
 * Checks if both versions can be satisfied
 * based on the given comparison operator.
 *
 * Allowed operators:
 *
 * - `=`  - Equality
 * - `>=` - Higher or equal to
 * - `<=` - Lower or equal to
 * - `<`  - Lower than
 * - `>`  - Higher than
 * - `^`  - Caret comparison (see https://docs.npmjs.com/misc/semver#caret-ranges-1-2-3-0-2-5-0-0-4)
 * - `~`  - Tilde comparison (see https://docs.npmjs.com/misc/semver#tilde-ranges-1-2-3-1-2-1)
 *
 * Returns:
 *
 * `1` - Can be satisfied
 * `0` - Cannot be satisfied
 */

i32
semver_satisfies (semver_t x, semver_t y, const char *op) {
  i32 first, second;
  /* Extract the comparison operator */
  first = op[0];
  second = op[1];

  /* Caret operator */
  if (first == SYMBOL_CF)
    return semver_satisfies_caret(x, y);

  /* Tilde operator */
  if (first == SYMBOL_TF)
    return semver_satisfies_patch(x, y);

  /* Strict equality */
  if (first == SYMBOL_EQ)
    return semver_eq(x, y);

  /* Greater than or equal comparison */
  if (first == SYMBOL_GT) {
    if (second == SYMBOL_EQ) {
      return semver_gte(x, y);
    }
    return semver_gt(x, y);
  }

  /* Lower than or equal comparison */
  if (first == SYMBOL_LT) {
    if (second == SYMBOL_EQ) {
      return semver_lte(x, y);
    }
    return semver_lt(x, y);
  }

  return 0;
}

/**
 * Free heep allocated memory of a given semver.
 * This is just a convenient function that you
 * should call when you're done.
 */

void
semver_free (semver_t *x) {
  if (x->metadata) {
    free(x->metadata);
    x->metadata = NULL;
  }
  if (x->prerelease) {
    free(x->prerelease);
    x->prerelease = NULL;
  }
}

/**
 * Renders
 */

static void
concat_num (char * str, i32 x, char * sep) {
  char buf[SLICE_SIZE] = {0};
  if (sep == NULL) sprintf(buf, "%d", x);
  else sprintf(buf, "%s%d", sep, x);
  strcat(str, buf);
}

static void
concat_char (char * str, char * x, char * sep) {
  char buf[SLICE_SIZE] = {0};
  sprintf(buf, "%s%s", sep, x);
  strcat(str, buf);
}

/**
 * Render a given semver as string
 */

void
semver_render (semver_t *x, char *dest) {
  concat_num(dest, x->major, NULL);
  concat_num(dest, x->minor, DELIMITER);
  concat_num(dest, x->patch, DELIMITER);
  if (x->prerelease) concat_char(dest, x->prerelease, PR_DELIMITER);
  if (x->metadata) concat_char(dest, x->metadata, MT_DELIMITER);
}

/**
 * Version bump helpers
 */

void
semver_bump (semver_t *x) {
  x->major++;
}

void
semver_bump_minor (semver_t *x) {
  x->minor++;
}

void
semver_bump_patch (semver_t *x) {
  x->patch++;
}

/**
 * Helpers
 */

static i32
has_valid_length (const char *s) {
  return strlen(s) <= MAX_SIZE;
}

/**
 * Checks if a given semver string is valid
 *
 * Returns:
 *
 * `1` - Valid expression
 * `0` - Invalid
 */

i32
semver_is_valid (const char *s) {
  return has_valid_length(s)
      && has_valid_chars(s, VALID_CHARS);
}

/**
 * Removes non-valid characters in the given string.
 *
 * Returns:
 *
 * `0`  - Valid
 * `-1` - Invalid input
 */

i32
semver_clean (char *s) {
  size_t i, len, mlen;
  i32 res;
  if (has_valid_length(s) == 0) return -1;

  len = strlen(s);
  mlen = strlen(VALID_CHARS);

  for (i = 0; i < len; i++) {
    if (contains(s[i], VALID_CHARS, mlen) == 0) {
      res = strcut(s, i, 1);
      if(res == -1) return -1;
      --len; --i;
    }
  }

  return 0;
}

static i32
char_to_int (const char * str) {
  i32 buf;
  size_t i,len, mlen;
  buf = 0;
  len = strlen(str);
  mlen = strlen(VALID_CHARS);

  for (i = 0; i < len; i++)
    if (contains(str[i], VALID_CHARS, mlen))
      buf += (i32) str[i];

  return buf;
}

/**
 * Render a given semver as numeric value.
 * Useful for ordering and filtering.
 */

i32
semver_numeric (semver_t *x) {
  i32 num;
  char buf[SLICE_SIZE * 3];
  memset(&buf, 0, SLICE_SIZE * 3);

  if (x->major) concat_num(buf, x->major, NULL);
  if (x->major || x->minor) concat_num(buf, x->minor, NULL);
  if (x->major || x->minor || x->patch) concat_num(buf, x->patch, NULL);

  num = parse_int(buf);
  if(num == -1) return -1;

  if (x->prerelease) num += char_to_int(x->prerelease);
  if (x->metadata) num += char_to_int(x->metadata);

  return num;
}
