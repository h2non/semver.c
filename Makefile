CC      ?= cc
CFLAGS   = -std=c89 -Ideps -Wall -Wextra -pedantic -Wno-unused-function
VALGRIND = valgrind

test: semver.c semver_test.c
	@$(CC) $^ $(CFLAGS) -o $@
	@./test

valgrind: ./test
	@$(VALGRIND) --leak-check=full --error-exitcode=1 $^

.PHONY: test
