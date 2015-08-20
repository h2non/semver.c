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

struct test_case {
  char * x;
  char * y;
  int  expected;
};

typedef int (*fn)(semver_t, semver_t);

static void
compare_helper (char *a, char *b, int expected, fn test_fn) {
  semver_t verX;
  semver_t verY;

  char * x = strdup(a);
  char * y = strdup(b);

  semver_parse(x, &verX);
  semver_parse(y, &verY);

  int resolution = test_fn(verX, verY);
  assert(resolution == expected);
}

static void
suite_runner(struct test_case cases[], int len, fn test_fn) {
  for (int i = 0; i < len; i++) {
    struct test_case args = cases[i];
    compare_helper(args.x, args.y, args.expected, test_fn);
  }
}

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
test_parse_prerelease() {
  test_start("parse_prerelease");

  char buf[] = "1.2.12-beta.alpha.1.1";
  semver_t ver;

  int error = semver_parse(buf, &ver);

  assert(error == 0);
  assert(ver.major == 1);
  assert(ver.minor == 2);
  assert(ver.patch == 12);
  assert(strcmp(ver.prerelease, "beta.alpha.1.1") == 0);

  test_end();
}

void
test_parse_metadata() {
  test_start("parse_metadata");

  char buf[] = "1.2.12+20130313144700";
  semver_t ver;

  int error = semver_parse(buf, &ver);

  assert(error == 0);
  assert(ver.major == 1);
  assert(ver.minor == 2);
  assert(ver.patch == 12);
  assert(strcmp(ver.metadata, "20130313144700") == 0);

  test_end();
}

void
test_parse_prerelerease_metadata() {
  test_start("parse_prerelease_metadata");

  char buf[] = "1.2.12-alpha.1+20130313144700";
  semver_t ver;

  int error = semver_parse(buf, &ver);

  assert(error == 0);
  assert(ver.major == 1);
  assert(ver.minor == 2);
  assert(ver.patch == 12);
  assert(strcmp(ver.prerelease, "alpha.1") == 0);
  assert(strcmp(ver.metadata, "20130313144700") == 0);

  test_end();
}

void
test_compare() {
  test_start("semver_compare");

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

  suite_runner(cases, 13, &semver_compare);
  test_end();
}

void
test_compare_full() {
  test_start("semver_compare_full");

  struct test_case cases[] = {
    {"1.5.1-beta.1", "1.5.1-alpha.1", 0},
  };

  suite_runner(cases, 1, &semver_compare);
  test_end();
}

void
test_compare_gt() {
  test_start("semver_gt");

  struct test_case cases[] = {
    {"1", "0", 1},
    {"1", "3", 0},
    {"3.0", "1", 1},
    {"1.0", "3", 0},
    {"1.0", "1", 0},
    {"1.5", "0.8", 1},
    {"1.2", "2.2", 0},
    {"3.0", "1.5", 1},
    {"1.1", "1.0.9", 1},
    {"1.0", "1.0.0", 0},
    {"1.1.9", "1.2", 0},
    {"1.0.9", "1.0.0", 1},
    {"1.0.9", "1.0.9", 0},
    {"1.1.5", "1.1.9", 0},
    {"1.2.2", "1.2.9", 0},
  };

  suite_runner(cases, 15, &semver_gt);
  test_end();
}

void
test_compare_lt() {
  test_start("semver_lt");

  struct test_case cases[] = {
    {"1", "0", 0},
    {"1", "3", 1},
    {"1.5", "0.8", 0},
    {"1.2", "2.2", 1},
    {"3.0", "1.5", 0},
    {"1.0.9", "1.0.0", 0},
    {"1.0.9", "1.0.9", 0},
    {"1.1.5", "1.1.9", 1},
    {"1.2.2", "1.2.9", 1},
  };

  suite_runner(cases, 9, &semver_lt);
  test_end();
}

