CC      ?= cc
CFLAGS  ?= -std=c99 -O2 -Wall -Wextra
LDFLAGS ?= -framework Cocoa -framework Metal -framework MetalKit -lc++

BUILD_DIR = build
SRCS = $(wildcard *.c)
BINS = $(SRCS:%.c=$(BUILD_DIR)/%)

.PHONY: all clean

all: $(BUILD_DIR) $(BINS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%: %.c dep/libminifb.a ../plotmini.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $< dep/libminifb.a $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)
