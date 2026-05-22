CC      ?= cc
CFLAGS  ?= -std=c99 -O2 -Wall -Wextra
LDFLAGS ?= -lm

SRCS   = $(wildcard test_*.c)
BINS   = $(SRCS:.c=)

.PHONY: all clean test

all: $(BINS)

%: %.c test_runner.h ../plotmini.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

test: all
	@passed=0; failed=0; \
	for t in $(BINS); do \
		echo "--- $$t ---"; \
		if ./$$t; then passed=$$((passed+1)); \
		else failed=$$((failed+1)); fi; \
		echo ""; \
	done; \
	echo "===== $$passed passed, $$failed failed ====="; \
	exit $$failed

clean:
	rm -f $(BINS)
