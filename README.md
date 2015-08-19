# server.c [![Build Status](https://travis-ci.org/h2non/semver.c.png)](https://travis-ci.org/h2non/semver.c)

[semver](http://semver.org) v2.0 compliant parser written in [ANSI C](https://en.wikipedia.org/wiki/ANSI_C) with zero dependencies.

**This is much a work in progress**

## Features

- [x] Basic parsing
- [x] Version metadata parsing
- [x] Version prerelease parsing
- [x] Version comparison
- [x] Comparison helpers
- [ ] Comparison operators
- [ ] Version bump helpers
- [ ] Stringify
- [ ] 100% coverage
- [ ] Fuzz testing

## Usage

```c
#include <stdio.h>
#include <semver.h>

char current[] = "1.5.10";
char compare[] = "2.3.0";

semver_t current_version;
semver_t compare_version;

if (semver_parse(str, &current_version)
  && semver_parse(str, &compare_version)) {
  printf("Invalid semver string");
}

int resolution = semver_compare(compare_version, current_version);

if (resolution == 0) {
  printf("Versions %s is equal to: %s", compare, current)
}
else if (resolution == -1) {
  printf("Version %s is lower than: %s", compare, current)
}
else {
  printf("Version %s is higher than: %s", compare, current)
}
```

## Installation

Clone this repository:

```bash
$ git clone https://github.com/h2non/semver.c
```

Or install with [clib](https://github.com/clibs/clib):

```bash
$ clib install h2non/semver.c
```

## API

#### struct semver_t { int major, int minor, int patch, semver_pr_t pr }

semver base struct.

#### struct semver_pr { int number, const char stage }

semver prerelease struct.

#### int semver_parse(const char *str, semver_t *ver)

Parses a string as semver expression.
Returns `-1` in case parsing error due to invalid expression.

#### int semver_compare(semver_t a, semver_t b)

Compare versions `a` with `b`.

Returns:
- `-1` in case of lower version.
- `0` in case of equal versions.
- `1` in case of higher version.

#### int semver_eq(semver_t a, semver_t b)

Equality comparison.

#### int semver_ne(semver_t a, semver_t b)

Non equal comparison.

#### int semver_gt(semver_t a, semver_t b)

Greater than comparison.

#### int semver_lt(semver_t a, semver_t b)

Lower than comparison.

#### int semver_gte(semver_t a, semver_t b)

Greater than or equal comparison.

#### int semver_lte(semver_t a, semver_t b)

Lower than or equal comparison.

## License

MIT - Tomas Aparicio
