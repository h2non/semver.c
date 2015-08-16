# server.c [![Build Status](https://travis-ci.org/h2non/semver.c.png)](https://travis-ci.org/h2non/semver.c)

[semver](http://semver.org) v2.0 compliant parser written in [ANSI C](https://en.wikipedia.org/wiki/ANSI_C) with zero dependencies.

**This is much a work in progress**

## Features

- [x] Basic parsing
- [ ] Version stage parsing
- [ ] Version build parsing
- [ ] Version prerelease parsing
- [x] Versions comparison
- [ ] Comparison helpers
- [ ] Comparison operators

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

`To do`

## License

MIT - Tomas Aparicio
