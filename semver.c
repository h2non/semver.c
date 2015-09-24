//
// semver.c
//
// Copyright (c) 2015 Tomas Aparicio
// MIT licensed
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "semver.h"

#define SLICE_SIZE   50
#define DELIMITER    "."
#define PR_DELIMITER "-"
#define MT_DELIMITER "+"
#define NUMBERS      "0123456789"
#define ALPHA        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define DELIMITERS   DELIMITER PR_DELIMITER MT_DELIMITER
#define VALID_CHARS  NUMBERS ALPHA DELIMITERS

static const int MAX_SIZE     = sizeof(char) * 255;
static const int MAX_SAFE_INT = (unsigned int) -1 >> 1;

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

static int
strcut (char *str, int begin, int len) {
  int l = strlen(str);

  if (len < 0) len = l - begin;
  if (begin + len > l) len = l - begin;
  memmove(str + begin, str + begin + len, l - len + 1);

  return len;
}

static int
contains (const char c, const char *matrix, int len) {
  for (unsigned int x = 0; x < len; x++)
    if ((char) matrix[x] == c) return 1;
  return 0;
}

static int
has_valid_chars (const char *str, const char *matrix) {
  size_t len = strlen(str);
  size_t mlen = strlen(matrix);

  for (unsigned int i = 0; i < len; i++)
    if (contains(str[i], matrix, mlen) == 0)
      return 0;

  return 1;
}

static int
semver_is_alpha (const char *s) {
  return has_valid_chars(s, ALPHA);
}

static int
semver_is_number (const char *s) {
  return has_valid_chars(s, NUMBERS);
}

static int
binary_comparison (int x, int y) {
  if (x == y) return 0;
  if (x > y) return 1;
  return -1;
}

static int
parse_int (const char *s) {
  int valid = has_valid_chars(s, NUMBERS);
  if (valid == 0) return -1;

  int num = strtol(s, NULL, 10);
  if (num > MAX_SAFE_INT) return -1;

  return num;
}

