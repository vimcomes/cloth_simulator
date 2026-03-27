# Cloth Simulation — C++ / SDL2 / OpenGL

## Project Goal
Build a real-time interactive cloth simulation with texture mapping.
A rectangular piece of cloth hangs from top edge, user can grab and drag any point with the mouse — cloth reacts with realistic heavy fabric physics (like a heavy curtain or satin flag).

## Tech Stack
- C++20
- SDL2 (window, input, events)
- OpenGL 4.x (rendering, textures)
- GLM (math)
- stb_image (texture loading)
- CMake build system
- Linux (Ubuntu 24.04, GCC 14)

## Architecture

### 1. Particle System (`particle.h`)
- Struct: `position` (vec3), `prev_position` (vec3), `acceleration` (vec3), `mass`, `pinned` (bool), `uv` (vec2)
- Verlet integration: `new_pos = pos + (pos - prev_pos) * damping + acc * dt * dt`
- Damping factor ~0.997 for heavy fabric feel

### 2. Cloth Grid (`cloth.h`, `cloth.cpp`)
- NxM grid of particles (default 40x40)
- Top row pinned (fixed) — cloth hangs from top
- Three constraint types:
  - Structural: horizontal + vertical neighbors (rest_length = spacing)
  - Shear: diagonal neighbors (rest_length = spacing * sqrt(2))
  - Bend: skip-one neighbors (rest_length = spacing * 2)
- Constraint solver: 15-20 iterations per frame for stiff heavy fabric
- Gravity: vec3(0, -9.81, 0) * mass_multiplier
- Wind: optional subtle sinusoidal force for ambient motion

### 3. Mouse Interaction (`interaction.h`)
- On mouse button down: ray cast from camera through mouse position
- Find nearest particle to ray (within threshold)
- While dragging: override selected particle position to follow mouse projected onto cloth plane
- On release: particle resumes physics
- Support multi-particle influence: drag affects nearby particles too (radius falloff) for smoother grab

### 4. Renderer (`renderer.h`, `renderer.cpp`)
- OpenGL textured mesh
- Vertex buffer: position + UV + normal per particle
- Index buffer: two triangles per grid cell
- Recompute normals each frame (cross product of edges) for lighting
- Simple Phong/Blinn-Phong shading
- Texture: load from file (flag, fabric pattern, or any image)
- Background: dark gradient or solid color

### 5. Shaders
- `vertex.glsl`: MVP transform, pass UV and normal to fragment
- `fragment.glsl`: texture sampling + directional light + ambient

### 6. Main Loop (`main.cpp`)
- Init SDL2 + OpenGL context (4.x core profile)
- Load texture (default: provide a built-in procedural checker if no file given)
- Main loop:
  1. Handle SDL events (mouse, keyboard, quit)
  2. Apply forces (gravity, wind)
  3. Verlet integration step
  4. Constraint solving (15-20 iterations)
  5. Update VBO (positions + normals)
  6. Render
  7. SDL_GL_SwapWindow
- Target: 60 FPS, physics step fixed at 1/60s

## Controls
- Left mouse button: grab and drag cloth
- R key: reset cloth to initial state
- Escape: quit
- Mouse wheel: zoom camera
- Right mouse drag: rotate camera

## Build
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
./cloth_sim [optional_texture.png]
```

## Dependencies (install before build)
```bash
sudo apt install libsdl2-dev libglm-dev libglew-dev
```
stb_image.h — download single header into `external/` dir.

## Key Physics Parameters (tune for heavy curtain feel)
- `DAMPING = 0.997` (high = heavy, slow movement)
- `GRAVITY_SCALE = 25.0` (strong pull)
- `CONSTRAINT_ITERATIONS = 18`
- `GRID_SIZE = 40x40`
- `PARTICLE_MASS = 1.5`
- `STIFFNESS = 1.0` (constraint compliance, 1.0 = rigid springs)

## File Structure
```
cloth_sim/
├── CMakeLists.txt
├── AGENTS.md
├── external/
│   └── stb_image.h
├── shaders/
│   ├── vertex.glsl
│   └── fragment.glsl
└── src/
    ├── main.cpp
    ├── cloth.h
    ├── cloth.cpp
    ├── particle.h
    ├── renderer.h
    ├── renderer.cpp
    ├── interaction.h
    ├── interaction.cpp
    └── camera.h
```

## Important Notes
- Do NOT use CUDA — keep it pure CPU, this grid size doesn't need GPU compute
- Do NOT use external physics engines (Bullet, PhysX) — implement Verlet from scratch
- Use `GL_DYNAMIC_DRAW` for VBO since mesh updates every frame
- Normals must be recalculated each frame for correct lighting on deformed cloth
- stb_image.h: `#define STB_IMAGE_IMPLEMENTATION` in exactly ONE .cpp file
- All code C++20, use `<format>` if needed, structured bindings, etc.
