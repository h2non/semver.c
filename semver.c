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

#define NUMBERS    "0123456789"
#define ALPHA      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define TOKENS     ".-+"
#define DELIMITER  "."
#define MAX_SIZE   sizeof(char) * 255
#define SLICE_SIZE 50

const int MAX_SAFE_INT = (unsigned int) -1 >> 1;

static enum operators {
  GT  = 0x3e,
  LT  = 0x3c,
  EQ  = 0x3d,
  TF  = 0x7e,
  AF  = 0x5e
};

static char *
substr (const char *input, int offset, int len, char *dest) {
  int input_len = strlen(input);
  if (offset + len > input_len) return NULL;

  strncpy(dest, input + offset, len);
  return dest;
}

static int
strcut (char *str, int begin, int len) {
  int l = strlen(str);

  if (len < 0) len = l - begin;
  if (begin + len > l) len = l - begin;
  memmove(str + begin, str + begin + len, l - len + 1);

  return len;
}

char *
semver_parse_slice (const char *str, char *buf, char sep) {
  int len = strlen(str);

  char * pr = strchr(buf, sep);
  if (pr == NULL) return pr;
  int plen = strlen(pr);

  // Extract the slice from buffer
  int size = sizeof(*pr) * plen;
  char * cache[size];
  strcpy(cache, buf);
  strcut(cache, 0, strlen(buf) - plen + 1);

  // Allocate in heap
  char * part = malloc(size);
  strcpy(part, cache);

  // Remove chars from original buffer buffer
  int offset = strlen(buf) - strlen(pr);
  strcut(buf, offset, len);

  return part;
}

int
semver_parse (const char *str, semver_t *ver) {
  char * buf[strlen(str)];
  strcpy(buf, str);

  int valid = semver_is_valid(buf);
  if (!valid) return -1;

  ver->metadata = semver_parse_slice(str, buf, '+');
  ver->prerelease = semver_parse_slice(str, buf, '-');

  return semver_parse_version(buf, ver);
}

int
semver_parse_version (const char *str, semver_t *ver) {
  char major_s[SLICE_SIZE] = "0";
  char minor_s[SLICE_SIZE] = "0";
  char patch_s[SLICE_SIZE] = "0";

  char * slice = strtok((char*)str, DELIMITER);

  int count = 0;
  while (slice != NULL) {
    count++;

    size_t len = strlen(slice);
    if (len > SLICE_SIZE) return -1;

    switch (count) {
      case 1: strcpy(major_s, slice); break;
      case 2: strcpy(minor_s, slice); break;
      case 3: strcpy(patch_s, slice); break;
    }

    if (count == 3) break;
    slice = strtok(NULL, DELIMITER);
  }

  int major = semver_parse_int(major_s);
  if (major == -1) return major;
  ver->major = major;

  int minor = semver_parse_int(minor_s);
  if (minor == -1) return minor;
  ver->minor = minor;

  int patch = semver_parse_int(patch_s);
  if (patch == -1) return patch;
  ver->patch = patch;

  return 0;
}

int
semver_parse_prerelease (const char *str, struct metadata_t *ver) {
  size_t len = strlen(str);
  if (len > SLICE_SIZE) return -1;

  char * buf[len];
  strcpy(buf, str);

  char * slice = strtok(buf, DELIMITER);

  int count = 0;
  int vcount = 0;

  while (slice != NULL) {
    count++;

    if (semver_is_alpha(slice)) {
      if (count > 1 && ver->stage != NULL) {
        char *buf = malloc( sizeof(ver->stage) + sizeof(slice) );
        strcpy(buf, ver->stage);
        strcat(str, slice);
        ver->stage = buf;

        slice = strtok(NULL, DELIMITER);
        continue;
      }

      char *buf = malloc(sizeof(slice));
      strcpy(buf, slice);
      ver->stage = buf;
      slice = strtok(NULL, DELIMITER);
      continue;
    }

    if (semver_is_number(slice)) {
      int num = semver_parse_int(slice);
      if (num == -1) return num;
      ver->pr_version[vcount] = num;
    }

    if (semver_is_alpha(slice)) {
      for (int i = 0; i < len; i++) {
        char v = slice[i];
        ver->pr_version[vcount] = (int)v;
      }
    }

    vcount++;
    slice = strtok(NULL, DELIMITER);
  }

  char *prerelease = malloc(sizeof(str));
  strcpy(prerelease, str);
  ver->prerelease = prerelease;

  ver->pr_version_count = vcount;

  return 0;
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

  if (x.prerelease != NULL) {
    struct metadata_t xm;
    semver_parse_prerelease(x.prerelease, &xm);
  }

  if (x.metadata != NULL) {
    // To do: metadata comparison
  }

  return 0;
}

