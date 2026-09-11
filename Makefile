# SDL2 Performance Test build system for the Miyoo Mini toolchain

SRC_DIR       := src
INC_DIR       := include
BUILD_DIR     ?= build
BIN_DIR       := $(BUILD_DIR)/bin
OBJ_DIR       := $(BUILD_DIR)/obj

PROGRAMS      := sdl2_title \
                 sdl2_bench_double_buf \
                 sdl2_space_bench \
                 sdl2_fill_bench \
                 sdl2_texture_bench \
                 sdl2_lines_bench \
                 sdl2_geometry_bench \
                 sdl2_scaling_bench \
                 sdl2_memory_bench \
                 sdl2_pixels_bench \
                 sdl2_render_suite \
                 sdl2_gl_fbo_effects \
                 sdl2_audio_bench \
                 sdl2_sprite_bench \
                 sdl2_gfx_bench \
                 sdl2_obj_model_loader \
                 sdl2_messagebox_probe

TARGETS       := $(addprefix $(BIN_DIR)/,$(PROGRAMS))

COMMON_SOURCES := \
    $(SRC_DIR)/common/asset_path.c \
    $(SRC_DIR)/common/backend_probe.c \
    $(SRC_DIR)/common/bench_stress.c \
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
    $(SRC_DIR)/common/overlay_rows.c \
    $(SRC_DIR)/common/rolling_chart.c \
    $(SRC_DIR)/common/driver_support.c \
    $(SRC_DIR)/common/display_config.c \
    $(SRC_DIR)/common/frame_limit.c \
    $(SRC_DIR)/common/loading_screen.c \
    $(SRC_DIR)/common/gl_effect.c \
    $(SRC_DIR)/common/math3d/vec3.c \
    $(SRC_DIR)/common/math3d/mat4.c \
    $(SRC_DIR)/common/model/mesh.c \
    $(SRC_DIR)/common/model/obj_loader.c \
    $(SRC_DIR)/common/render3d/material_texture.c \
    $(SRC_DIR)/common/render3d/model_instance.c \
    $(SRC_DIR)/common/render3d/camera.c \
    $(SRC_DIR)/common/render3d/pipeline.c
ifeq ($(DEBUG),1)
COMMON_SOURCES += $(SRC_DIR)/common/overlay_debug_stats.c
endif
COMMON_OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(COMMON_SOURCES))

TITLE_SOURCES := \
    $(SRC_DIR)/title/input.c \
    $(SRC_DIR)/title/main.c \
    $(SRC_DIR)/title/menu.c \
    $(SRC_DIR)/title/config_panel.c \
    $(SRC_DIR)/title/launcher.c \
    $(SRC_DIR)/title/state.c \
    $(SRC_DIR)/title/backend_status.c \
    $(SRC_DIR)/title/statusbar.c \
    $(SRC_DIR)/title/render_util.c \
    $(SRC_DIR)/title/battery_icon.c \
    $(SRC_DIR)/title/battery_fill.c \
    $(SRC_DIR)/title/battery_glow.c \
    $(SRC_DIR)/title/background.c \
    $(SRC_DIR)/title/fireflies.c \
    $(SRC_DIR)/title/modal.c
TITLE_OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(TITLE_SOURCES))
TITLE_TARGET  := $(BIN_DIR)/sdl2_title
TITLE_VERSION_HEADER := $(INC_DIR)/title/version.h

SPACE_SOURCES := \
    $(SRC_DIR)/space_bench/input.c \
    $(SRC_DIR)/space_bench/main.c \
    $(SRC_DIR)/space_bench/gl_effects.c \
    $(SRC_DIR)/space_bench/render/render_main.c \
    $(SRC_DIR)/space_bench/render/util.c \
    $(SRC_DIR)/space_bench/render/background.c \
    $(SRC_DIR)/space_bench/render/upgrades.c \
    $(SRC_DIR)/space_bench/render/projectiles.c \
    $(SRC_DIR)/space_bench/render/particles.c \
    $(SRC_DIR)/space_bench/render/enemies.c \
    $(SRC_DIR)/space_bench/render/drones.c \
    $(SRC_DIR)/space_bench/render/anomaly.c \
    $(SRC_DIR)/space_bench/render/player.c \
    $(SRC_DIR)/space_bench/render/game_over.c \
    $(SRC_DIR)/space_bench/state/state_main.c \
    $(SRC_DIR)/space_bench/state/util.c \
    $(SRC_DIR)/space_bench/state/upgrades.c \
    $(SRC_DIR)/space_bench/state/player.c \
    $(SRC_DIR)/space_bench/state/projectiles.c \
    $(SRC_DIR)/space_bench/state/anomaly.c \
    $(SRC_DIR)/space_bench/state/effects.c \
    $(SRC_DIR)/space_bench/state/spawn.c
