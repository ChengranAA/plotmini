CC      ?= cc
CFLAGS  ?= -std=c99 -O2 -Wall -Wextra
LDFLAGS ?= -framework Cocoa -framework Metal -framework MetalKit -lc++

BUILD_DIR = build

# ---- source lists ---------------------------------------------------
SRCS_2D = $(wildcard 2d/*.c)
SRCS_3D = $(wildcard 3d/*.c)

BINS_2D = $(SRCS_2D:2d/%.c=$(BUILD_DIR)/%)
BINS_3D = $(SRCS_3D:3d/%.c=$(BUILD_DIR)/%)
BINS    = $(BINS_2D) $(BINS_3D)

.PHONY: all clean

all: $(BUILD_DIR) $(BINS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# ---- 2D examples (link with MiniFB) ---------------------------------
$(BINS_2D): $(BUILD_DIR)/%: 2d/%.c 2d/dep/libminifb.a | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $< 2d/dep/libminifb.a $(LDFLAGS)

# ---- 3D examples (no MiniFB dependency) -----------------------------
$(BINS_3D): $(BUILD_DIR)/%: 3d/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# ---- clean ----------------------------------------------------------
clean:
	rm -rf $(BUILD_DIR)
