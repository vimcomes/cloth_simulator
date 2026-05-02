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
    grabbed_idx = -1;

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

        float wz = wind_strength * (
            std::sin(time * 0.4f + p.position.x * 3.0f) +
            std::sin(time * 0.7f + p.position.y * 2.5f) * 0.5f +
            std::sin(time * 0.2f + p.position.z * 4.0f) * 0.3f
        );
        float wx = wind_strength * 0.3f * (
            std::sin(time * 0.3f + p.position.y * 3.0f) +
            std::sin(time * 0.6f + p.position.x * 2.0f) * 0.5f
        );
        p.apply_force(glm::vec3(wx, 0.0f, wz));
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
            glm::vec3 correction = delta * 0.5f * diff * stiffness;

            if (!a.pinned) a.position += correction;
            if (!b.pinned) b.position -= correction;
        }
    }
}

void Cloth::self_collision() {
    const float cell_size = SPACING * 2.0f;
    const float min_dist  = SPACING * 0.8f;

    auto hash_fn = [](const glm::ivec3& k) -> size_t {
        return size_t(k.x * 73856093) ^ size_t(k.y * 19349663) ^ size_t(k.z * 83492791);
    };

    std::unordered_map<glm::ivec3, std::vector<int>, decltype(hash_fn)> grid(0, hash_fn);

    for (int i = 0; i < (int)particles.size(); ++i) {
        glm::ivec3 key(
            (int)std::floor(particles[i].position.x / cell_size),
            (int)std::floor(particles[i].position.y / cell_size),
            (int)std::floor(particles[i].position.z / cell_size)
        );
        grid[key].push_back(i);
    }

    for (int i = 0; i < (int)particles.size(); ++i) {
        if (particles[i].pinned) continue;

        glm::ivec3 base(
            (int)std::floor(particles[i].position.x / cell_size),
            (int)std::floor(particles[i].position.y / cell_size),
            (int)std::floor(particles[i].position.z / cell_size)
        );

        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dz = -1; dz <= 1; ++dz) {
                    glm::ivec3 nk = base + glm::ivec3(dx, dy, dz);
                    auto it = grid.find(nk);
                    if (it == grid.end()) continue;

                    for (int j : it->second) {
                        if (j <= i) continue;
                        if (are_connected(i, j, cols)) continue;

                        glm::vec3 delta = particles[j].position - particles[i].position;
                        float dist = glm::length(delta);
                        if (dist < min_dist && dist > 1e-6f) {
                            float correction = (min_dist - dist) / dist * 0.5f;
                            glm::vec3 push = delta * correction;
                            if (!particles[i].pinned) particles[i].position -= push;
                            if (!particles[j].pinned) particles[j].position += push;
                        }
                    }
                }
            }
        }
    }
}

bool Cloth::are_connected(int i, int j, int c) {
    int ci = i % c, ri = i / c;
    int cj = j % c, rj = j / c;
    int dc = std::abs(ci - cj);
    int dr = std::abs(ri - rj);
    return (dc <= 1 && dr <= 1) || (dc == 2 && dr == 0) || (dc == 0 && dr == 2);
}

void Cloth::update(float dt, float time) {
    bool was_pinned = false;
    if (grabbed_idx >= 0 && grabbed_idx < (int)particles.size()) {
        was_pinned = particles[grabbed_idx].pinned;
        particles[grabbed_idx].pinned = true;
    }

    float sub_dt = dt / NUM_SUBSTEPS;
    for (int s = 0; s < NUM_SUBSTEPS; ++s) {
        apply_forces(time + s * sub_dt);
        integrate(sub_dt);
        solve_constraints();
        self_collision();
    }

    if (grabbed_idx >= 0 && grabbed_idx < (int)particles.size()) {
        particles[grabbed_idx].pinned = was_pinned;
    }
}