SPACE_OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SPACE_SOURCES))
SPACE_TARGET  := $(BIN_DIR)/sdl2_space_bench

DOUBLE_SOURCES := \
    $(SRC_DIR)/double_buf/input.c \
    $(SRC_DIR)/double_buf/main.c \
    $(SRC_DIR)/double_buf/particles.c \
    $(SRC_DIR)/double_buf/render.c \
    $(SRC_DIR)/double_buf/state.c
DOUBLE_OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(DOUBLE_SOURCES))
DOUBLE_TARGET  := $(BIN_DIR)/sdl2_bench_double_buf

FILL_BENCH_SOURCES := \
    $(SRC_DIR)/fill_bench/input.c \
    $(SRC_DIR)/fill_bench/main.c \
    $(SRC_DIR)/fill_bench/render.c \
    $(SRC_DIR)/fill_bench/state.c
FILL_BENCH_OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(FILL_BENCH_SOURCES))
FILL_BENCH_TARGET  := $(BIN_DIR)/sdl2_fill_bench

TEXTURE_BENCH_SOURCES := \
    $(SRC_DIR)/texture_bench/input.c \
    $(SRC_DIR)/texture_bench/main.c \
    $(SRC_DIR)/texture_bench/render.c \
    $(SRC_DIR)/texture_bench/state.c
TEXTURE_BENCH_OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(TEXTURE_BENCH_SOURCES))
TEXTURE_BENCH_TARGET  := $(BIN_DIR)/sdl2_texture_bench

LINES_BENCH_SOURCES := \
    $(SRC_DIR)/lines_bench/input.c \
    $(SRC_DIR)/lines_bench/main.c \
    $(SRC_DIR)/lines_bench/render.c \
    $(SRC_DIR)/lines_bench/state.c
LINES_BENCH_OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(LINES_BENCH_SOURCES))
LINES_BENCH_TARGET  := $(BIN_DIR)/sdl2_lines_bench

GEOMETRY_BENCH_SOURCES := \
    $(SRC_DIR)/geometry_bench/input.c \
    $(SRC_DIR)/geometry_bench/main.c \
    $(SRC_DIR)/geometry_bench/render.c \
    $(SRC_DIR)/geometry_bench/state.c
GEOMETRY_BENCH_OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(GEOMETRY_BENCH_SOURCES))
GEOMETRY_BENCH_TARGET  := $(BIN_DIR)/sdl2_geometry_bench

SCALING_BENCH_SOURCES := \
    $(SRC_DIR)/scaling_bench/input.c \
    $(SRC_DIR)/scaling_bench/main.c \
    $(SRC_DIR)/scaling_bench/render.c \
    $(SRC_DIR)/scaling_bench/state.c
SCALING_BENCH_OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SCALING_BENCH_SOURCES))
SCALING_BENCH_TARGET  := $(BIN_DIR)/sdl2_scaling_bench

MEMORY_BENCH_SOURCES := \
    $(SRC_DIR)/memory_bench/input.c \
    $(SRC_DIR)/memory_bench/main.c \
    $(SRC_DIR)/memory_bench/render.c \
    $(SRC_DIR)/memory_bench/state.c
MEMORY_BENCH_OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(MEMORY_BENCH_SOURCES))
MEMORY_BENCH_TARGET  := $(BIN_DIR)/sdl2_memory_bench

PIXELS_BENCH_SOURCES := \
    $(SRC_DIR)/pixels_bench/input.c \
    $(SRC_DIR)/pixels_bench/main.c \
    $(SRC_DIR)/pixels_bench/render.c \
    $(SRC_DIR)/pixels_bench/state.c
PIXELS_BENCH_OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(PIXELS_BENCH_SOURCES))
PIXELS_BENCH_TARGET  := $(BIN_DIR)/sdl2_pixels_bench

RENDER_SOURCES := \
    $(SRC_DIR)/render_suite/input.c \
    $(SRC_DIR)/render_suite/main.c \
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