static char *
parse_slice (char *buf, int len, char sep) {
  char * pr = strchr(buf, sep);
  if (pr == NULL) return pr;

  // Extract the slice from buffer
  int plen = strlen(pr);
  int size = sizeof(*pr) * plen;
  char * cache[size];
  strcpy((char *) cache, buf);
  strcut((char *) cache, 0, strlen(buf) - plen + 1);

  // Allocate in heap
  char * part = malloc(size);
  if (part == NULL) return NULL;
  strcpy(part, (char *) cache);

  // Remove chars from original buffer
  int offset = strlen(buf) - strlen(pr);
  strcut(buf, offset, len);

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

int
semver_parse (const char *str, semver_t *ver) {
  int valid = semver_is_valid(str);
  if (!valid) return -1;

  int len = strlen(str);
  char * buf[len];
  strcpy((char *) buf, str);

  ver->metadata = parse_slice((char *) buf, len, MT_DELIMITER[0]);
  ver->prerelease = parse_slice((char *) buf, len, PR_DELIMITER[0]);

  return semver_parse_version((char *) buf, ver);
}

/**
 * Parses a given string as semver expression.
 *
 * Returns:
 *
 * `0` - Parsed successfully
 * `-1` - Parse error or invalid
 */

int
semver_parse_version (const char *str, semver_t *ver) {
  int index = 0;
  char * slice = strtok((char *) str, DELIMITER);

  while (slice != NULL && index++ < 4) {
    size_t len = strlen(slice);
    if (len > SLICE_SIZE) return -1;

    // Cast to integer and store
    int value = parse_int(slice);
    if (value == -1) return value;

    switch (index) {
      case 1: ver->major = value; break;
      case 2: ver->minor = value; break;
      case 3: ver->patch = value; break;
    }

    // Continue with the next slice
    slice = strtok(NULL, DELIMITER);
  }

  return 0;
}

static int
parse_prerelease_meta (struct metadata_s *ver, const char *slice) {
  // If first alpha slice, init the heap allocation
  if (ver->meta == NULL) {
    char * buf = malloc(sizeof(slice));
    if (buf == NULL) return -1;

    strcpy(buf, slice);
    ver->meta = buf;
  }

  // Otherwise, push it into the buffer
  else {
    int size = sizeof(ver->meta) + sizeof(slice) + 1;
    char * buf = realloc(ver->meta, size);

    if (buf == NULL) {
      free(ver->meta);
      return -1;
    }

    ver->meta = buf;
    strcat(ver->meta, DELIMITER);
    strcat(ver->meta, slice);
  }

  return 0;
}

static int
parse_prerelease_version (struct metadata_s *ver, const char *slice) {
  int num = parse_int(slice);
  if (num == -1) return num;

  ver->version[ver->version_count++] = num;
  return 0;
}

/**
 * Parses the metadata slice of a semver expression.
 * This function is mostly used internally.
 *
 * Returns:
 *
 * `0` - Parsed successfully
 * `-1` - Parse error or invalid
 */

int
semver_parse_prerelease (char *str, struct metadata_s *ver) {
  ver->meta = NULL;
  ver->version_count = 0;

  size_t len = strlen(str);
  if (len > SLICE_SIZE) return -1;

  char * slice = strtok(str, DELIMITER);

  while (slice != NULL) {
    // If numeric, cast it and store in the version buffer
    if (semver_is_number(slice)) {
      if (parse_prerelease_version(ver, slice))
        return -1;
    }
    // If non-numeric, push to the buffer
    else if (parse_prerelease_meta(ver, slice)) {
      return -1;
    }

    // Continue with the next slice
    slice = strtok(NULL, DELIMITER);
  }

  return 0;
}

static int
compare_metadata_prerelease (char *x, struct metadata_s *xm) {
  int error = semver_parse_prerelease(x, xm);
  if (error) {
     if (xm->meta) free(xm->meta);
     return error;
  }
  return 0;
}

static int
compare_metadata_string (struct metadata_s xm, struct metadata_s ym) {
  if (xm.meta == NULL && ym.meta != NULL) return 1;
  if (xm.meta != NULL && ym.meta == NULL) return -1;

  // Compare strings by length (?)
  if (xm.meta != NULL && ym.meta != NULL) {
    int xl = strlen(xm.meta);
    int yl = strlen(ym.meta);
    return binary_comparison(yl, xl);
  }

  return 0;
}

static int
compare_metadata_versions (struct metadata_s xm, struct metadata_s ym) {
  // First compare that version length matches
  if (xm.version_count != ym.version_count) {
    return xm.version_count < ym.version_count ? 1 : -1;
  }

  // Then compare each version slice individually
  for (int i = 0; i < xm.version_count; i++) {
    int xv = xm.version[i];
    int yv = ym.version[i];
    int resolution = binary_comparison(xv, yv);
    if (resolution) return resolution;
  }

  return 0;
}

static int
compare_build_slice (struct metadata_s xm, struct metadata_s ym) {
  int res = 0;

  // Compare metadata strings by length
  (  (res = compare_metadata_string(xm, ym)) == 0
  // Compare versions per number range
  && (res = compare_metadata_versions(xm, ym)));

  return res;
}

static int
compare_metadata (char *x, char *y) {
  if (x == NULL && y == NULL) return 0;
  if (y == NULL && x) return -1;
  if (x == NULL && y) return 1;

  struct metadata_s xm = {};
  struct metadata_s ym = {};

  if (compare_metadata_prerelease(x, &xm)
  ||  compare_metadata_prerelease(y, &ym)) return -1;

  int resolution = compare_build_slice(xm, ym);

  // Free allocations from heap
  if (xm.meta) free((&xm)->meta);
  if (ym.meta) free((&ym)->meta);

  return resolution;
}

int
semver_compare_metadata (semver_t x, semver_t y) {
  int res = compare_metadata(x.prerelease, y.prerelease);

  if (res
  && (x.metadata == NULL
  ||  y.metadata == NULL)) return res;

  return compare_metadata(x.metadata, y.metadata);
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

int
semver_compare_version (semver_t x, semver_t y) {
  int res = 0;

  (  (res = binary_comparison(x.major, y.major)) == 0
  && (res = binary_comparison(x.minor, y.minor)) == 0
  && (res = binary_comparison(x.patch, y.patch)));

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

int
semver_compare (semver_t x, semver_t y) {
  int res = 0;

  (  (res = semver_compare_version(x, y)) == 0
  && (res = semver_compare_metadata(x, y)));

  return res;
}

/**
 * Performs a `greater than` comparison
 */

int
semver_gt (semver_t x, semver_t y) {
  return semver_compare(x, y) == 1;
}

/**
 * Performs a `lower than` comparison
 */

int
semver_lt (semver_t x, semver_t y) {
  return semver_compare(x, y) == -1;
}

/**
 * Performs a `equality` comparison
 */

int
semver_eq (semver_t x, semver_t y) {
  return semver_compare(x, y) == 0;
}

/**
 * Performs a `non equal to` comparison
 */

int
semver_neq (semver_t x, semver_t y) {
  return semver_compare(x, y) != 0;
}

/**
 * Performs a `greater than or equal` comparison
 */

int
semver_gte (semver_t x, semver_t y) {
  return semver_compare(x, y) >= 0;
}

/**
 * Performs a `lower than or equal` comparison
 */

int
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

int
semver_satisfies_caret (semver_t x, semver_t y) {
  if (x.major == y.major) {
    if (x.major == 0) {
      return x.minor >= y.minor;
    }
    return 1;
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

int
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

int
semver_satisfies (semver_t x, semver_t y, const char *op) {
  // Extract the comparison operator
  int first = op[0];
  int second = op[1];

  // Caret operator
  if (first == SYMBOL_CF)
    return semver_satisfies_caret(x, y);

  // Tilde operator
  if (first == SYMBOL_TF)
    return semver_satisfies_patch(x, y);

  // Strict equality
  if (first == SYMBOL_EQ)
    return semver_eq(x, y);

  // Greater than or equal comparison
  if (first == SYMBOL_GT) {
    if (second == SYMBOL_EQ) {
      return semver_gte(x, y);
    }
    return semver_gt(x, y);
  }

  // Lower than or equal comparison
  if (first == SYMBOL_LT) {
    if (second == SYMBOL_EQ) {
      return semver_lte(x, y);
    }
    return semver_lt(x, y);
  }

  return 0;
}

/**
 * Free allocated memory for the given version.
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
concat_num (char * str, int x, char * sep) {
  char buf[SLICE_SIZE] = {};
  if (sep == NULL) sprintf(buf, "%d", x);
  else sprintf(buf, "%s%d", sep, x);
  strcat(str, buf);
}

static void
concat_char (char * str, char * x, char * sep) {
  char buf[SLICE_SIZE] = {};
  sprintf(buf, "%s%s", sep, x);
  strcat(str, buf);
}

/**
 * Render a given semver as string
 */

void
semver_render (semver_t *x, char *dest) {
  if (x->major) concat_num(dest, x->major, NULL);
  if (x->minor) concat_num(dest, x->minor, DELIMITER);
  if (x->patch) concat_num(dest, x->patch, DELIMITER);
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

static int
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

int
semver_is_valid (const char *s) {
  return has_valid_length(s)
      && has_valid_chars(s, VALID_CHARS);
}

/**
 * Removes non-valid characters in a given string,
 * returning a new one.
 *
 * Returns:
 *
 * `0`  - Valid
 * `-1` - Invalid input
 */

int
semver_clean (const char *s, char *dest) {
  if (has_valid_length(s) == 0) return -1;

  int offset = 0;
  strcpy((char *) dest, s);
  size_t len = strlen(s);
  size_t mlen = strlen(VALID_CHARS);

  for (unsigned int i = 0; i < len; i++) {
    if (contains(s[i], VALID_CHARS, mlen) == 0) {
      strcut((char *) dest, i - offset, 1);
      offset++;
    }
  }

  return 0;
}
