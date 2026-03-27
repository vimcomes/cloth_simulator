#pragma once
#include "particle.h"
#include <vector>

struct Constraint {
    int   a, b;
    float rest_length;
};

class Cloth {
public:
    static constexpr float SPACING          = 0.05f;
    static constexpr float GRAVITY_SCALE    = 12.0f;
    static constexpr float PARTICLE_MASS    = 1.0f;
    static constexpr int   CONSTRAINT_ITERS = 8;   // per substep
    static constexpr int   NUM_SUBSTEPS     = 8;

    // Размер сетки — задаётся при reset()
    int cols = 40;
    int rows = 40;

    // Runtime-параметры — управляются через heaviness
    float damping       = 0.9994f;
    float wind_strength = 0.04f;

    std::vector<Particle>   particles;
    std::vector<Constraint> constraints;

    Cloth();
    void reset(int new_cols = -1, int new_rows = -1);  // -1 = использовать текущие
    void update(float dt, float time);

    int index(int col, int row) const { return row * cols + col; }

private:
    void build_constraints();
    void apply_forces(float time);
    void integrate(float dt);
    void solve_constraints();
};