GL_FBO_EFFECTS_SOURCES := \
    $(SRC_DIR)/gl_fbo_effects/input.c \
    $(SRC_DIR)/gl_fbo_effects/main.c \
    $(SRC_DIR)/gl_fbo_effects/state.c \
    $(SRC_DIR)/gl_fbo_effects/scenes/effects.c
GL_FBO_EFFECTS_OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(GL_FBO_EFFECTS_SOURCES))
GL_FBO_EFFECTS_TARGET  := $(BIN_DIR)/sdl2_gl_fbo_effects

AUDIO_SOURCES := \
    $(SRC_DIR)/audio_bench/audio_device.c \
    $(SRC_DIR)/audio_bench/input.c \
    $(SRC_DIR)/audio_bench/main.c \
    $(SRC_DIR)/audio_bench/waveform.c
AUDIO_OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(AUDIO_SOURCES))
AUDIO_TARGET  := $(BIN_DIR)/sdl2_audio_bench

SPRITE_BENCH_SOURCES := \
    $(SRC_DIR)/sprite_bench/input.c \
    $(SRC_DIR)/sprite_bench/main.c \
    $(SRC_DIR)/sprite_bench/state.c
SPRITE_BENCH_OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SPRITE_BENCH_SOURCES))
SPRITE_BENCH_TARGET  := $(BIN_DIR)/sdl2_sprite_bench

GFX_BENCH_SOURCES := \
    $(SRC_DIR)/gfx_bench/input.c \
    $(SRC_DIR)/gfx_bench/main.c \
    $(SRC_DIR)/gfx_bench/state.c \
    $(SRC_DIR)/gfx_bench/scenes/aa_shapes.c \
    $(SRC_DIR)/gfx_bench/scenes/rounded_rects.c \
    $(SRC_DIR)/gfx_bench/scenes/polygons.c \
    $(SRC_DIR)/gfx_bench/scenes/bezier.c \
    $(SRC_DIR)/gfx_bench/scenes/thick_lines.c
GFX_BENCH_OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(GFX_BENCH_SOURCES))
GFX_BENCH_TARGET  := $(BIN_DIR)/sdl2_gfx_bench

OBJ_MODEL_LOADER_SOURCES := \
    $(SRC_DIR)/obj_model_loader/input.c \
    $(SRC_DIR)/obj_model_loader/main.c \
    $(SRC_DIR)/obj_model_loader/placeholder_model.c \
    $(SRC_DIR)/obj_model_loader/state.c
OBJ_MODEL_LOADER_OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(OBJ_MODEL_LOADER_SOURCES))
OBJ_MODEL_LOADER_TARGET  := $(BIN_DIR)/sdl2_obj_model_loader

MESSAGEBOX_PROBE_SOURCES := \
    $(SRC_DIR)/messagebox_probe/main.c
MESSAGEBOX_PROBE_OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(MESSAGEBOX_PROBE_SOURCES))
MESSAGEBOX_PROBE_TARGET  := $(BIN_DIR)/sdl2_messagebox_probe

ALL_OBJECTS   := $(COMMON_OBJECTS) \
                 $(TITLE_OBJECTS) \
                 $(SPACE_OBJECTS) \
                 $(DOUBLE_OBJECTS) \
                 $(FILL_BENCH_OBJECTS) \
                 $(TEXTURE_BENCH_OBJECTS) \
                 $(LINES_BENCH_OBJECTS) \
                 $(GEOMETRY_BENCH_OBJECTS) \
                 $(SCALING_BENCH_OBJECTS) \
                 $(MEMORY_BENCH_OBJECTS) \
                 $(PIXELS_BENCH_OBJECTS) \
                 $(RENDER_OBJECTS) \
                 $(GL_FBO_EFFECTS_OBJECTS) \
                 $(AUDIO_OBJECTS) \
                 $(SPRITE_BENCH_OBJECTS) \
                 $(GFX_BENCH_OBJECTS) \
                 $(OBJ_MODEL_LOADER_OBJECTS) \
                 $(MESSAGEBOX_PROBE_OBJECTS)
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

SDL_PREFIX         ?= /opt/mmiyoo-sdl2
SDL_ADDONS_PREFIX  ?= /opt/mmiyoo-sdl2-addons
SDL_INCLUDE        := $(SDL_PREFIX)/include
SDL_ADDONS_INCLUDE := $(SDL_ADDONS_PREFIX)/include
SDL_LIBDIR         := $(SDL_PREFIX)/lib
SDL_ADDONS_LIBDIR  := $(SDL_ADDONS_PREFIX)/lib

