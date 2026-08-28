#!/bin/bash

# SDL2 Benchmarks Docker Compilation Pipeline
# Automates the complete toolchain setup -> SDL2 compilation -> benchmark building process

set -e  # Exit on any error

# Parse command line arguments
VERBOSE=false
DEBUG=false
while [[ $# -gt 0 ]]; do
    case $1 in
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -d|--debug)
            DEBUG=true
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo "Options:"
            echo "  -v, --verbose    Enable verbose output"
            echo "  -d, --debug      Build benchmarks with -g -Og -fno-omit-frame-pointer (make DEBUG=1), for gdbserver"
            echo "  -h, --help       Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use -h or --help for usage information"
            exit 1
            ;;
    esac
done

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
UTILITY_DIR="${SCRIPT_DIR}/utility"
TOOLCHAIN_DIR="${SCRIPT_DIR}/union-miyoomini-toolchain"
WORKSPACE_DIR="${TOOLCHAIN_DIR}/workspace"

EXPECTED_BINARIES=(
    sdl2_title
    sdl2_bench_double_buf
    sdl2_space_bench
    sdl2_render_suite
    sdl2_gl_fbo_effects
    sdl2_audio_bench
    sdl2_sprite_bench
)

echo "=========================================="
echo "SDL2 Benchmarks Docker Build Pipeline"
echo "=========================================="

# Step 1: Clone toolchain if needed
if [ ! -d "$TOOLCHAIN_DIR" ]; then
    echo "Cloning union-miyoomini-toolchain..."
    git clone https://github.com/XK9274/union-miyoomini-toolchain.git "$TOOLCHAIN_DIR"
    if [ $? -ne 0 ]; then
        echo "ERROR: Failed to clone toolchain repository"
        exit 1
    fi
    echo "Toolchain cloned successfully"
else
    echo "Toolchain already exists at: $TOOLCHAIN_DIR"
fi

# Step 2: Copy scripts to workspace
echo "Copying scripts to Docker workspace..."
mkdir -p "$WORKSPACE_DIR"
"$UTILITY_DIR/prepare-neon.sh" "$WORKSPACE_DIR/neon-arm-library"
"$UTILITY_DIR/prepare-sdl2-miyoo.sh" "$WORKSPACE_DIR/sdl2_miyoo"
cp -f "$UTILITY_DIR/mksdl2.sh" "$WORKSPACE_DIR/"
cp -f "$UTILITY_DIR/compile.sh" "$WORKSPACE_DIR/"
cp -f "$SCRIPT_DIR/Makefile" "$WORKSPACE_DIR/"
cp -rf "$SCRIPT_DIR/src" "$WORKSPACE_DIR/"
cp -rf "$SCRIPT_DIR/include" "$WORKSPACE_DIR/"
if [ -d "$SCRIPT_DIR/build_artifacts" ]; then
    mkdir -p "$WORKSPACE_DIR/build_artifacts"
    cp -a "$SCRIPT_DIR/build_artifacts/." "$WORKSPACE_DIR/build_artifacts/"
fi
# GLES libs for compile-time linking come straight from the sdl2_miyoo checkout
# (vendored at its repo root, not built) -- available as soon as it's cloned.
mkdir -p "$WORKSPACE_DIR/build_artifacts/gles_libs"
cp -a "$WORKSPACE_DIR/sdl2_miyoo/libEGL.so" "$WORKSPACE_DIR/sdl2_miyoo/libGLESv2.so" "$WORKSPACE_DIR/build_artifacts/gles_libs/"
echo "Scripts and source files copied to workspace"

# Step 3: Make scripts executable
chmod +x "$WORKSPACE_DIR/mksdl2.sh"
chmod +x "$WORKSPACE_DIR/compile.sh"
echo "Scripts made executable"

# Step 4: Build Docker container and run SDL2 compilation
echo ""
echo "Building Docker toolchain and compiling SDL2 libraries..."
echo "This will take several minutes..."

# Preflight: Ensure Docker is installed and available in PATH
if ! command -v docker >/dev/null 2>&1; then
    echo "ERROR: Docker is not installed or not available in your PATH."
    echo "Please install Docker Engine and ensure the 'docker' command works for your user."
    echo "Installation guide: https://docs.docker.com/engine/install/"
    exit 1
fi

cd "$TOOLCHAIN_DIR"

# Build Docker toolchain using the official Makefile
if [ "$VERBOSE" = "true" ]; then
    make .build || echo "Docker image already built"
else
    make .build 2>/dev/null || echo "Docker image ready"
fi

# Set verbose environment for scripts inside Docker
verbose_env=""
if [ "$VERBOSE" = "true" ]; then
    verbose_env="VERBOSE=true"
fi

make_debug_arg=""
if [ "$DEBUG" = "true" ]; then
    make_debug_arg="DEBUG=1"
    echo "Debug build requested: benchmarks will be built with $make_debug_arg (symbols, no frame-pointer omission)"
fi

echo "Running compilation inside Docker container..."

# Run Docker with automatic SDL2 compilation
docker_cmd="
    cd /root/workspace/build_source
    export $verbose_env

    # Container runs as root; everything it writes into this bind mount would
    # otherwise be left root-owned on the host. Reset ownership back to the
    # invoking user on exit, success or failure, so re-runs never need sudo.
    trap 'chown -R \"\$HOST_UID:\$HOST_GID\" /root/workspace/build_source 2>/dev/null || true' EXIT

    echo 'Compiling SDL2 libraries...'
    ./mksdl2.sh

    echo ''
    echo 'IMPORTANT: Compiled SDL2 libraries are for COMPILE-TIME ONLY'
    echo '   Do NOT copy these libraries to the Miyoo device for runtime'
    echo '   The device has its own SDL2 runtime libraries'
    echo ''

    echo 'Compiling SDL2 benchmarks...'
    if [ \"$VERBOSE\" = \"true\" ]; then
        make clean && make $make_debug_arg
    else
        make clean > /dev/null 2>&1 && make $make_debug_arg > /dev/null 2>&1
    fi

    if [ \$? -eq 0 ]; then
        echo 'Benchmark compilation successful!'
        if [ \"$VERBOSE\" = \"true\" ]; then
            ls -la build/bin/
        fi
    else
        echo 'ERROR: Benchmark compilation failed'
        exit 1
    fi

    echo 'Extracting runtime libSDL2_ttf/libz from the toolchain sysroot...'
    mkdir -p runtime_libs
    ttf_lib=/opt/miyoomini-toolchain/usr/arm-linux-gnueabihf/sysroot/usr/lib/libSDL2_ttf-2.0.so.0
    if [ -f \"\$ttf_lib\" ]; then
        cp -L \"\$ttf_lib\" runtime_libs/
    else
        echo 'ERROR: libSDL2_ttf-2.0.so.0 not found in toolchain sysroot'
        exit 1
    fi
    libz_path=\$(find /opt/miyoomini-toolchain -name 'libz.so.1' 2>/dev/null | head -1)
    if [ -n \"\$libz_path\" ]; then
        cp -L \"\$libz_path\" runtime_libs/
    else
        echo 'ERROR: libz.so.1 not found under /opt/miyoomini-toolchain'
        exit 1
    fi
"

# Release builds provide the resolved tag explicitly. Development builds retain
# the descriptive Git fallback, including commit and dirty-worktree details.
TITLE_GIT_VERSION="${TITLE_GIT_VERSION:-$(git -C "$SCRIPT_DIR" describe --tags --always --dirty 2>/dev/null || echo unknown)}"

docker run --rm \
    -e TITLE_GIT_VERSION="$TITLE_GIT_VERSION" \
    -e HOST_UID="$(id -u)" \
    -e HOST_GID="$(id -g)" \
    -v "$WORKSPACE_DIR":/root/workspace/build_source \
    miyoomini-toolchain bash -c "$docker_cmd"

if [ $? -ne 0 ]; then
    echo "ERROR: Docker compilation failed"
    exit 1
fi

echo "Docker compilation completed successfully"

# Sync cached SDL2 artifacts back to the project directory for reuse
if [ -d "$WORKSPACE_DIR/build_artifacts" ]; then
    mkdir -p "$SCRIPT_DIR/build_artifacts"
    cp -a "$WORKSPACE_DIR/build_artifacts/." "$SCRIPT_DIR/build_artifacts/"
    echo "SDL2 build artifacts updated"
fi

# Step 4b: Build the MMIYOO SDL2 runtime driver (libSDL2-2.0.so.0 + the neon
# helper it builds as a sub-step). Separate docker invocation, same toolchain
# image, mirroring how mk_miyoo.sh --docker already runs standalone.
echo ""
echo "Building sdl2_miyoo runtime driver..."
(cd "$WORKSPACE_DIR/sdl2_miyoo" && ./build-scripts/mk_miyoo.sh --docker --enable-gles --clean build)

# Step 5: Populate the runtime lib/ directory. Nothing here is committed to
# git -- every file is produced fresh by this script or the sdl2_miyoo build.
echo ""
echo "Populating runtime libraries..."
sdl_output="$WORKSPACE_DIR/sdl2_miyoo/output/libSDL2-2.0.so.0"
neon_lib="$WORKSPACE_DIR/sdl2_miyoo/libneonarmmiyoo.so"
for required in "$sdl_output" "$neon_lib"; do
    if [ ! -f "$required" ]; then
        echo "ERROR: expected sdl2_miyoo build output missing: $required"
        exit 1
    fi
done

for binary in "${EXPECTED_BINARIES[@]}"; do
    if [ ! -f "$WORKSPACE_DIR/build/bin/$binary" ]; then
        echo "ERROR: expected benchmark binary missing: $WORKSPACE_DIR/build/bin/$binary"
        exit 1
    fi
done

# Replace generated package outputs so removed or renamed artifacts cannot
# survive into a later package. Preserve the tracked directory placeholder.
mkdir -p "$SCRIPT_DIR/app-dist/sdl_bench/bin" "$SCRIPT_DIR/app-dist/sdl_bench/lib"
find "$SCRIPT_DIR/app-dist/sdl_bench/bin" -mindepth 1 -maxdepth 1 -exec rm -rf -- {} +
find "$SCRIPT_DIR/app-dist/sdl_bench/lib" -mindepth 1 -maxdepth 1 ! -name '.gitkeep' -exec rm -rf -- {} +

cp -f "$sdl_output" "$SCRIPT_DIR/app-dist/sdl_bench/lib/libSDL2-2.0.so.0"
cp -f "$neon_lib" "$SCRIPT_DIR/app-dist/sdl_bench/lib/libneonarmmiyoo.so"
cp -f "$WORKSPACE_DIR/sdl2_miyoo/libEGL.so" "$SCRIPT_DIR/app-dist/sdl_bench/lib/libEGL.so"
cp -f "$WORKSPACE_DIR/sdl2_miyoo/libGLESv2.so" "$SCRIPT_DIR/app-dist/sdl_bench/lib/libGLESv2.so"
cp -f "$WORKSPACE_DIR/runtime_libs/libSDL2_ttf-2.0.so.0" "$SCRIPT_DIR/app-dist/sdl_bench/lib/libSDL2_ttf-2.0.so.0"
cp -f "$WORKSPACE_DIR/runtime_libs/libz.so.1" "$SCRIPT_DIR/app-dist/sdl_bench/lib/libz.so.1"
echo "Runtime libraries populated:"
ls -la "$SCRIPT_DIR/app-dist/sdl_bench/lib/"

# Step 6: Copy compiled binaries to distribution directory
echo "Copying compiled binaries to distribution directory..."
mkdir -p "$SCRIPT_DIR/app-dist/sdl_bench/bin"

if [ -d "$WORKSPACE_DIR/build/bin" ]; then
    for binary in "${EXPECTED_BINARIES[@]}"; do
        cp -f "$WORKSPACE_DIR/build/bin/$binary" "$SCRIPT_DIR/app-dist/sdl_bench/bin/"
    done
    echo "Binaries copied to: $SCRIPT_DIR/app-dist/sdl_bench/bin/"

    # List the compiled binaries
    echo ""
    echo "Compiled binaries:"
    ls -la "$SCRIPT_DIR/app-dist/sdl_bench/bin/"
else
    echo "ERROR: No binaries found in workspace build directory"
    exit 1
fi

# Step 7: Cleanup workspace files
echo ""
echo "Cleaning up workspace..."
rm -rf "$WORKSPACE_DIR/SDL2-*"
rm -rf "$WORKSPACE_DIR/logs"
rm -f "$WORKSPACE_DIR"/*.tar.gz
echo "Workspace cleaned"

echo ""
echo "=========================================="
echo "BUILD COMPLETE!"
echo "=========================================="
echo ""
echo "Installation instructions:"
echo "1. Copy the entire 'app-dist/sdl_bench' directory to your Miyoo Mini SD card:"
echo "   /mnt/SDCARD/App/sdl_bench"
echo ""
echo "2. Restart MainUI or reboot your device"
echo ""
echo "3. Navigate to Apps and run 'SDL Benchmark'"
echo ""
echo "The app-dist/sdl_bench directory is ready for deployment!"
echo "=========================================="
