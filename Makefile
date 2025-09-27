# SDL2 Performance Test build system for the Miyoo Mini toolchain

SRC_DIR       := src
INC_DIR       := include
BUILD_DIR     ?= build
BIN_DIR       := $(BUILD_DIR)/bin
OBJ_DIR       := $(BUILD_DIR)/obj

PROGRAMS      := sdl2_bench_software_double_buf \
                 sdl2_bench_double_buf \
                 sdl2_space_bench \
                 sdl2_render_suite \
                 sdl2_audio_bench

TARGETS       := $(addprefix $(BIN_DIR)/,$(PROGRAMS))

COMMON_SOURCES := \
    $(SRC_DIR)/common/format.c \
    $(SRC_DIR)/common/geometry/core.c \
    $(SRC_DIR)/common/geometry/shapes.c \
    $(SRC_DIR)/common/geometry/cube.c \
    $(SRC_DIR)/common/geometry/octahedron.c \
    $(SRC_DIR)/common/geometry/tetrahedron.c \
    $(SRC_DIR)/common/geometry/sphere.c \
    $(SRC_DIR)/common/geometry/icosahedron.c \
    $(SRC_DIR)/common/geometry/pentagonal_prism.c \
    $(SRC_DIR)/common/geometry/square_pyramid.c \
    $(SRC_DIR)/common/metrics.c \
    $(SRC_DIR)/common/overlay.c \
    $(SRC_DIR)/common/overlay_grid.c
COMMON_OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(COMMON_SOURCES))

SPACE_SOURCES := \
    $(SRC_DIR)/space_bench/input.c \
    $(SRC_DIR)/space_bench/main.c \
    $(SRC_DIR)/space_bench/overlay.c \
    $(SRC_DIR)/space_bench/render.c \
    $(SRC_DIR)/space_bench/state.c
SPACE_OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SPACE_SOURCES))
SPACE_TARGET  := $(BIN_DIR)/sdl2_space_bench

SOFTWARE_SOURCES := \
    $(SRC_DIR)/software_buf/input.c \
    $(SRC_DIR)/software_buf/main.c \
    $(SRC_DIR)/software_buf/overlay.c \
    $(SRC_DIR)/software_buf/particles.c \
    $(SRC_DIR)/software_buf/render.c \
    $(SRC_DIR)/software_buf/state.c
SOFTWARE_OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SOFTWARE_SOURCES))
SOFTWARE_TARGET  := $(BIN_DIR)/sdl2_bench_software_double_buf

DOUBLE_SOURCES := \
    $(SRC_DIR)/double_buf/input.c \
    $(SRC_DIR)/double_buf/main.c \
    $(SRC_DIR)/double_buf/overlay.c \
    $(SRC_DIR)/double_buf/particles.c \
    $(SRC_DIR)/double_buf/render.c \
    $(SRC_DIR)/double_buf/state.c
DOUBLE_OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(DOUBLE_SOURCES))
DOUBLE_TARGET  := $(BIN_DIR)/sdl2_bench_double_buf

RENDER_SOURCES := \
    $(SRC_DIR)/render_suite/input.c \
    $(SRC_DIR)/render_suite/main.c \
    $(SRC_DIR)/render_suite/overlay.c \
    $(SRC_DIR)/render_suite/resources.c \
    $(SRC_DIR)/render_suite/state.c \
    $(SRC_DIR)/render_suite/scenes/fill.c \
    $(SRC_DIR)/render_suite/scenes/lines.c \
    $(SRC_DIR)/render_suite/scenes/texture.c \
    $(SRC_DIR)/render_suite/scenes/geometry.c \
    $(SRC_DIR)/render_suite/scenes/scaling.c \
    $(SRC_DIR)/render_suite/scenes/memory.c \
    $(SRC_DIR)/render_suite/scenes/pixels.c
RENDER_OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(RENDER_SOURCES))
RENDER_TARGET  := $(BIN_DIR)/sdl2_render_suite

AUDIO_SOURCES := \
    $(SRC_DIR)/audio_bench/audio_device.c \
    $(SRC_DIR)/audio_bench/input.c \
    $(SRC_DIR)/audio_bench/main.c \
    $(SRC_DIR)/audio_bench/overlay.c \
    $(SRC_DIR)/audio_bench/waveform.c
AUDIO_OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(AUDIO_SOURCES))
AUDIO_TARGET  := $(BIN_DIR)/sdl2_audio_bench

ALL_OBJECTS   := $(COMMON_OBJECTS) \
                 $(SPACE_OBJECTS) \
                 $(SOFTWARE_OBJECTS) \
                 $(DOUBLE_OBJECTS) \
                 $(RENDER_OBJECTS) \
                 $(AUDIO_OBJECTS)