ARM_NEON_DEFINE := -D__ARM_NEON
ARM_CPU_FLAGS   := -mcpu=cortex-a7 -mfpu=neon -mfloat-abi=hard -ftree-vectorize -fomit-frame-pointer -fdata-sections -ffunction-sections

# Flags ----------------------------------------------------------------------
DEBUG        ?= 0
CFLAGS       ?= -O2
CFLAGS       := $(filter-out $(ARM_NEON_DEFINE),$(CFLAGS))
CFLAGS       += -std=c11 -Wall -Wextra -D_REENTRANT -DMMIYOO $(ARM_CPU_FLAGS)
ifeq ($(DEBUG),1)
# Must come after ARM_CPU_FLAGS to override its -fomit-frame-pointer.
CFLAGS       := $(filter-out -O2,$(CFLAGS)) -Og -g -fno-omit-frame-pointer -DDEBUG_BUILD
endif
CPPFLAGS     := $(filter-out $(ARM_NEON_DEFINE),$(CPPFLAGS))
CPPFLAGS     += $(SYSROOT_FLAG) -I$(SDL_INCLUDE) -I$(SDL_INCLUDE)/SDL2 -I$(SDL_ADDONS_INCLUDE) -I$(SYSROOT)/usr/include -I$(INC_DIR) -I$(SRC_DIR) $(ARM_NEON_DEFINE)
LDFLAGS      += $(SYSROOT_FLAG) -L$(SDL_LIBDIR) -L$(SDL_ADDONS_LIBDIR)
LDFLAGS      += $(ARM_CPU_FLAGS) -Wl,--gc-sections
LDLIBS       += -lSDL2 -lSDL2_ttf -lSDL2_gfx -lSDL2_image -lEGL -lGLESv2 -lneonarmmiyoo -lm -lpthread

.PHONY: all clean print-config test

all: $(TARGETS)

TITLE_GIT_VERSION ?= $(shell git -C $(CURDIR) describe --tags --always --dirty 2>/dev/null || echo unknown)

.PHONY: $(TITLE_VERSION_HEADER)
$(TITLE_VERSION_HEADER):
	@mkdir -p $(dir $@)
	@echo '#define TITLE_VERSION_STRING "$(TITLE_GIT_VERSION)"' > $@

$(TITLE_OBJECTS): $(TITLE_VERSION_HEADER)

$(TITLE_TARGET): $(COMMON_OBJECTS) $(TITLE_OBJECTS) | $(BIN_DIR)
	$(CC) $(COMMON_OBJECTS) $(TITLE_OBJECTS) $(LDFLAGS) $(LDLIBS) -o $@
	@echo "Built $@ successfully"

$(SPACE_TARGET): $(COMMON_OBJECTS) $(SPACE_OBJECTS) | $(BIN_DIR)
	$(CC) $(COMMON_OBJECTS) $(SPACE_OBJECTS) $(LDFLAGS) $(LDLIBS) -o $@
	@echo "Built $@ successfully"

$(DOUBLE_TARGET): $(COMMON_OBJECTS) $(DOUBLE_OBJECTS) | $(BIN_DIR)
	$(CC) $(COMMON_OBJECTS) $(DOUBLE_OBJECTS) $(LDFLAGS) $(LDLIBS) -o $@
	@echo "Built $@ successfully"

$(FILL_BENCH_TARGET): $(COMMON_OBJECTS) $(FILL_BENCH_OBJECTS) | $(BIN_DIR)
	$(CC) $(COMMON_OBJECTS) $(FILL_BENCH_OBJECTS) $(LDFLAGS) $(LDLIBS) -o $@
	@echo "Built $@ successfully"

$(TEXTURE_BENCH_TARGET): $(COMMON_OBJECTS) $(TEXTURE_BENCH_OBJECTS) | $(BIN_DIR)
	$(CC) $(COMMON_OBJECTS) $(TEXTURE_BENCH_OBJECTS) $(LDFLAGS) $(LDLIBS) -o $@
	@echo "Built $@ successfully"

