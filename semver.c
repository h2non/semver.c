//
// semver.c
//
// Copyright (c) 2015 Tomas Aparicio
// MIT licensed
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "semver.h"
#include "version.h"

#define NUMBERS      "0123456789"
#define ALPHA        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define DELIMITER    "."
#define PR_DELIMITER "-"
#define MT_DELIMITER "+"
#define DELIMITERS   DELIMITER PR_DELIMITER MT_DELIMITER
#define MAX_SIZE     sizeof(char) * 255
#define SLICE_SIZE   50

const int MAX_SAFE_INT = (unsigned int) -1 >> 1;

enum operators {
  SYMBOL_GT = 0x3e,
  SYMBOL_LT = 0x3c,
  SYMBOL_EQ = 0x3d,
  SYMBOL_TF = 0x7e,
  SYMBOL_CF = 0x5e
};

static int
strcut (char *str, int begin, int len) {
  int l = strlen(str);

  if (len < 0) len = l - begin;
  if (begin + len > l) len = l - begin;
  memmove(str + begin, str + begin + len, l - len + 1);

  return len;
}

static int
has_valid_chars (const char *s, const char *c) {
  size_t clen = strlen(c);
  size_t slen = strlen(s);

  for (unsigned int i = 0; i < slen; i++) {
    char v = s[i];
    unsigned match = -1;

    for (unsigned int x = 0; x < clen; x++) {
      char y = c[x];
      if (y == v) {
        match = 0;
        break;
      }
    }

    if (match) return 0;
  }

  return 1;
}

static int
parse_int (const char *s) {
  int valid = has_valid_chars(s, NUMBERS);
  if (valid == 0) return -1;
  int num = strtol(s, NULL, 10);
  if (num > MAX_SAFE_INT) return -1;
  return num;
}

static int
semver_is_alpha (const char *s) {
  return has_valid_chars(s, ALPHA);
}

static int
semver_is_number (const char *s) {
  return has_valid_chars(s, NUMBERS);
}

static char *
parse_slice (char *buf, int len, char sep) {
  char * pr = strchr(buf, sep);
  if (pr == NULL) return pr;
  int plen = strlen(pr);

  // Extract the slice from buffer
  int size = sizeof(*pr) * plen;
  char * cache[size];
  strcpy((char *) cache, buf);
  strcut((char *) cache, 0, strlen(buf) - plen + 1);

  // Allocate in heap
  char * part = malloc(size);
  strcpy(part, (char *) cache);

  // Remove chars from original buffer buffer
  int offset = strlen(buf) - strlen(pr);
  strcut(buf, offset, len);

  return part;
}

int
semver_parse (const char *str, semver_t *ver) {
  int len = strlen(str);
  char * buf[len];
  strcpy((char *) buf, str);

  int valid = semver_is_valid(str);
  if (!valid) return -1;

  ver->metadata = parse_slice((char *) buf, len, MT_DELIMITER[0]);
  ver->prerelease = parse_slice((char *) buf, len, PR_DELIMITER[0]);

  return semver_parse_version((char *) buf, ver);
}

int
semver_parse_version (const char *str, semver_t *ver) {
  int count = 0;
  char * slice = strtok((char *) str, DELIMITER);

  while (slice != NULL && count < 3) {
    count++;

    size_t len = strlen(slice);
    if (len > SLICE_SIZE) return -1;

    // Cast to integer and store
    int value = parse_int(slice);
    if (value == -1) return value;

    switch (count) {
      case 1: ver->major = value; break;
      case 2: ver->minor = value; break;
      case 3: ver->patch = value; break;
    }

    // Continue with the next slice
    slice = strtok(NULL, DELIMITER);
  }

  return 0;
}

int
semver_parse_prerelease (char *str, struct metadata_s *ver) {
  size_t len = strlen(str);
  if (len > SLICE_SIZE) return -1;

  char * slice = strtok(str, DELIMITER);

  int count = 0;
  ver->version_count = 0;

  while (slice != NULL) {
    if (count++ > SLICE_SIZE) return -1;

    // If number, cast it and store in the version buffer
    if (semver_is_number(slice)) {
      int num = parse_int(slice);
      if (num == -1) return num;
      ver->version[ver->version_count++] = num;
      slice = strtok(NULL, DELIMITER);
      continue;
    }

    // If alpha, store in the buffer
    if (count > 1 && ver->stage != NULL) {
      ver->stage = (char *) realloc(ver->stage, sizeof(slice) + 1);
      strcat(ver->stage, DELIMITER);
      strcat(ver->stage, slice);
    } else {
      char * buf = malloc(sizeof(slice));
      strcpy(buf, slice);
      ver->stage = buf;
    }

    slice = strtok(NULL, DELIMITER);
  }

  return 0;
}

