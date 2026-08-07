#!/bin/sh
bench_dir=$(dirname "$0")

export HOME=$bench_dir
export PATH=$bench_dir:$PATH
export LD_LIBRARY_PATH=$bench_dir/lib:$LD_LIBRARY_PATH
export SDL_VIDEODRIVER=mmiyoo
export SDL_AUDIODRIVER=mmiyoo
export EGL_VIDEODRIVER=mmiyoo

freemma="/mnt/SDCARD/.tmp_update/bin/freemma"
cpuclock="/mnt/SDCARD/.tmp_update/bin/cpuclock"

# Stop audio services
if [ -f /mnt/SDCARD/.tmp_update/script/stop_audioserver.sh ]; then
    /mnt/SDCARD/.tmp_update/script/stop_audioserver.sh
else
    killall audioserver audioserver.mod 2>/dev/null
fi

# Execute freemma if available
# The current SDL2 doesn't correctly release MI_SYS resources, causing OOM and screen flip on exit
# Not actually needed now as backend is fixed.
exe_freemma() {
    if [ -f "$freemma" ]; then
        echo "Running memory cleanup..."
        "$freemma"
        sleep 0.1
    else
        echo "Warning: freemma not found at $freemma"
    fi
}

exe_cpuclock() {
    "$cpuclock 1700"
}

# Function to run benchmark with error checking
run_benchmark() {
    local bench_name="$1"
    local bench_path="$2"
    
    echo "========================================="
    echo "Running $bench_name..."
    echo "========================================="
    
    if [ -f "$bench_path" ]; then
        "$bench_path"
        local exit_code=$?
        if [ $exit_code -eq 0 ]; then
            echo "$bench_name completed successfully"
        else
            echo "Warning: $bench_name exited with code $exit_code"
        fi
    else
        echo "Error: $bench_path not found"
        return 1
    fi
    
    # exe_freemma # Not needed now, fixed the SDL backend 
}

cd "$bench_dir"

# TEMP DIAGNOSTIC (universal segfault investigation): launch a single
# benchmark directly under gdbserver instead of running the full suite, so
# gdb attaches before the process runs rather than racing a fast crash.
# Usage: ./launch.sh --gdb <bin-name-under-bin/> [port]
#   e.g. ./launch.sh --gdb sdl2_render_suite 2345
# On the host: gdb-multiarch -> target remote <device-ip>:<port>
if [ "$1" = "--gdb" ]; then
    gdbserver_bin="/mnt/SDCARD/.tmp_update/bin/gdbserver"
    gdb_bench_name="$2"
    gdb_port="${3:-2345}"
    gdb_bench_path="bin/$gdb_bench_name"

    if [ -z "$gdb_bench_name" ]; then
        echo "Usage: $0 --gdb <bin-name-under-bin/> [port]"
        exit 1
    fi
    if [ ! -f "$gdbserver_bin" ]; then
        echo "Error: gdbserver not found at $gdbserver_bin"
        exit 1
    fi
    if [ ! -f "$gdb_bench_path" ]; then
        echo "Error: $gdb_bench_path not found"
        exit 1
    fi

    echo "Launching $gdb_bench_path under gdbserver on :$gdb_port"
    echo "On host: gdb-multiarch, then 'target remote <device-ip>:$gdb_port'"
    exec "$gdbserver_bin" ":$gdb_port" "$gdb_bench_path"
fi

echo "Starting SDL2 benchmark suite..."
echo "Directory: $bench_dir"

# Uncomment to enable debug output to UART
# export SDL_MMIYOO_DEBUG=1

# Run benchmarks
exe_cpuclock
run_benchmark "SDL2 Render Suite" "bin/sdl2_render_suite"

# TEMP DIAGNOSTIC (GL boot-failure investigation): capture render_suite_gl's
# stdout/stderr + MMIYOO renderer debug logging to a file on the SD card,
# since MainUI swallows console output otherwise. Remove this block and
# restore the plain run_benchmark call once the failure is root-caused.
echo "Running SDL2 Render Suite GL (debug capture to gl_debug.log)..."
SDL_MMIYOO_DEBUG=1 SDL_MMIYOO_DEBUG_VERBOSE=1 run_benchmark "SDL2 Render Suite GL" "bin/sdl2_render_suite_gl" > "$bench_dir/gl_debug.log" 2>&1
echo "SDL2 Render Suite GL run finished, see gl_debug.log for exit code and output"

run_benchmark "SDL2 Software Double Buffer Benchmark" "bin/sdl2_bench_software_double_buf"
run_benchmark "SDL2 Double Buffer Benchmark" "bin/sdl2_bench_double_buf"
run_benchmark "SDL2 Interactive Demo" "bin/sdl2_space_bench"
run_benchmark "SDL2 Audio Benchmark" "bin/sdl2_audio_bench"


echo "========================================="
echo "All benchmarks completed!"
echo "========================================="

exe_freemma

if [ -f /mnt/SDCARD/.tmp_update/script/start_audioserver.sh ]; then
    echo "Restarting audio services..."
    /mnt/SDCARD/.tmp_update/script/start_audioserver.sh
fi