DEPS          := $(ALL_OBJECTS:.o=.d)

# Toolchain ------------------------------------------------------------------
CROSS_PREFIX  ?= /opt/miyoomini-toolchain/usr/bin/arm-linux-gnueabihf-
CC            := $(CROSS_PREFIX)gcc
AR            := $(CROSS_PREFIX)ar

SYSROOT       ?= /opt/miyoomini-toolchain/usr/arm-linux-gnueabihf/sysroot
SYSROOT_FLAG  :=
ifneq ($(SYSROOT),)
SYSROOT_FLAG  := --sysroot=$(SYSROOT)
endif

SDL_INCLUDE   := $(SYSROOT)/usr/include/SDL2
SDL_LIBDIR    := $(SYSROOT)/usr/lib

# Flags ----------------------------------------------------------------------
CFLAGS       ?= -O2
CFLAGS       += -std=c11 -Wall -Wextra -D_REENTRANT -DMMIYOO
CPPFLAGS     += $(SYSROOT_FLAG) -I$(SDL_INCLUDE) -I$(SYSROOT)/usr/include -I$(INC_DIR) -I$(SRC_DIR)
LDFLAGS      += $(SYSROOT_FLAG) -L$(SDL_LIBDIR)
LDLIBS       += -lSDL2 -lSDL2_ttf -lm -lpthread

# Shared objects to bundle next to the binary
SDL_SHARED_LIBS := \
	libSDL2-2.0.so.0 \
	libSDL2_ttf-2.0.so.0

.PHONY: all clean bundle print-config test

all: $(TARGETS)

$(SPACE_TARGET): $(COMMON_OBJECTS) $(SPACE_OBJECTS) | $(BIN_DIR)
	$(CC) $(COMMON_OBJECTS) $(SPACE_OBJECTS) $(LDFLAGS) $(LDLIBS) -o $@
	@echo "Built $@ successfully"

$(SOFTWARE_TARGET): $(COMMON_OBJECTS) $(SOFTWARE_OBJECTS) | $(BIN_DIR)
	$(CC) $(COMMON_OBJECTS) $(SOFTWARE_OBJECTS) $(LDFLAGS) $(LDLIBS) -o $@
	@echo "Built $@ successfully"

$(DOUBLE_TARGET): $(COMMON_OBJECTS) $(DOUBLE_OBJECTS) | $(BIN_DIR)
	$(CC) $(COMMON_OBJECTS) $(DOUBLE_OBJECTS) $(LDFLAGS) $(LDLIBS) -o $@
	@echo "Built $@ successfully"

$(RENDER_TARGET): $(COMMON_OBJECTS) $(RENDER_OBJECTS) | $(BIN_DIR)
	$(CC) $(COMMON_OBJECTS) $(RENDER_OBJECTS) $(LDFLAGS) $(LDLIBS) -o $@
	@echo "Built $@ successfully"

$(AUDIO_TARGET): $(COMMON_OBJECTS) $(AUDIO_OBJECTS) | $(BIN_DIR)
	$(CC) $(COMMON_OBJECTS) $(AUDIO_OBJECTS) $(LDFLAGS) $(LDLIBS) -o $@
	@echo "Built $@ successfully"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

bundle: $(TARGETS)
	@echo "Bundling SDL2 libraries..."
	@set -e; \
	for lib in $(SDL_SHARED_LIBS); do \
		if [ -f $(SDL_LIBDIR)/$$lib ]; then \
			cp -u $(SDL_LIBDIR)/$$lib $(BIN_DIR)/; \
			echo "Copied $$lib"; \
		else \
			echo "Warning: $$lib not found in $(SDL_LIBDIR)" >&2; \
		fi; \
	done

$(BIN_DIR):
	@mkdir -p $@

$(OBJ_DIR):
	@mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)
	@echo "Cleaned build directory"

print-config:
	@echo "=== Build Configuration ==="
	@echo "CC        = $(CC)"
	@echo "SYSROOT   = $(SYSROOT)"
	@echo "CFLAGS    = $(CFLAGS)"
	@echo "LDFLAGS   = $(LDFLAGS)"
	@echo "LDLIBS    = $(LDLIBS)"
	@echo "PROGRAMS  = $(PROGRAMS)"
	@echo "TARGETS   = $(TARGETS)"

test: $(TARGETS)
	@echo "Binaries built successfully: $(TARGETS)"
	@for bin in $(TARGETS); do \
		file $$bin; \
		ls -lh $$bin; \
	done

-include $(DEPS)
