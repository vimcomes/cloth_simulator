# Cloth Simulation

Real-time interactive cloth simulation — CPU Verlet physics, OpenGL 4.x rendering, SDL2 input.

![cloth](https://img.shields.io/badge/C%2B%2B-20-blue) ![opengl](https://img.shields.io/badge/OpenGL-4.1-green) ![sdl2](https://img.shields.io/badge/SDL2-2.30-orange)

## Features

- Position-Based Dynamics with Verlet integration
- 40×N particle grid, top row pinned (cloth hangs from top edge)
- Three constraint types: structural, shear, bend
- 8 physics substeps per frame — stable, non-stretchy fabric
- Phong/Blinn-Phong shading with per-frame normal recomputation
- Mouse grab and drag — single-point pick with velocity transfer on release
- **Heaviness slider** — one knob controls damping + wind, `[` lighter `]` heavier
- **Custom texture** — drag-and-drop any PNG/JPG onto the window
- Grid proportions auto-adapt to image aspect ratio (~1600 particles total)
- Orbit camera: right-drag to rotate, scroll to zoom

## Controls

| Input | Action |
|---|---|
| Left mouse | Grab and drag cloth |
| Right mouse drag | Rotate camera |
| Mouse wheel | Zoom in/out |
| `[` / `]` | Decrease / increase heaviness |
| `R` | Reset cloth |
| `Esc` | Quit |
| Drag & Drop file | Load texture + resize grid |

## Build

### Dependencies

```bash
# Debian/Ubuntu
sudo apt install libsdl2-dev libglm-dev libglew-dev
```

If the dev packages are unavailable, the build system falls back to bundled headers in `external/` (SDL2 headers and GLM headers are checked in for convenience).

### Compile

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### Run

```bash
./cloth_sim                        # procedural checker texture, square 40×40 grid
./cloth_sim /path/to/image.png     # custom texture, grid adapts to image proportions
```

Or drag any image onto the running window to reload texture and resize the grid live.

## Physics Parameters

All tunable via constants in `src/cloth.h` and runtime via keyboard:

| Parameter | Value | Effect |
|---|---|---|
| `NUM_SUBSTEPS` | 8 | Substeps per frame — higher = less stretch |
| `CONSTRAINT_ITERS` | 8/substep | Solver iterations — higher = stiffer |
| `GRAVITY_SCALE` | 12.0 | Gravity multiplier |
| `PARTICLE_MASS` | 1.0 | Particle mass (kg) |
| `SPACING` | 0.05 m | Rest distance between particles |
| `damping` (runtime) | 0.9940–0.9999/substep | Energy loss per substep |

**Heaviness** maps to `damping` on an exponential scale:
- Level 0 → `damping=0.9999` (~95% energy remaining per second, sheer fabric)
- Level 7 → `damping=0.9970` (~10% energy remaining per second, heavy curtain)
- Level 10 → `damping=0.9940` (~0.6% energy remaining per second, near-static)

## Project Structure

```
cloth_sim/
├── CMakeLists.txt
├── README.md
├── AGENTS.md           ← AI agent continuation guide
├── external/
│   ├── stb_image.h     ← single-header image loader
│   ├── SDL2/           ← SDL2 headers (bundled for headless builds)
│   ├── glm/            ← GLM headers (bundled)
│   └── lib/            ← libSDL2.so symlink
├── shaders/
│   ├── vertex.glsl
│   └── fragment.glsl
└── src/
    ├── main.cpp         ← SDL2 init, event loop, physics/render orchestration
    ├── particle.h       ← Particle struct + Verlet integration
    ├── cloth.h / .cpp   ← NxM grid, constraints, substep update
    ├── renderer.h / .cpp← OpenGL VAO/VBO, shaders, texture, normals
    ├── interaction.h / .cpp ← Ray-cast pick, drag with velocity transfer
    └── camera.h         ← Orbit camera, ray unprojection
```

## Architecture Notes

**Physics loop** (per frame):
1. For each of 8 substeps:
   - Apply gravity + wind forces
   - Verlet integrate positions
   - Solve constraints (8 iterations)
2. Grabbed particle is temporarily pinned during the full substep loop so constraints propagate naturally from the fixed point outward

**Interaction**: single-particle grab, not area-based. Velocity transferred on release via `prev_position = prev_mouse_target` — cloth continues moving in mouse direction after release.

**Texture orientation**: `stbi_set_flip_vertically_on_load` is intentionally **off** — UV row 0 (top of cloth, pinned) maps to the top of the image file directly.
