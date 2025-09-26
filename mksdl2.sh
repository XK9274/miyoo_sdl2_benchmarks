#!/bin/bash
# SDL2 Cross-compilation Script for Miyoo Mini
# Streamlined version with verbose control

# Verbose mode (default: quiet)
VERBOSE=${VERBOSE:-false}

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ARTIFACT_DIR="$SCRIPT_DIR/build_artifacts"
ARTIFACT_LIB_DIR="$ARTIFACT_DIR/sdl2_libs"
ARTIFACT_INCLUDE_DIR="$ARTIFACT_DIR/sdl2_headers"

log_output() {
    if [ "$VERBOSE" = "true" ]; then
        cat
    else
        cat > /dev/null
    fi
}

status_msg() {
    echo -e "\033[32m$1\033[0m"
}

check_dev_tools() {
    if [ "$VERBOSE" = "true" ]; then
        status_msg "Checking development tools..."
    fi

    declare -A tool_to_package_map=(
        ["pkg-config"]="pkg-config"
        ["autoconf"]="autoconf"
        ["libtoolize"]="libtool"
        ["m4"]="m4"
        ["automake"]="automake"
    )

    missing_packages=()
    for tool in "${!tool_to_package_map[@]}"; do
        if ! command -v "$tool" > /dev/null 2>&1; then
            package=${tool_to_package_map[$tool]}
            if ! [[ " ${missing_packages[@]} " =~ " ${package} " ]]; then
                missing_packages+=("$package")
            fi
        fi
    done

    if [ ${#missing_packages[@]} -ne 0 ]; then
        status_msg "Installing missing build tools..."
        if [ "$VERBOSE" = "true" ]; then
            apt-get update
            apt-get install -y "${missing_packages[@]}"
        else
            apt-get update >/dev/null 2>&1
            DEBIAN_FRONTEND=noninteractive apt-get install -y "${missing_packages[@]}" >/dev/null 2>&1
        fi
    fi
}

# Setup toolchain environment (matches setup-env.sh from union-miyoomini-toolchain)
export PATH="/opt/miyoomini-toolchain/usr/bin:${PATH}:/opt/miyoomini-toolchain/usr/arm-linux-gnueabihf/sysroot/bin"
export CROSS_COMPILE=/opt/miyoomini-toolchain/usr/bin/arm-linux-gnueabihf-
export PREFIX=/opt/miyoomini-toolchain/usr/arm-linux-gnueabihf/sysroot/usr

export FIN_BIN_DIR="$PREFIX"
export AR=${CROSS_COMPILE}ar
export AS=${CROSS_COMPILE}as
export LD=${CROSS_COMPILE}ld
export RANLIB=${CROSS_COMPILE}ranlib
export CC=${CROSS_COMPILE}gcc
export NM=${CROSS_COMPILE}nm
export HOST=arm-linux-gnueabihf
export BUILD=x86_64-linux-gnu
export CFLAGS="-Wno-undef -Os -marm -mtune=cortex-a7 -mfpu=neon-vfpv4 -march=armv7ve+simd -mfloat-abi=hard -ffunction-sections -fdata-sections"
export CXXFLAGS="-s -O3 -fPIC -pthread"
export LDFLAGS="-L/opt/miyoomini-toolchain/usr/arm-linux-gnueabihf/sysroot/lib -L/opt/miyoomini-toolchain/usr/arm-linux-gnueabihf/sysroot/usr/lib"

# PKG_CONFIG setup for SDL2 extension libraries to find SDL2
export PKG_CONFIG_PATH="$FIN_BIN_DIR/lib/pkgconfig:$PKG_CONFIG_PATH"
export PKG_CONFIG_LIBDIR="$FIN_BIN_DIR/lib/pkgconfig"

mkdir -p "$ARTIFACT_LIB_DIR" "$ARTIFACT_INCLUDE_DIR"

artifacts_available() {
    [ -f "$ARTIFACT_LIB_DIR/libSDL2.a" ] && \
    [ -f "$ARTIFACT_LIB_DIR/libSDL2_ttf.a" ] && \
    [ -d "$ARTIFACT_INCLUDE_DIR/SDL2" ]
}

install_artifacts_into_sysroot() {
    status_msg "Installing cached SDL2 artifacts into toolchain sysroot..."
    mkdir -p "$FIN_BIN_DIR/lib" "$FIN_BIN_DIR/lib/pkgconfig" "$FIN_BIN_DIR/include"

    # Refresh headers
    if [ -d "$FIN_BIN_DIR/include/SDL2" ]; then
        rm -rf "$FIN_BIN_DIR/include/SDL2"
    fi
    if [ -d "$ARTIFACT_INCLUDE_DIR/SDL2" ]; then
        cp -a "$ARTIFACT_INCLUDE_DIR/SDL2" "$FIN_BIN_DIR/include/"
    fi

    # Copy libraries and pkg-config files
    for artifact in "$ARTIFACT_LIB_DIR"/*; do
        [ -e "$artifact" ] || continue
        base_name=$(basename "$artifact")
        case "$base_name" in
            *.pc)
                cp -a "$artifact" "$FIN_BIN_DIR/lib/pkgconfig/$base_name"
                ;;
            *)
                cp -a "$artifact" "$FIN_BIN_DIR/lib/$base_name"
                ;;
        esac
    done

    status_msg "Cached SDL2 artifacts installed"
}

if artifacts_available; then
    install_artifacts_into_sysroot
    exit 0
fi

check_dev_tools

# Setup logging
mkdir -p ./logs

cd ~/workspace/

# SDL2 source packages to download and compile
declare -A sdl_packages=(
    ["SDL2-2.26.5.tar.gz"]="https://github.com/libsdl-org/SDL/releases/download/release-2.26.5/SDL2-2.26.5.tar.gz"
    ["SDL2_ttf-2.20.2.tar.gz"]="https://github.com/libsdl-org/SDL_ttf/releases/download/release-2.20.2/SDL2_ttf-2.20.2.tar.gz"
    # ["SDL2_net-2.2.0.tar.gz"]="https://github.com/libsdl-org/SDL_net/releases/download/release-2.2.0/SDL2_net-2.2.0.tar.gz"
    ["SDL2_mixer-2.6.3.tar.gz"]="https://github.com/libsdl-org/SDL_mixer/releases/download/release-2.6.3/SDL2_mixer-2.6.3.tar.gz"
)

# Download required packages
status_msg "Downloading SDL2 packages..."
download_pids=()
for file in "${!sdl_packages[@]}"; do
    if [ ! -f "$file" ]; then
        if [ "$VERBOSE" = "true" ]; then
            echo "Downloading $file..."
        fi
        wget -q "${sdl_packages[$file]}" &
        download_pids+=($!)
    fi
done

# Wait for all downloads to complete
for pid in "${download_pids[@]}"; do
    wait $pid
done


# Check if library is already installed
is_library_installed() {
    local lib_name="$1"
    if [ -f "$FIN_BIN_DIR/lib/pkgconfig/$lib_name.pc" ]; then
        return 0
    fi
    return 1
}

# Compile function for SDL2 packages
compile_package() {
    local package_file="$1"
    local package_name="$2"
    local package_dir="$3"
    local extra_configure_flags="$4"
    local lib_check="$5"

    # Check if already installed
    if is_library_installed "$lib_check"; then
        status_msg "$package_name already installed - skipping"
        return 0
    fi

    status_msg "Compiling $package_name"

    # Extract source
    if [ ! -d "$package_dir" ]; then
        tar -xf "$package_file"
        if [ $? -ne 0 ]; then
            echo "❌ ERROR: Failed to extract $package_file"
            exit 1
        fi
    fi

    cd "$package_dir"

    # Run autogen
    echo "  • Running autogen..."
    if [ "$VERBOSE" = "true" ]; then
        ./autogen.sh
    else
        ./autogen.sh | log_output 2>&1
    fi
    if [ $? -ne 0 ]; then
        echo "❌ ERROR: autogen failed for $package_name"
        exit 1
    fi

    # Configure
    echo "  • Configuring..."
    if [ "$VERBOSE" = "true" ]; then
        ./configure CC=$CC --host=$HOST --build=$BUILD --prefix=$FIN_BIN_DIR $extra_configure_flags
    else
        ./configure CC=$CC --host=$HOST --build=$BUILD --prefix=$FIN_BIN_DIR $extra_configure_flags >/dev/null 2>&1
    fi
    if [ $? -ne 0 ]; then
        echo "❌ ERROR: Configure failed for $package_name"
        echo "Check that dependencies are properly installed"
        exit 1
    fi

    # Build
    echo "  • Building..."
    if [ "$VERBOSE" = "true" ]; then
        make clean && make -j$(( $(nproc) - 1 ))
    else
        make clean | log_output 2>&1 && make -j$(( $(nproc) - 1 )) | log_output 2>&1
    fi
    if [ $? -ne 0 ]; then
        echo "❌ ERROR: Build failed for $package_name"
        exit 1
    fi

    # Install
    echo "  • Installing..."
    if [ "$VERBOSE" = "true" ]; then
        make install
    else
        make install | log_output 2>&1
    fi
    if [ $? -ne 0 ]; then
        echo "❌ ERROR: Install failed for $package_name"
        exit 1
    fi

    # Verify installation
    if ! is_library_installed "$lib_check"; then
        echo "❌ ERROR: $package_name installation verification failed"
        echo "Expected $FIN_BIN_DIR/lib/pkgconfig/$lib_check.pc"
        ls -la "$FIN_BIN_DIR/lib/pkgconfig/" | head -10
        exit 1
    fi

    echo "✅ $package_name compiled and installed successfully"
    cd ..
}

# Compile SDL2 packages in order with library checks
status_msg "Starting SDL2 compilation sequence..."

compile_package "SDL2-2.26.5.tar.gz" "SDL2" "SDL2-2.26.5" "--disable-joystick-virtual --disable-sensor --disable-power --disable-video-x11 --disable-video-wayland --disable-video-vulkan --disable-dbus --disable-ime --disable-fcitx --disable-hidapi --disable-pulseaudio --disable-sndio --disable-libudev --disable-jack --disable-video-opengl --disable-video-opengles --disable-video-opengles2" "sdl2"

# Check SDL2 installation before continuing with extensions
if ! is_library_installed "sdl2"; then
    echo "❌ CRITICAL ERROR: SDL2 base library is not properly installed"
    echo "Cannot continue with SDL2 extensions"
    exit 1
fi

# Add sdl2-config to PATH for extension libraries
export PATH="$FIN_BIN_DIR/bin:$PATH"

compile_package "SDL2_ttf-2.20.2.tar.gz" "SDL2_TTF" "SDL2_ttf-2.20.2" "" "SDL2_ttf"

# compile_package "SDL2_net-2.2.0.tar.gz" "SDL2_NET" "SDL2_net-2.2.0" "" "SDL2_net"

compile_package "SDL2_mixer-2.6.3.tar.gz" "SDL2_MIXER" "SDL2_mixer-2.6.3" "" "SDL2_mixer"

status_msg "✅ All SDL2 libraries compiled successfully!"

status_msg "Caching SDL2 artifacts for future builds..."

persist_artifacts() {
    mkdir -p "$ARTIFACT_LIB_DIR" "$ARTIFACT_INCLUDE_DIR"

    # Clear cached copies so old versions do not linger
    find "$ARTIFACT_LIB_DIR" -maxdepth 1 -type f \( -name 'libSDL2*' -o -name 'SDL2*.pc' -o -name 'sdl2*.pc' \) -delete 2>/dev/null || true

    # Copy library files
    for lib_path in "$FIN_BIN_DIR/lib"/libSDL2* "$FIN_BIN_DIR/lib"/SDL2_* "$FIN_BIN_DIR/lib"/SDL2*; do
        [ -e "$lib_path" ] || continue
        cp -a "$lib_path" "$ARTIFACT_LIB_DIR/"
    done

    # Copy pkg-config files
    if [ -d "$FIN_BIN_DIR/lib/pkgconfig" ]; then
        for pc_file in "$FIN_BIN_DIR/lib/pkgconfig"/SDL2*.pc "$FIN_BIN_DIR/lib/pkgconfig"/sdl2*.pc; do
            [ -e "$pc_file" ] || continue
            cp -a "$pc_file" "$ARTIFACT_LIB_DIR/"
        done
    fi

    # Refresh headers
    if [ -d "$FIN_BIN_DIR/include/SDL2" ]; then
        rm -rf "$ARTIFACT_INCLUDE_DIR/SDL2"
        cp -a "$FIN_BIN_DIR/include/SDL2" "$ARTIFACT_INCLUDE_DIR/"
    fi
}

persist_artifacts
status_msg "Cached SDL2 artifacts updated"

# Display installation summary
echo ""
echo "📋 Installation Summary:"
echo "SDL2 Headers: $FIN_BIN_DIR/include/SDL2/"
echo "SDL2 Libraries: $FIN_BIN_DIR/lib/"
echo "PKG Config: $FIN_BIN_DIR/lib/pkgconfig/"
echo ""
ls -la "$FIN_BIN_DIR/lib/pkgconfig/"*.pc 2>/dev/null || echo "No .pc files found"
