# Miyoo SDL2/OpenGL Benchmarks

SDL2 benchmark programs for the Miyoo Mini. The project is used to exercise
rendering, audio, OpenGL ES, and SDL2 backend behaviour against a custom SDL2
build for the device.

The generated package is written to `app-dist/sdl_bench/`.

> **AI disclosure:** there's been a substantial usage of various LLM in this
> project to both write the code & maintain the repo itself.

## Screenshots

<div align="center">

<table>
  <tr>
    <td align="center"><img src="assets/sdl_bench_030.png" width="300"></td>
    <td align="center"><img src="assets/sdl_bench_029.png" width="300"></td>
  </tr>
  <tr>
    <td align="center"><img src="assets/sdl_bench_028.png" width="300"></td>
    <td align="center"><img src="assets/sdl_bench_022.png" width="300"></td>
  </tr>
  <tr>
    <td align="center"><img src="assets/sdl_bench_010.png" width="300"></td>
    <td align="center"><img src="assets/sdl_bench_008.png" width="300"></td>
  </tr>
  <tr>
    <td align="center"><img src="assets/sdl_bench_007.png" width="300"></td>
    <td align="center"><img src="assets/sdl_bench_006.png" width="300"></td>
  </tr>
  <tr>
    <td align="center"><img src="assets/sdl_bench_033.png" width="300"></td>
    <td align="center"><img src="assets/sdl_bench_032.png" width="300"></td>
  </tr>
  <tr>
    <td align="center"><img src="assets/sdl_bench_031.png" width="300"></td>
    <td align="center"></td>
  </tr>
</table>

</div>

## Requirements

- Docker, for the default build path.
- Git, used by the Docker build script when the toolchain checkout is missing.
- A Miyoo Mini SD card or deployment target for running the packaged app.

The root build script automatically uses `union-miyoomini-toolchain/`. If that
directory does not exist, it clones the toolchain repository.

## Build

Build all benchmark binaries and prepare the distribution package:

```bash
./docker-compile.sh
```

Use verbose output when diagnosing build issues:

```bash
./docker-compile.sh --verbose
```

The build process:

- prepares the Miyoo Mini Docker toolchain;
- installs or reuses cached SDL2 build artifacts for compile-time linking;
- compiles the benchmark binaries with the project `Makefile`;
- copies the resulting ARM binaries into `app-dist/sdl_bench/bin/`;
- leaves `app-dist/sdl_bench/` ready to copy to the device.

Helper scripts live in `utility/`. The root `docker-compile.sh` script remains
the main entry point.

## Local Build

`utility/compile.sh --local` can be used when the Miyoo Mini cross-compilation
toolchain is already installed locally and available at the paths expected by
the `Makefile`.

```bash
./utility/compile.sh --local
```

The Docker build is the supported default because it sets up the toolchain and
SDL2 compile-time dependencies consistently.

## Installation

Copy the generated app directory to the Miyoo Mini SD card:

```text
app-dist/sdl_bench/ -> /mnt/SDCARD/App/sdl_bench/
```

Then restart MainUI or reboot the device. The app appears under Apps as
`SDL Benchmark`.

When launched normally, `app-dist/sdl_bench/launch.sh` starts `sdl2_title`,
the themed launcher/title screen, from which you pick and configure a suite
to run. Each suite run writes its own log under `app-dist/sdl_bench/logs/`.

The launcher also has two diagnostic entry points:

```bash
./launch.sh --gdb <binary-name> [port]
./launch.sh --geometry <tag> [duration_s]
```

`--gdb` starts one binary under `gdbserver`. `--geometry` runs the render
suite's geometry scene only and tags its periodic FPS output for A/B testing.

## Benchmarks

- `sdl2_title`
  - Themed launcher/title screen shown when the app starts.
  - Lets you pick a suite, configure resolution/vsync/frame-limit/input, and
    launches the selected binary; shows live battery/backend status.

