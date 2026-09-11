SDL Benchmark (Miyoo Mini)

Repository
- Source: https://github.com/XK9274/miyoo_sdl2_benchmarks

Overview
This package contains SDL2 performance benchmarks for the Miyoo Mini handheld. It measures rendering, audio, OpenGL ES, and SDL2 backend behaviour using several small test apps.

What’s inside this folder (sdl_bench/)
- bin/
  Contains the title screen and benchmark executables:
  • sdl2_title                       – Title screen: select a suite, configure resolution/vsync/frame limit/input mode, launch it
  • sdl2_bench_double_buf            – Combined shape/particle/fill stress scene
  • sdl2_fill_bench                  – Solid fill-rect throughput
  • sdl2_texture_bench               – Texture blit throughput
  • sdl2_lines_bench                 – Line & shape drawing throughput
  • sdl2_geometry_bench              – Rotating NEON-projected mesh with particle trails
  • sdl2_scaling_bench               – Resolution/render-target scaling
  • sdl2_memory_bench                – Dynamic texture allocation/lifetime churn
  • sdl2_pixels_bench                – CPU pixel-buffer effects (plasma/fire/mandelbrot/cellular)
  • sdl2_gl_fbo_effects              – 15 offscreen GL FBO shader effects, read back and composited via the 2D renderer
  • sdl2_audio_bench                 – Audio device/sample/buffer tests
  • sdl2_space_bench                 – Space Bench: space shooter with metrics
  • sdl2_sprite_bench                – Fullscreen morphing sprite stress test, no overlay, no vsync

- lib/
  Required runtime libraries for the benchmarks (SDL2 and friends).

- assets/
  Shared assets used by the tests (e.g., textures and audio samples).

- config.json
  App configuration used by the launcher.

- launch.sh
  Launch script used by MainUI to start the benchmarks.

- logs/
  Only used by the --geometry diagnostic mode below; not written during normal use.

Install/Run
- Copy the entire sdl_bench/ directory to your Miyoo Mini at: /mnt/SDCARD/App/
- Restart MainUI or reboot, then open Apps → “SDL Benchmark”.
- A normal launch opens the "SDL2 Demo Suites" title screen: pick a suite from the
  list, adjust Resolution / VSync Mode / Frame Limit / Input Mode in the config
  panel, then press A to launch it. Exiting a suite returns to the title screen;
  the title screen's own Exit quits back to MainUI. Each suite still has its own
  in-app controls (stress levels, etc.) once running.
- Diagnostic modes are available from the shell:
  ./launch.sh --gdb <binary-name> [port]
  ./launch.sh --geometry <tag> [duration_s]

Notes
- Built for the Miyoo Mini through the shared mm-buildbot SDL/toolchain
  providers.
- For source code, build instructions, and updates, see the repository above.


Former Render Suite, now 7 Standalone Benches:

1. sdl2_fill_bench - Color fills, gradients, alpha blending
2. sdl2_lines_bench - Rapid line rendering, patterns
3. sdl2_texture_bench - Streaming, scaling, rotation
4. sdl2_geometry_bench - 3D meshes, tessellation, wireframes
5. sdl2_scaling_bench - Multi-resolution performance tests
6. sdl2_memory_bench - Dynamic texture allocation stress testing
7. sdl2_pixels_bench - Direct pixel manipulation effects

GL FBO Effects - 15 Effect Modes:

Sunrise Gradient, Soft Waves, Scanline Glow, Floating Orbs, Aurora Borealis,
Nebula Clouds, Fire Effect, Lightning Storm, Crystal Cavern, Plasma Flow,
Electric Grid, Ocean Depths, Retro Sun, Digital Rain, Chromatic Shift

Audio Benchmark - 4 Visualization Modes:

- Bars (rect) - Traditional bar visualization
- Curves (line) - Smooth waveform curves
- Dots (rect) - Particle-style visualization
- Ribbons (geom) - Advanced geometry rendering

Space Bench - Space Shooter Game:

- Space combat simulation with performance tracking
- Player controls, projectile systems, enemy drones
- Anomaly effects, upgrade mechanics, background rendering
- Real-time metrics overlay (draw calls, vertices, triangles)

Sprite Bench - Fullscreen Sprite Stress Test:

- Bouncing, morphing multicolour 16x16 sprites, no vsync, no overlay
- Auto-ramps sprite count over time; Up/Down step, Left/Right interval, A/B grow/shrink
- X toggles Static/Dynamic texture mode; SELECT exits
- Used to isolate render-pipeline flicker independent of vsync/overlay/HUD code
