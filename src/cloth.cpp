#include "cloth.h"
#include <cmath>
#include <glm/glm.hpp>

Cloth::Cloth() {
    reset();
}

void Cloth::reset(int new_cols, int new_rows) {
    if (new_cols > 0) cols = new_cols;
    if (new_rows > 0) rows = new_rows;

    particles.clear();
    constraints.clear();

    particles.reserve(rows * cols);
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            Particle p;
            p.position      = glm::vec3(c * SPACING, -r * SPACING, 0.0f);
            p.prev_position = p.position;
            p.acceleration  = glm::vec3(0.0f);
            p.mass          = PARTICLE_MASS;
            p.pinned        = (r == 0);
            p.uv            = glm::vec2(
                static_cast<float>(c) / (cols - 1),
                static_cast<float>(r) / (rows - 1)
            );
            particles.push_back(p);
        }
    }

    build_constraints();
}

void Cloth::build_constraints() {
    auto add = [&](int a, int b, float len) {
        constraints.push_back({a, b, len});
    };

    const float diag = SPACING * std::sqrt(2.0f);
    const float bend = SPACING * 2.0f;

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            int i = index(c, r);

            if (c + 1 < cols) add(i, index(c+1, r), SPACING);
            if (r + 1 < rows) add(i, index(c, r+1), SPACING);

            if (c + 1 < cols && r + 1 < rows) add(i, index(c+1, r+1), diag);
            if (c - 1 >= 0   && r + 1 < rows) add(i, index(c-1, r+1), diag);

            if (c + 2 < cols) add(i, index(c+2, r), bend);
            if (r + 2 < rows) add(i, index(c, r+2), bend);
        }
    }
}

void Cloth::apply_forces(float time) {
    const glm::vec3 gravity(0.0f, -9.81f * GRAVITY_SCALE, 0.0f);

    for (auto& p : particles) {
        if (p.pinned) continue;
        p.apply_force(gravity * p.mass);

        float wind_z = wind_strength * std::sin(time * 0.4f + p.position.x * 3.0f);
        p.apply_force(glm::vec3(0.0f, 0.0f, wind_z));
    }
}

void Cloth::integrate(float dt) {
    for (auto& p : particles)
        p.integrate(dt, damping);
}

void Cloth::solve_constraints() {
    for (int iter = 0; iter < CONSTRAINT_ITERS; ++iter) {
        for (auto& c : constraints) {
            Particle& a = particles[c.a];
            Particle& b = particles[c.b];

            glm::vec3 delta = b.position - a.position;
            float dist = glm::length(delta);
            if (dist < 1e-6f) continue;

            float diff = (dist - c.rest_length) / dist;
            glm::vec3 correction = delta * 0.5f * diff;

            if (!a.pinned) a.position += correction;
            if (!b.pinned) b.position -= correction;
        }
    }
}

void Cloth::update(float dt, float time) {
    float sub_dt = dt / NUM_SUBSTEPS;
    for (int s = 0; s < NUM_SUBSTEPS; ++s) {
        apply_forces(time + s * sub_dt);
        integrate(sub_dt);
        solve_constraints();
    }
}