- `sdl2_bench_double_buf`
  - Exercises hardware double buffering in the SDL2 Miyoo backend.
  - Uses MI_GFX and MI_SYS backed presentation paths.
  - Renders particle and cube-style geometry workloads.

- `sdl2_render_suite`
  - Runs a broader 2D rendering workload set.
  - Includes fill, line, texture, geometry, scaling, memory, and pixel scenes.

- `sdl2_gl_fbo_effects`
  - Renders 15 shader-based effects offscreen into a hidden window's FBO and
    reads the pixels back with `glReadPixels`, composited via the ordinary 2D
    `SDL_Renderer`. Does not exercise a real on-screen GL swap chain.

- `sdl2_audio_bench`
  - Exercises SDL2 audio device setup and buffer behaviour.
  - Includes waveform and sample playback related checks.

- `sdl2_space_bench`
  - Runs an interactive space shooter style benchmark.
  - Exercises sprites, particles, projectiles, drones, anomalies, GL effects,
    overlay rendering, and runtime metrics.

- `sdl2_sprite_bench`
  - Fullscreen bouncing/morphing sprite stress test, deliberately without an
    overlay/HUD or vsync, to isolate the render/present path from the
    overlay code every other suite carries.

## Build Flow

```text
./docker-compile.sh
  -> ensure union-miyoomini-toolchain exists
  -> copy source, Makefile, utility scripts, and cached artifacts to workspace
  -> build or reuse the Docker toolchain image
  -> run utility/mksdl2.sh inside the container as mksdl2.sh
  -> run make inside the container
  -> copy build/bin/* to app-dist/sdl_bench/bin/
```

SDL2 libraries built by `mksdl2.sh` are for compile-time linking in the
toolchain sysroot. The runtime package uses the libraries already present under
`app-dist/sdl_bench/lib/`.

## Directory Layout

```text
miyoo_sdl2_benchmarks/
|-- docker-compile.sh              # Root Docker build entry point
|-- Makefile                       # Benchmark build configuration
|-- README.md
|-- TODO.md
|-- app-dist/
|   `-- sdl_bench/                 # Device package
|       |-- assets/                # Runtime assets
|       |-- bin/                   # Built benchmark binaries
|       |-- lib/                   # Runtime libraries bundled with the app
|       |-- logs/                  # Runtime-generated benchmark logs
|       |-- config.json
|       `-- launch.sh
|-- assets/                        # README screenshots
|-- build/                         # Local/generated build output
|-- build_artifacts/               # Cached compile-time artifacts
|-- include/                       # Shared headers and GLES headers
|-- neon-arm-library/              # NEON helper library
|-- src/
|   |-- audio_bench/
|   |-- common/
|   |-- double_buf/
|   |-- gl_fbo_effects/
|   |-- render_suite/
|   |-- space_bench/
|   |-- sprite_bench/
|   `-- title/                     # Launcher/title screen (sdl2_title)
|-- union-miyoomini-toolchain/      # Auto-cloned Docker toolchain checkout
`-- utility/
    |-- compile.sh                 # Optional local/Docker helper wrapper
    `-- mksdl2.sh                  # SDL2 compile-time library builder
```

## Asset Credits

- `app-dist/sdl_bench/assets/bgm.wav`: [Audio Test](https://pixabay.com/music/video-games-arcade-beat-323176/)
  by [NoCopyrightSound633](https://pixabay.com/users/nocopyrightsound633-47610058/).
- `app-dist/sdl_bench/assets/ThaleahFat.ttf`: [Free Pixel Font Thaleah](https://tinyworlds.itch.io/free-pixel-font-thaleah)
  by [Tiny Worlds](https://tinyworlds.itch.io/).
- `app-dist/sdl_bench/icon.png`: [Pixel Skill Icons Pack - 250 Warrior Abilities](https://batareya.itch.io/pixel-skill-icons-pack-250-warrior-abilities)
  by [batareya](https://batareya.itch.io/).
