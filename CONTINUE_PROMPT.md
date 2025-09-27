# Space Bench Anomaly System Fixes Required

## Current Issues to Fix:

### 1. Multiple Anomalies Spawning
- **Problem**: More than one anomaly is spawning instead of just one
- **Fix**: Ensure only one anomaly exists at a time, debug spawn logic

### 2. Anomaly Visual Design
- **Problem**: Current 5x5 rectangles for weapon points look stupid
- **Fix**: Replace with orbital system:
  - Use 1x1 pixel points circling around anomaly center
  - Create spiraling pattern with randomized orbital points
  - Give each orbital point different velocities
  - Make them look like they're naturally orbiting the anomaly core

### 3. Laser System Completely Broken
- **Problem**: Current tracking lasers look fucking ridiculous and move around
- **Fix**: Redesign laser system entirely:
  - Lasers should stay ATTACHED to the anomaly weapon points
  - Think of them as rapid-fire guns with VERY high rate of fire
  - Lasers should be "fired" as projectiles toward the player
  - Remove the stupid moving/tracking laser beams
  - Make them behave like fast bullets shot from anomaly toward player

### 4. Missile vs Bullet Collision
- **Problem**: Player bullets cannot destroy anomaly missiles
- **Fix**: Add collision detection between player bullets and enemy missiles
  - When player bullet hits missile, both should be destroyed
  - Add small explosion effect when this happens

### 5. Missile Trail System
- **Problem**: Current missile trails don't look right
- **Fix**: Revert to proper missile trail system:
  - Trails should be points emitted from the REAR of the missile as it travels
  - NOT like player trails
  - Give trail points a short lifespan
  - Should look like exhaust particles being left behind

## Files to Modify:
- `src/space_bench/state.h` - Anomaly struct, missile collision
- `src/space_bench/state.c` - Spawn logic, laser firing, collision detection, orbital points
- `src/space_bench/render.c` - Visual rendering of orbital points, missile trails

## Priority Order:
1. Fix multiple anomaly spawning (critical bug)
2. Redesign laser system to be rapid-fire projectiles
3. Add bullet-missile collision
4. Improve visual design with orbital points
5. Fix missile trail rendering

## Notes:
- The layered HP system is good, keep that
- The HP bar visualization is fine
- Focus on making the anomaly look and behave more naturally
- Prioritize gameplay mechanics over fancy visuals