static int
compare_versions (int x, int y) {
  if (x != y) {
    if (x > y) {
      return 1;
    }
    return -1;
  }
  return 0;
}

int
semver_compare_version (semver_t x, semver_t y) {
  int match;

  match = compare_versions(x.major, y.major);
  if (match) return match;

  match = compare_versions(x.minor, y.minor);
  if (match) return match;

  match = compare_versions(x.patch, y.patch);
  if (match) return match;

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
semver_ne (semver_t x, semver_t y) {
  int resolution = semver_compare(x, y);
  return resolution == -1 || resolution == 1 ? 1 : 0;
}

int
semver_gte (semver_t x, semver_t y) {
  int resolution = semver_compare(x, y);
  return resolution == 1 || resolution == 0 ? 1 : 0;
}

int
semver_lte (semver_t x, semver_t y) {
  int resolution = semver_compare(x, y);
  return resolution == -1 || resolution == 0 ? 1 : 0;
}

int
semver_match (const char * operator, semver_t x, semver_t y) {
  int len = strlen(operator);
  if (len == 0) return -1;

  int op[2];
  len = len > 2 ? 2 : len;
  for (int i = 0; i < len; i++) {
    op[i] = (int) operator[i];
  }

  // Match operator and perform the comparison
  if (op[0] == GT) {
    if (op[1] == EQ) {
      return semver_gte(x, y);
    }
    return semver_gt(x, y);
  }

  if (op[0] == LT) {
    if (op[1] == EQ) {
      return semver_lte(x, y);
    }
    return semver_lt(x, y);
  }

  if (op[0] == EQ) {
    return semver_eq(x, y);
  }

  if (op[0] == TF) {
    return semver_gte(x, y);
  }

  if (op[0] == AF) {
    return semver_gte(x, y);
  }

  return -1;
}

void
semver_free (semver_t *x) {
  free(x->metadata);
  free(x->prerelease);
  free(x);
}

/**
 * Renders
 */

static void
concat_num (char * str, int x, char * sep) {
  char buf[MAX_SIZE];
  if (sep == NULL) sprintf(buf, "%d", x);
  else sprintf(buf, "%s%d", sep, x);
  strcat(str, buf);
}

static void
concat_char (char * str, char * x, char * sep) {
  char buf[MAX_SIZE];
  printf("Concat char: %s\n", str);
  sprintf(buf, "%s%s", sep, x);
  strcat(str, buf);
}

int
semver_render (semver_t *x, char * dest) {
  char * buf[MAX_SIZE];

  if (x->major) concat_num(buf, x->major, NULL);
  if (x->minor) concat_num(buf, x->minor, ".");
  if (x->patch) concat_num(buf, x->patch, ".");
  if (x->prerelease) concat_char(buf, x->prerelease, "-");
  if (x->metadata) concat_char(buf, x->metadata, "+");

  strcpy(dest, buf);
  return 0;
}

/**
 * Helpers
 */

int
semver_parse_int (const char *s) {
  int invalid = semver_valid_chars(s, NUMBERS);
  if (invalid) return -1;
  int num = strtol(s, NULL, 10);
  if (num > MAX_SAFE_INT) return -1;
  return num;
}

int
semver_is_alpha (const char *s) {
  return semver_valid_chars(s, ALPHA) == 0 ? 1 : 0;
}

int
semver_is_number (const char *s) {
  return semver_valid_chars(s, NUMBERS) == 0 ? 1 : 0;
}

int
semver_is_valid (const char *s) {
  char tokens[] = NUMBERS ALPHA ".-+";
  return semver_valid_chars(s, tokens) == 0 ? 1 : 0;
}

int
semver_valid_chars (const char *s, const char *c) {
  size_t clen = strlen(c);
  size_t slen = strlen(s);

  for (int i = 0; i < slen; i++) {
    char v = s[i];
    int match = -1;

    for (int x = 0; x < clen; x++) {
      char y = c[x];
      if (y == v) {
        match = 0;
        break;
      }
    }

    if (match) return -1;
  }

  return 0;
}
