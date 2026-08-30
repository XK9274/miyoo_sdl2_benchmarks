#!/bin/sh
bench_dir=$(dirname "$0")

export HOME=$bench_dir
export PATH=$bench_dir:$PATH
export LD_LIBRARY_PATH=$bench_dir/lib:$LD_LIBRARY_PATH

# Left unset intentionally: auto-detect selects mmiyoo via a real hardware probe. Uncomment to force a specific backend.
# export SDL_VIDEODRIVER=mmiyoo
# export SDL_AUDIODRIVER=mmiyoo
# export EGL_VIDEODRIVER=mmiyoo

# SDL_MMIYOO_VSYNC_MODE: off (no wait), adaptive (skip wait if running late),
# or strict (hard 30fps steps under load -- more consistent pacing but can
# flicker). Title screen's config panel sets this per suite; uncomment to
# force it for the title screen too.
# export SDL_MMIYOO_VSYNC_MODE=strict

cpuclock="/mnt/SDCARD/.tmp_update/bin/cpuclock"

# MainUI's App/ folder convention doesn't track or kill a previously launched
# process, so re-tapping the icon can spawn a second process tree that fights
# the first over the driver's singleton joystick/framebuffer resources.
my_pid=$$
for stale_pid in $(pgrep -f "bin/sdl2_" 2>/dev/null); do
    if [ "$stale_pid" != "$my_pid" ]; then
        kill -9 "$stale_pid" 2>/dev/null
    fi
done

if [ -f /mnt/SDCARD/.tmp_update/script/stop_audioserver.sh ]; then
    /mnt/SDCARD/.tmp_update/script/stop_audioserver.sh
else
    killall audioserver audioserver.mod 2>/dev/null
fi

exe_cpuclock() {
    if [ -f "$cpuclock" ]; then
        "$cpuclock" 1700
    else
        echo "Warning: cpuclock not found at $cpuclock"
    fi
}

cd "$bench_dir"

# Launches a single benchmark directly under gdbserver so gdb attaches
# before the process runs, instead of racing a fast crash.
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

# Isolated single-scene A/B perf run, tagged fps logging. No env vars other
# than these are set, so vsync mode etc. stays at the binary's default.
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

# Usage: ./launch.sh --obj-model <tag> [duration_s]
#   e.g. ./launch.sh --obj-model perf1 30
if [ "$1" = "--obj-model" ]; then
    obj_tag="${2:-untagged}"
    obj_duration="${3:-30}"
    obj_log="$bench_dir/logs/obj_model_loader_${obj_tag}.log"

    if [ -z "$2" ]; then
        echo "Usage: $0 --obj-model <tag> [duration_s]"
        exit 1
    fi

    mkdir -p "$bench_dir/logs"
    echo "Running obj_model_loader stage-timing benchmark, tag=$obj_tag duration=${obj_duration}s"
    echo "===== START obj-model ($obj_tag): $(date) =====" > "$obj_log"
    ps >> "$obj_log"
    echo "-----" >> "$obj_log"
    OBJ_BENCH_DURATION_S="$obj_duration" OBJ_BENCH_TAG="$obj_tag" \
        "bin/sdl2_obj_model_loader" >> "$obj_log" 2>&1
    obj_exit=$?
    echo "-----" >> "$obj_log"
    ps >> "$obj_log"
    echo "===== END obj-model ($obj_tag): $(date) exit=$obj_exit =====" >> "$obj_log"
    echo "Done. Log: $obj_log"
    exit $obj_exit
fi

echo "Starting SDL2 Demo Suites title screen..."
echo "Directory: $bench_dir"

exe_cpuclock

if [ ! -f "bin/sdl2_title" ]; then
    echo "Error: bin/sdl2_title not found"
    exit 1
fi

# Suites launched from the title menu inherit this process's stdio with no
# separate per-suite log capture, so route the whole session to one log file.
session_log="$bench_dir/logs/session_$(date +%Y%m%d_%H%M%S).log"
mkdir -p "$bench_dir/logs"

# Direct-writes untextured triangle fill spans and lines into the mapped
# framebuffer instead of dispatching one hardware call per span/line. Off by
# default in the driver.
export SDL_MMIYOO_GEOMETRY_DIRECT_WRITE=1

bin/sdl2_title >> "$session_log" 2>&1

if [ -f /mnt/SDCARD/.tmp_update/script/start_audioserver.sh ]; then
    echo "Restarting audio services..."
    /mnt/SDCARD/.tmp_update/script/start_audioserver.sh
fi