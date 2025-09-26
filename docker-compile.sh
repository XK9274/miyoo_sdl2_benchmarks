#!/bin/bash

# SDL2 Benchmarks Docker Compilation Pipeline
# Automates the complete toolchain setup -> SDL2 compilation -> benchmark building process

set -e  # Exit on any error

# Parse command line arguments
VERBOSE=false
while [[ $# -gt 0 ]]; do
    case $1 in
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo "Options:"
            echo "  -v, --verbose    Enable verbose output"
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
TOOLCHAIN_DIR="${SCRIPT_DIR}/union-miyoomini-toolchain"
WORKSPACE_DIR="${TOOLCHAIN_DIR}/workspace"

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
    echo "✓ Toolchain cloned successfully"
else
    echo "✓ Toolchain already exists at: $TOOLCHAIN_DIR"
fi

# Step 2: Copy scripts to workspace
echo "Copying scripts to Docker workspace..."
mkdir -p "$WORKSPACE_DIR"
cp -f "$SCRIPT_DIR/mksdl2.sh" "$WORKSPACE_DIR/"
cp -f "$SCRIPT_DIR/compile.sh" "$WORKSPACE_DIR/"
cp -f "$SCRIPT_DIR/Makefile" "$WORKSPACE_DIR/"
cp -rf "$SCRIPT_DIR/src" "$WORKSPACE_DIR/"
cp -rf "$SCRIPT_DIR/include" "$WORKSPACE_DIR/"
if [ -d "$SCRIPT_DIR/build_artifacts" ]; then
    mkdir -p "$WORKSPACE_DIR/build_artifacts"
    cp -a "$SCRIPT_DIR/build_artifacts/." "$WORKSPACE_DIR/build_artifacts/"
fi
echo "✓ Scripts and source files copied to workspace"

# Step 3: Make scripts executable
chmod +x "$WORKSPACE_DIR/mksdl2.sh"
chmod +x "$WORKSPACE_DIR/compile.sh"
echo "✓ Scripts made executable"

# Step 4: Build Docker container and run SDL2 compilation
echo ""
echo "Building Docker toolchain and compiling SDL2 libraries..."
echo "This will take several minutes..."
cd "$TOOLCHAIN_DIR"

# Build Docker toolchain using the official Makefile
if [ "$VERBOSE" = "true" ]; then
    make .build || echo "Docker image already built"
else
    make .build 2>/dev/null || echo "✓ Docker image ready"
fi

# Set verbose environment for scripts inside Docker
verbose_env=""
if [ "$VERBOSE" = "true" ]; then
    verbose_env="VERBOSE=true"
fi

echo "🐳 Running compilation inside Docker container..."

# Run Docker with automatic SDL2 compilation
docker_cmd="
    cd /root/workspace/build_source
    export $verbose_env

    echo '📦 Compiling SDL2 libraries...'
    ./mksdl2.sh

    echo ''
    echo '⚠️  IMPORTANT: Compiled SDL2 libraries are for COMPILE-TIME ONLY'
    echo '   Do NOT copy these libraries to the Miyoo device for runtime'
    echo '   The device has its own SDL2 runtime libraries'
    echo ''

    echo '🎯 Compiling SDL2 benchmarks...'
    if [ \"$VERBOSE\" = \"true\" ]; then
        make clean && make
    else
        make clean > /dev/null 2>&1 && make > /dev/null 2>&1
    fi

    if [ \$? -eq 0 ]; then
        echo '✅ Benchmark compilation successful!'
        if [ \"$VERBOSE\" = \"true\" ]; then
            ls -la build/bin/
        fi
    else
        echo '❌ ERROR: Benchmark compilation failed'
        exit 1
    fi
"

docker run --rm -v "$WORKSPACE_DIR":/root/workspace/build_source miyoomini-toolchain bash -c "$docker_cmd"

if [ $? -ne 0 ]; then
    echo "ERROR: Docker compilation failed"
    exit 1
fi

echo "✓ Docker compilation completed successfully"

# Sync cached SDL2 artifacts back to the project directory for reuse
if [ -d "$WORKSPACE_DIR/build_artifacts" ]; then
    mkdir -p "$SCRIPT_DIR/build_artifacts"
    cp -a "$WORKSPACE_DIR/build_artifacts/." "$SCRIPT_DIR/build_artifacts/"
    echo "✓ SDL2 build artifacts updated"
fi

# Step 5: Copy compiled binaries to distribution directory
echo "Copying compiled binaries to distribution directory..."
mkdir -p "$SCRIPT_DIR/app-dist/sdl_bench/bin"

if [ -d "$WORKSPACE_DIR/build/bin" ]; then
    cp -f "$WORKSPACE_DIR/build/bin"/* "$SCRIPT_DIR/app-dist/sdl_bench/bin/"
    echo "✓ Binaries copied to: $SCRIPT_DIR/app-dist/sdl_bench/bin/"

    # List the compiled binaries
    echo ""
    echo "Compiled binaries:"
    ls -la "$SCRIPT_DIR/app-dist/sdl_bench/bin/"
else
    echo "ERROR: No binaries found in workspace build directory"
    exit 1
fi

# Step 6: Cleanup workspace files
echo ""
echo "Cleaning up workspace..."
rm -rf "$WORKSPACE_DIR/SDL2-*"
rm -rf "$WORKSPACE_DIR/logs"
rm -f "$WORKSPACE_DIR"/*.tar.gz
echo "✓ Workspace cleaned"

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
