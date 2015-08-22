CC     ?= cc
CFLAGS  = -std=c99 -Ideps -Wall -Wno-unused-function -U__STRICT_ANSI__

test: semver.c semver_test.c
	@$(CC) $^ $(CFLAGS) -o $@
	@./test

.PHONY: test
