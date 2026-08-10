#!/bin/sh
bench_dir=$(dirname "$0")

export HOME=$bench_dir
export PATH=$bench_dir:$PATH
export LD_LIBRARY_PATH=$bench_dir/lib:$LD_LIBRARY_PATH
export SDL_VIDEODRIVER=mmiyoo
export SDL_AUDIODRIVER=mmiyoo
export EGL_VIDEODRIVER=mmiyoo

# SDL_MMIYOO_VSYNC_MODE controls present pacing: "off" (no wait), "adaptive"
# (default -- our own FBIO_WAITFORVSYNC, skipped if already running late),
# or "strict" -- real FBIOPAN_DISPLAY panning paced by /dev/l.
#
# /dev/l in the Miyoo firmware controls double buffering and MI_DISP
# interaction. It can pan for you, but when /dev/l handles it, you're
# forced into "strict mode" vsync, where you get 60fps but whenever load
# is too high, you're instantly forced to 30fps. You can kill /dev/l to
# control this behaviour, but it will introduce flickering.
#
# Below, every suite runs three times back-to-back -- off, adaptive,
# strict -- so the three logs sit next to each other for direct comparison.
# export SDL_MMIYOO_VSYNC_MODE=strict

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

# Function to run benchmark with error checking. Captures stdout/stderr
# (including MMIYOO_LOG_WARN/ERROR/DEBUG driver logging) to its own file
# under logs/, since MainUI/interactive launch swallows console output.
run_benchmark() {
    local bench_name="$1"
    local bench_path="$2"
    local log_name="$3"
    local present_mode="$4"
    local log_file="$bench_dir/logs/$log_name.log"

    echo "========================================="
    echo "Running $bench_name..."
    echo "========================================="

    if [ -f "$bench_path" ]; then
        echo "===== START $bench_name: $(date) =====" > "$log_file"
        ps >> "$log_file"
        echo "-----" >> "$log_file"
        case "$present_mode" in
            off)
                SDL_MMIYOO_VSYNC_MODE=off SDL_MMIYOO_DEBUG=1 SDL_MMIYOO_DEBUG_VERBOSE=1 "$bench_path" >> "$log_file" 2>&1
                ;;
            strict)
                SDL_MMIYOO_VSYNC_MODE=strict SDL_MMIYOO_DEBUG=1 SDL_MMIYOO_DEBUG_VERBOSE=1 "$bench_path" >> "$log_file" 2>&1
                ;;
            *)
                SDL_MMIYOO_VSYNC_MODE=adaptive SDL_MMIYOO_DEBUG=1 SDL_MMIYOO_DEBUG_VERBOSE=1 "$bench_path" >> "$log_file" 2>&1
                ;;
        esac
        local exit_code=$?
        echo "-----" >> "$log_file"
        ps >> "$log_file"
        echo "===== END $bench_name: $(date) exit=$exit_code =====" >> "$log_file"
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

mkdir -p "$bench_dir/logs"

# Run benchmarks (each captures its own log under logs/, see run_benchmark).
# Each suite runs three times back-to-back: off, adaptive, strict, so the
# three logs sit next to each other for a direct FPS comparison.
exe_cpuclock
run_benchmark "SDL2 Sprite Bench (off)"      "bin/sdl2_sprite_bench" "sdl2_sprite_bench_off" off
run_benchmark "SDL2 Sprite Bench (adaptive)" "bin/sdl2_sprite_bench" "sdl2_sprite_bench_adaptive" adaptive
run_benchmark "SDL2 Sprite Bench (strict)"   "bin/sdl2_sprite_bench" "sdl2_sprite_bench_strict" strict

run_benchmark "SDL2 Render Suite (off)"      "bin/sdl2_render_suite" "sdl2_render_suite_off" off
run_benchmark "SDL2 Render Suite (adaptive)" "bin/sdl2_render_suite" "sdl2_render_suite_adaptive" adaptive
run_benchmark "SDL2 Render Suite (strict)"   "bin/sdl2_render_suite" "sdl2_render_suite_strict" strict

run_benchmark "SDL2 Render Suite GL (off)"      "bin/sdl2_render_suite_gl" "sdl2_render_suite_gl_off" off
run_benchmark "SDL2 Render Suite GL (adaptive)" "bin/sdl2_render_suite_gl" "sdl2_render_suite_gl_adaptive" adaptive
run_benchmark "SDL2 Render Suite GL (strict)"   "bin/sdl2_render_suite_gl" "sdl2_render_suite_gl_strict" strict

run_benchmark "SDL2 Software Double Buffer Benchmark (off)"      "bin/sdl2_bench_software_double_buf" "sdl2_bench_software_double_buf_off" off
run_benchmark "SDL2 Software Double Buffer Benchmark (adaptive)" "bin/sdl2_bench_software_double_buf" "sdl2_bench_software_double_buf_adaptive" adaptive
run_benchmark "SDL2 Software Double Buffer Benchmark (strict)"   "bin/sdl2_bench_software_double_buf" "sdl2_bench_software_double_buf_strict" strict

run_benchmark "SDL2 Double Buffer Benchmark (off)"      "bin/sdl2_bench_double_buf" "sdl2_bench_double_buf_off" off
run_benchmark "SDL2 Double Buffer Benchmark (adaptive)" "bin/sdl2_bench_double_buf" "sdl2_bench_double_buf_adaptive" adaptive
run_benchmark "SDL2 Double Buffer Benchmark (strict)"   "bin/sdl2_bench_double_buf" "sdl2_bench_double_buf_strict" strict

run_benchmark "SDL2 Interactive Demo (off)"      "bin/sdl2_space_bench" "sdl2_space_bench_off" off
run_benchmark "SDL2 Interactive Demo (adaptive)" "bin/sdl2_space_bench" "sdl2_space_bench_adaptive" adaptive
run_benchmark "SDL2 Interactive Demo (strict)"   "bin/sdl2_space_bench" "sdl2_space_bench_strict" strict

run_benchmark "SDL2 Audio Benchmark (off)"      "bin/sdl2_audio_bench" "sdl2_audio_bench_off" off
run_benchmark "SDL2 Audio Benchmark (adaptive)" "bin/sdl2_audio_bench" "sdl2_audio_bench_adaptive" adaptive
run_benchmark "SDL2 Audio Benchmark (strict)"   "bin/sdl2_audio_bench" "sdl2_audio_bench_strict" strict


echo "========================================="
echo "All benchmarks completed!"
echo "========================================="

# exe_freemma

if [ -f /mnt/SDCARD/.tmp_update/script/start_audioserver.sh ]; then
    echo "Restarting audio services..."
    /mnt/SDCARD/.tmp_update/script/start_audioserver.sh
fi