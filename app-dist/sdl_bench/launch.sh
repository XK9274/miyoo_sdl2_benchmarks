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
    
    exe_freemma
}

cd "$bench_dir"

echo "Starting SDL2 benchmark suite..."
echo "Directory: $bench_dir"

# Uncomment to enable debug output to UART
# export SDL_MMIYOO_DEBUG=1

# Run benchmarks
exe_cpuclock
run_benchmark "SDL2 Render Suite" "bin/sdl2_render_suite"
run_benchmark "SDL2 Software Double Buffer Benchmark" "bin/sdl2_bench_software_double_buf"
run_benchmark "SDL2 Double Buffer Benchmark" "bin/sdl2_bench_double_buf"
run_benchmark "SDL2 Audio Benchmark" "bin/sdl2_audio_bench"
# run_benchmark "SDL2 Interactive Demo" "bin/sdl2_spaceship_game"

echo "========================================="
echo "All benchmarks completed!"
echo "========================================="

exe_freemma

if [ -f /mnt/SDCARD/.tmp_update/script/start_audioserver.sh ]; then
    echo "Restarting audio services..."
    /mnt/SDCARD/.tmp_update/script/start_audioserver.sh
fi