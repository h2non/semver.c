//
// semver_test.c
//
// Copyright (c) 2015 Tomas Aparicio
// MIT licensed
//

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "semver.h"

#define test_start(x) \
  printf("\n# Test: %s\n", x)  \

#define test_end() \
  printf("OK\n")  \

/**
 * Simple string parsing
 */
void
test_parse_simple() {
  test_start("parse_simple");

  char buf[] = "1.2.12";
  semver_t ver;

  int error = semver_parse(buf, &ver);

  assert(error == 0);
  assert(ver.major == 1);
  assert(ver.minor == 2);
  assert(ver.patch == 12);

  test_end();
}

void
test_parse_major() {
  test_start("parse_major");

  char buf[] = "1";
  semver_t ver;

  int error = semver_parse(buf, &ver);

  assert(error == 0);
  assert(ver.major == 1);
  assert(ver.minor == 0);
  assert(ver.patch == 0);

  test_end();
}

void
test_parse_minor() {
  test_start("parse_minor");

  char buf[] = "1.2";
  semver_t ver;

  int error = semver_parse(buf, &ver);

  assert(error == 0);
  assert(ver.major == 1);
  assert(ver.minor == 2);
  assert(ver.patch == 0);

  test_end();
}

void
test_parse_compare() {
  test_start("parse_compare");

  static struct test_case {
    char * x;
    char * y;
    int  expected;
  };

  struct test_case cases[] = {
    {"1", "0", 1},
    {"1", "1", 0},
    {"1", "3", -1},
    {"1.5", "0.8", 1},
    {"1.5", "1.3", 1},
    {"1.2", "2.2", -1},
    {"3.0", "1.5", 1},
    {"1.5", "1.5", 0},
    {"1.0.9", "1.0.0", 1},
    {"1.0.9", "1.0.9", 0},
    {"1.1.5", "1.1.9", -1},
    {"1.2.2", "1.1.9", 1},
    {"1.2.2", "1.2.9", -1},
  };

  int len = sizeof(cases) / sizeof(cases[0]);
  for (int i = 0; i < len; i++) {
    semver_t verX;
    semver_t verY;
    struct test_case args = cases[i];

    char *x = strdup(args.x);
    char *y = strdup(args.y);

    semver_parse(x, &verX);
    semver_parse(y, &verY);

    int resolution = semver_compare(verX, verY);
    assert(resolution == args.expected);
  }

  test_end();
}

/**
 * Helper functions
 */
void
test_valid_chars() {
  test_start("valid_chars");

  int error;
  char chars[] = "01234556789";

  error = semver_valid_chars("1", chars);
  assert(error == 0);

  error = semver_valid_chars("159", chars);
  assert(error == 0);

  error = semver_valid_chars("1b3", chars);
  assert(error == -1);

  error = semver_valid_chars("3 0 1", chars);
  assert(error == -1);

  test_end();
}

int
main() {

  test_parse_simple();
  test_parse_major();
  test_parse_minor();
  test_parse_compare();

  // Helpers
  test_valid_chars();

  return 0;
}
