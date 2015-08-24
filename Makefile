CC      ?= cc
CFLAGS   = -std=c99 -Ideps -Wall -Wno-unused-function -U__STRICT_ANSI__
VALGRIND = valgrind

test: semver.c semver_test.c
	@$(CC) $^ $(CFLAGS) -o $@
	@./test

leaks:
	@$(VALGRIND) --leak-check=full --error-exitcode=1 ./test

.PHONY: test
