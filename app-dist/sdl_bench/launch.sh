#!/bin/sh
bench_dir=$(dirname "$0")

export HOME=$bench_dir
export PATH=$bench_dir:$PATH
export LD_LIBRARY_PATH=$bench_dir/lib:$LD_LIBRARY_PATH

# Left unset intentionally: auto-detect selects mmiyoo via a real hardware probe. Uncomment to force a specific backend.
# export SDL_VIDEODRIVER=mmiyoo
# export SDL_AUDIODRIVER=mmiyoo
# export EGL_VIDEODRIVER=mmiyoo

# SDL_MMIYOO_VSYNC_MODE: "off" (no wait), "adaptive" (FBIO_WAITFORVSYNC, skipped if running late), "strict" (real FBIOPAN_DISPLAY panning paced by /dev/l; hard-steps to 30fps under load, killing /dev/l regains control but introduces flickering).
# The title screen's config panel sets this per launched suite; uncomment to force a value for the title screen itself too.
# export SDL_MMIYOO_VSYNC_MODE=strict

freemma="/mnt/SDCARD/.tmp_update/bin/freemma"
cpuclock="/mnt/SDCARD/.tmp_update/bin/cpuclock"

# MainUI's App/ folder convention doesn't track or kill a previously launched
# process -- re-tapping the icon (e.g. after backing out via the system
# Menu/Home button instead of this app's own Exit control) spawns a second
# full process tree that fights the first over the MMIYOO driver's singleton
# joystick/framebuffer resources. Clear out any stale instance first so every
# launch starts clean.
my_pid=$$
for stale_pid in $(pgrep -f "bin/sdl2_" 2>/dev/null); do
    if [ "$stale_pid" != "$my_pid" ]; then
        kill -9 "$stale_pid" 2>/dev/null
    fi
done

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
    if [ -f "$cpuclock" ]; then
        "$cpuclock" 1700
    else
        echo "Warning: cpuclock not found at $cpuclock"
    fi
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

# Isolated single-scene A/B perf run: forces one render_suite scene, disables
# auto-cycle, runs for a fixed duration, and logs periodic [BENCH] fps lines
# tagged for comparison across builds. No env vars other than the ones below
# are set, so vsync mode etc. stays at the binary's own default.
# Usage: ./launch.sh --geometry <tag> [duration_s]
#   e.g. ./launch.sh --geometry neon 30
if [ "$1" = "--geometry" ]; then
    geo_tag="${2:-untagged}"
    geo_duration="${3:-30}"
    geo_log="$bench_dir/logs/render_suite_geometry_${geo_tag}.log"

    if [ -z "$2" ]; then
        echo "Usage: $0 --geometry <tag> [duration_s]"
        exit 1
    fi

    mkdir -p "$bench_dir/logs"
    echo "Running geometry-scene-only benchmark, tag=$geo_tag duration=${geo_duration}s"
    echo "===== START geometry ($geo_tag): $(date) =====" > "$geo_log"
    ps >> "$geo_log"
    echo "-----" >> "$geo_log"
    RS_FORCE_SCENE=geometry RS_BENCH_DURATION_S="$geo_duration" RS_BENCH_TAG="$geo_tag" \
        "bin/sdl2_render_suite" >> "$geo_log" 2>&1
    geo_exit=$?
    echo "-----" >> "$geo_log"
    ps >> "$geo_log"
    echo "===== END geometry ($geo_tag): $(date) exit=$geo_exit =====" >> "$geo_log"
    echo "Done. Log: $geo_log"
    exit $geo_exit
fi

echo "Starting SDL2 Demo Suites title screen..."
echo "Directory: $bench_dir"

exe_cpuclock

if [ ! -f "bin/sdl2_title" ]; then
    echo "Error: bin/sdl2_title not found"
    exit 1
fi

bin/sdl2_title

if [ -f /mnt/SDCARD/.tmp_update/script/start_audioserver.sh ]; then
    echo "Restarting audio services..."
    /mnt/SDCARD/.tmp_update/script/start_audioserver.sh
fi