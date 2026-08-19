SDL Benchmark (Miyoo Mini)

Repository
- Source: https://github.com/XK9274/miyoo_sdl2_benchmarks

Overview
This package contains SDL2 performance benchmarks for the Miyoo Mini handheld. It measures rendering and audio performance using several small test apps built with SDL2/SDL2_ttf/SDL2_mixer.

What’s inside this folder (sdl_bench/)
- bin/
  Contains the benchmark executables:
  • sdl2_bench_software_double_buf  – Software backbuffer rendering test
  • sdl2_bench_double_buf            – Hardware double buffering test (mmiyoo backend)
  • sdl2_render_suite                – Comprehensive rendering suite (fills, lines, textures)
  • sdl2_gl_fbo_effects              – 15 offscreen GL FBO shader effects, read back and composited via the 2D renderer
  • sdl2_audio_bench                 – Audio device/sample/buffer tests
  • space_bench                     – Star Wing space shooter with metrics
  • sdl2_sprite_bench                 – Fullscreen morphing sprite stress test, no overlay, no vsync

- lib/
  Required runtime libraries for the benchmarks (SDL2 and friends).

- assets/
  Shared assets used by the tests (e.g., textures and audio samples).

- config.json
  App configuration used by the launcher.

- launch.sh
  Launch script used by MainUI to start the benchmarks.

Install/Run
- Copy the entire sdl_bench/ directory to your Miyoo Mini at: /mnt/SDCARD/App/
- Restart MainUI or reboot, then open Apps → “SDL Benchmark”.

Notes
- Built for the Miyoo Mini using the union-miyoomini-toolchain via Docker.
- For source code, build instructions, and updates, see the repository above.


Render Suite - 7 Test Scenes:

1. Fill Operations - Color fills, gradients, alpha blending
2. Line Drawing - Rapid line rendering, patterns
3. Texture Operations - Streaming, scaling, rotation
4. Geometric Complexity - 3D meshes, tessellation, wireframes
5. Resolution Scaling - Multi-resolution performance tests
6. Memory Management - Dynamic texture allocation stress testing
7. Pixel Operations - Direct pixel manipulation effects

GL FBO Effects - 15 Effect Modes:

Sunrise Gradient, Soft Waves, Scanline Glow, Floating Orbs, Aurora Borealis,
Nebula Clouds, Fire Effect, Lightning Storm, Crystal Cavern, Plasma Flow,
Electric Grid, Ocean Depths, Retro Sun, Digital Rain, Chromatic Shift

Audio Benchmark - 4 Visualization Modes:

- Bars (rect) - Traditional bar visualization
- Curves (line) - Smooth waveform curves
- Dots (rect) - Particle-style visualization
- Ribbons (geom) - Advanced geometry rendering

Star Wing Bench - Space Shooter Game:

- Space combat simulation with performance tracking
- Player controls, projectile systems, enemy drones
- Anomaly effects, upgrade mechanics, background rendering
- Real-time metrics overlay (draw calls, vertices, triangles)

Sprite Bench - Fullscreen Sprite Stress Test:

- Bouncing, morphing multicolour 16x16 sprites, no vsync, no overlay
- Auto-ramps sprite count over time; Up/Down step, Left/Right interval, A/B grow/shrink
- X toggles Static/Dynamic texture mode; SELECT exits
- Used to isolate render-pipeline flicker independent of vsync/overlay/HUD code