void
test_compare_eq() {
  test_start("semver_eq");

  struct test_case cases[] = {
    {"1", "0", 0},
    {"1", "3", 0},
    {"1", "1", 1},
    {"1.5", "0.8", 0},
    {"1.2", "2.2", 0},
    {"3.0", "1.5", 0},
    {"1.0", "1.0", 1},
    {"1.0.9", "1.0.0", 0},
    {"1.1.5", "1.1.9", 0},
    {"1.2.2", "1.2.9", 0},
    {"1.0.0", "1.0.0", 1},
  };

  suite_runner(cases, 11, &semver_eq);
  test_end();
}

void
test_compare_ne() {
  test_start("semver_ne");

  struct test_case cases[] = {
    {"1", "0", 1},
    {"1", "3", 1},
    {"1", "1", 0},
    {"1.5", "0.8", 1},
    {"1.2", "2.2", 1},
    {"3.0", "1.5", 1},
    {"1.0", "1.0", 0},
    {"1.0.9", "1.0.0", 1},
    {"1.1.5", "1.1.9", 1},
    {"1.2.2", "1.2.9", 1},
    {"1.0.0", "1.0.0", 0},
  };

  suite_runner(cases, 11, &semver_ne);
  test_end();
}

void
test_compare_gte() {
  test_start("semver_gte");

  struct test_case cases[] = {
    {"1", "0", 1},
    {"1", "3", 0},
    {"1", "1", 1},
    {"1.5", "0.8", 1},
    {"1.2", "2.2", 0},
    {"3.0", "1.5", 1},
    {"1.0", "1.0", 1},
    {"1.0.9", "1.0.0", 1},
    {"1.1.5", "1.1.9", 0},
    {"1.2.2", "1.2.9", 0},
    {"1.0.0", "1.0.0", 1},
  };

  suite_runner(cases, 11, &semver_gte);
  test_end();
}

void
test_compare_lte() {
  test_start("semver_lte");

  struct test_case cases[] = {
    {"1", "0", 0},
    {"1", "3", 1},
    {"1", "1", 1},
    {"1.5", "0.8", 0},
    {"1.2", "2.2", 1},
    {"3.0", "1.5", 0},
    {"1.0", "1.0", 1},
    {"1.0.9", "1.0.0", 0},
    {"1.1.5", "1.1.9", 1},
    {"1.2.2", "1.2.9", 1},
    {"1.0.0", "1.0.0", 1},
  };

  suite_runner(cases, 11, &semver_lte);
  test_end();
}


void
test_match() {
  test_start("semver_match");

  semver_t x = {1, 2, 6};
  semver_t y = {1, 6, 0};

  assert(semver_match(">=", x, y) == 0);
  assert(semver_match("<=", x, y) == 1);
  assert(semver_match(">", x, y)  == 0);
  assert(semver_match("<", x, y)  == 1);
  assert(semver_match("=", x, y)  == 0);
  // to do
  assert(semver_match("^", x, y)  == 0);
  assert(semver_match("~", x, y)  == 0);

  test_end();
}

/**
 * Renders
 */
void
test_render() {
  test_start("valid_render");

  semver_t ver = {1, 5, 8};
  char * str[1];
  semver_render(&ver, str);
  assert(strcmp(str, "1.5.8") == 0);

  semver_t ver2 = {1, 5, 8};
  ver2.prerelease = "alpha.1";
  ver2.metadata = "1232323";
  char * str2[1];
  semver_render(&ver2, str2);
  assert(strcmp(str2, "1.5.8-alpha.1+1232323") == 0);

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

  // Parser
  test_parse_simple();
  test_parse_major();
  test_parse_minor();
  test_parse_prerelease();
  test_parse_metadata();
  test_parse_prerelerease_metadata();

  // Comparison
  test_compare();
  test_compare_full();
  test_compare_gt();
  test_compare_lt();
  test_compare_eq();
  test_compare_ne();
  test_compare_gte();
  test_compare_lte();
  test_match();

  // Render
  //test_render();

  // Private helpers
  test_valid_chars();

  return 0;
}
