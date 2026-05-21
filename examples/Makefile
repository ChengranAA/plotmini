CC      ?= cc
CFLAGS  ?= -std=c99 -O2 -Wall -Wextra
LDFLAGS ?= -framework Cocoa -framework Metal -framework MetalKit -lc++

SRCS = $(wildcard *.c)
BINS = $(SRCS:.c=)

.PHONY: all clean

all: $(BINS)

%: %.c dep/libminifb.a ../plotmini.h
	$(CC) $(CFLAGS) -o $@ $< dep/libminifb.a $(LDFLAGS)

clean:
	rm -f $(BINS)