$(LINES_BENCH_TARGET): $(COMMON_OBJECTS) $(LINES_BENCH_OBJECTS) | $(BIN_DIR)
	$(CC) $(COMMON_OBJECTS) $(LINES_BENCH_OBJECTS) $(LDFLAGS) $(LDLIBS) -o $@
	@echo "Built $@ successfully"

$(GEOMETRY_BENCH_TARGET): $(COMMON_OBJECTS) $(GEOMETRY_BENCH_OBJECTS) | $(BIN_DIR)
	$(CC) $(COMMON_OBJECTS) $(GEOMETRY_BENCH_OBJECTS) $(LDFLAGS) $(LDLIBS) -o $@
	@echo "Built $@ successfully"

$(SCALING_BENCH_TARGET): $(COMMON_OBJECTS) $(SCALING_BENCH_OBJECTS) | $(BIN_DIR)
	$(CC) $(COMMON_OBJECTS) $(SCALING_BENCH_OBJECTS) $(LDFLAGS) $(LDLIBS) -o $@
	@echo "Built $@ successfully"

$(MEMORY_BENCH_TARGET): $(COMMON_OBJECTS) $(MEMORY_BENCH_OBJECTS) | $(BIN_DIR)
	$(CC) $(COMMON_OBJECTS) $(MEMORY_BENCH_OBJECTS) $(LDFLAGS) $(LDLIBS) -o $@
	@echo "Built $@ successfully"

$(PIXELS_BENCH_TARGET): $(COMMON_OBJECTS) $(PIXELS_BENCH_OBJECTS) | $(BIN_DIR)
	$(CC) $(COMMON_OBJECTS) $(PIXELS_BENCH_OBJECTS) $(LDFLAGS) $(LDLIBS) -o $@
	@echo "Built $@ successfully"

$(RENDER_TARGET): $(COMMON_OBJECTS) $(RENDER_OBJECTS) | $(BIN_DIR)
	$(CC) $(COMMON_OBJECTS) $(RENDER_OBJECTS) $(LDFLAGS) $(LDLIBS) -o $@
	@echo "Built $@ successfully"

$(GL_FBO_EFFECTS_TARGET): $(COMMON_OBJECTS) $(GL_FBO_EFFECTS_OBJECTS) | $(BIN_DIR)
	$(CC) $(COMMON_OBJECTS) $(GL_FBO_EFFECTS_OBJECTS) $(LDFLAGS) $(LDLIBS) -o $@
	@echo "Built $@ successfully"

sdl2_render_suite: $(RENDER_TARGET)

sdl2_gl_fbo_effects: $(GL_FBO_EFFECTS_TARGET)
$(AUDIO_TARGET): $(COMMON_OBJECTS) $(AUDIO_OBJECTS) | $(BIN_DIR)
	$(CC) $(COMMON_OBJECTS) $(AUDIO_OBJECTS) $(LDFLAGS) $(LDLIBS) -o $@
	@echo "Built $@ successfully"

$(SPRITE_BENCH_TARGET): $(COMMON_OBJECTS) $(SPRITE_BENCH_OBJECTS) | $(BIN_DIR)
	$(CC) $(COMMON_OBJECTS) $(SPRITE_BENCH_OBJECTS) $(LDFLAGS) $(LDLIBS) -o $@
	@echo "Built $@ successfully"

$(GFX_BENCH_TARGET): $(COMMON_OBJECTS) $(GFX_BENCH_OBJECTS) | $(BIN_DIR)
	$(CC) $(COMMON_OBJECTS) $(GFX_BENCH_OBJECTS) $(LDFLAGS) $(LDLIBS) -o $@
	@echo "Built $@ successfully"

$(OBJ_MODEL_LOADER_TARGET): $(COMMON_OBJECTS) $(OBJ_MODEL_LOADER_OBJECTS) | $(BIN_DIR)
	$(CC) $(COMMON_OBJECTS) $(OBJ_MODEL_LOADER_OBJECTS) $(LDFLAGS) $(LDLIBS) -o $@
	@echo "Built $@ successfully"

$(MESSAGEBOX_PROBE_TARGET): $(COMMON_OBJECTS) $(MESSAGEBOX_PROBE_OBJECTS) | $(BIN_DIR)
	$(CC) $(COMMON_OBJECTS) $(MESSAGEBOX_PROBE_OBJECTS) $(LDFLAGS) $(LDLIBS) -o $@
	@echo "Built $@ successfully"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@


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