static int
compare_meta_init (char *x, char *y) {
  int error;
  struct metadata_s xm = {};
  struct metadata_s ym = {};

  error = semver_parse_prerelease(x, &xm);
  if (error) {
     if (xm.stage) free((&xm)->stage);
     return error;
  }

  error = semver_parse_prerelease(y, &ym);
  if (error) {
    if (ym.stage) free((&ym)->stage);
    return error;
  }

  int resolution = semver_compare_meta(xm, ym);

  // Free from heap
  if (xm.stage) free((&xm)->stage);
  if (ym.stage) free((&ym)->stage);

  return resolution;
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
  int matches = semver_compare_version(x, y);
  if (matches) return matches;

  // Compare prerelease, if exists
  if (x.metadata == NULL
      && y.prerelease == NULL
      && x.prerelease) return -1;
  if (x.metadata == NULL
      && x.prerelease == NULL
      && y.prerelease) return 1;

  if (x.prerelease && y.prerelease) {
    int valid = compare_meta_init(x.prerelease, y.prerelease);
    if (valid) return valid;
  }

  // Compare metadata, if exists
  if (y.metadata == NULL
      && x.metadata) return -1;
  if (x.metadata == NULL
      && y.metadata) return 1;

  if (x.metadata && y.metadata) {
    int valid = compare_meta_init(x.metadata, y.metadata);
    if (valid) return valid;
  }

  return 0;
}

static int
compare_versions (int x, int y) {
  printf("Compare version: %d == %d => %d\n", x, y, x > y);
  if (x == y) return 0;
  if (x > y) return 1;
  return -1;
}

int
semver_compare_version (semver_t x, semver_t y) {
  int match;

  match = compare_versions(x.major, y.major);
  printf(">> Result comparison: %d\n\n", match);
  if (match) return match;

  match = compare_versions(x.minor, y.minor);
  if (match) return match;

  match = compare_versions(x.patch, y.patch);
  if (match) return match;

  return 0;
}

int
semver_compare_meta (struct metadata_s xm, struct metadata_s ym) {
  // Compare stage string by length
  if (xm.stage != NULL && ym.stage != NULL) {
    int xl = strlen(xm.stage);
    int yl = strlen(ym.stage);
    if (xl > yl) return -1;
    if (xl < yl) return 1;
  } else {
    if (xm.stage == NULL && ym.stage != NULL) return 1;
    if (xm.stage != NULL && ym.stage == NULL) return -1;
  }

  // Compare version numbers length
  if (xm.version_count != ym.version_count) {
    return xm.version_count < ym.version_count ? 1 : -1;
  }

  // Compare each version slice
  for (int i = 0; i < xm.version_count; i++) {
    int xv = xm.version[i];
    int yv = ym.version[i];
    if (xv > yv) return 1;
    if (xv < yv) return -1;
  }

  return 0;
}

int
semver_gt (semver_t x, semver_t y) {
  int resolution = semver_compare(x, y);
  return resolution == 1 ? 1 : 0;
}

int
semver_lt (semver_t x, semver_t y) {
  int resolution = semver_compare(x, y);
  return resolution == -1 ? 1 : 0;
}

int
semver_eq (semver_t x, semver_t y) {
  int resolution = semver_compare(x, y);
  return resolution == 0 ? 1 : 0;
}

int
semver_neq (semver_t x, semver_t y) {
  int resolution = semver_compare(x, y);
  return resolution != 0;
}

int
semver_gte (semver_t x, semver_t y) {
  int resolution = semver_compare(x, y);
  return resolution >= 0;
}

int
semver_lte (semver_t x, semver_t y) {
  int resolution = semver_compare(x, y);
  return resolution <= 0;
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
semver_satisfies (semver_t x, semver_t y, const char * operator) {
  int len = strlen(operator);
  if (len == 0) return 0;

  // Infer the comparison operator
  int op[2];
  len = len > 2 ? 2 : len;
  for (int i = 0; i < len; i++) {
    op[i] = (int) operator[i];
  }

  // Compare based on the specific operator
  if (op[0] == SYMBOL_GT) {
    if (op[1] == SYMBOL_EQ) {
      return semver_gte(x, y);
    }
    return semver_gt(x, y);
  }

  if (op[0] == SYMBOL_LT) {
    if (op[1] == SYMBOL_EQ) {
      return semver_lte(x, y);
    }
    return semver_lt(x, y);
  }

  // Strict equality
  if (op[0] == SYMBOL_EQ) return semver_eq(x, y);

  // Caret operator
  if (op[0] == SYMBOL_CF) {
    if (x.major == y.major) {
      if (x.major == 0) {
        return x.minor >= y.minor;
      } else {
        return 1;
      }
    }
    return 0;
  }

  // Tilde operator
  if (op[0] == SYMBOL_TF) {
    return x.major == y.major && x.minor == y.minor;
  }

  return 0;
}

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

void
semver_render (semver_t *x, char *dest) {
  if (x->major) concat_num(dest, x->major, NULL);
  if (x->minor) concat_num(dest, x->minor, DELIMITER);
  if (x->patch) concat_num(dest, x->patch, DELIMITER);
  if (x->prerelease) concat_char(dest, x->prerelease, PR_DELIMITER);
  if (x->metadata) concat_char(dest, x->metadata, MT_DELIMITER);
}

/**
 * Bump
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

int
semver_is_valid (const char *s) {
  char tokens[] = NUMBERS ALPHA DELIMITERS;
  return strlen(s) <= MAX_SIZE
    && has_valid_chars(s, tokens);
}
