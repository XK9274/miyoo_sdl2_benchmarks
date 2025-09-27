#!/bin/bash
# Miyoo SDL2 Benchmark Compilation Script
#
# This script provides two compilation modes:
# 1. Docker mode (default): Full toolchain setup with SDL2 compilation
# 2. Local mode (--local): Direct make compilation (requires pre-installed toolchain)
#
# Options:
#   --local     Use local toolchain instead of Docker
#   --verbose   Enable verbose output
#   --help      Show this help message

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Parse arguments
LOCAL_MODE=false
VERBOSE=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --local)
            LOCAL_MODE=true
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Compile SDL2 benchmarks for Miyoo Mini"
            echo ""
            echo "Options:"
            echo "  --local      Use local toolchain instead of Docker"
            echo "  -v, --verbose Enable verbose output"
            echo "  -h, --help   Show this help message"
            echo ""
            echo "Examples:"
            echo "  $0                    # Docker build (default)"
            echo "  $0 --verbose          # Docker build with verbose output"
            echo "  $0 --local            # Local build"
            echo "  $0 --local --verbose  # Local build with verbose output"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use -h or --help for usage information"
            exit 1
            ;;
    esac
done

# Check for local compilation
if [ "$LOCAL_MODE" = "true" ]; then
    echo "Building SDL2 benchmarks for Miyoo Mini (local mode)..."
    echo "WARNING: This requires the cross-compilation toolchain to be already set up"
    echo ""

    # Build all benchmark binaries locally
    make clean
    make

    # Check if build succeeded
    if [ $? -ne 0 ]; then
        echo "Build failed! Please check the output above."
        echo "Consider using Docker mode: ./compile.sh"
        exit 1
    fi

    echo "Build successful!"

    # Copy compiled binaries to distribution directory
    echo "Copying binaries to app-dist/sdl_bench/bin/..."
    mkdir -p app-dist/sdl_bench/bin
    cp build/bin/* app-dist/sdl_bench/bin/

    echo ""
    echo "=========================================="
    echo "INSTALLATION INSTRUCTIONS"
    echo "=========================================="
    echo ""
    echo "To install on your Miyoo Mini:"
    echo "1. Copy the 'sdl_bench' directory to:"
    echo "   /mnt/SDCARD/App/sdl_bench"
    echo ""
    echo "2. Restart MainUI or reboot your device"
    echo ""
    echo "3. Navigate to Apps and run SDL Benchmark"
    echo ""
    echo "The app-dist/sdl_bench directory is ready for deployment!"
    echo "=========================================="

else
    # Default: Use Docker compilation pipeline
    echo "🚀 Starting Docker compilation pipeline..."
    if [ "$VERBOSE" = "false" ]; then
        echo "💡 Use './compile.sh --verbose' for detailed output"
    fi
    echo ""

    # Run the Docker orchestration script with verbose flag
    if [ "$VERBOSE" = "true" ]; then
        exec "$SCRIPT_DIR/docker-compile.sh" --verbose
    else
        exec "$SCRIPT_DIR/docker-compile.sh"
    fi
fi
