#pragma once
#include <glm/glm.hpp>

struct Particle {
    glm::vec3 position;
    glm::vec3 prev_position;
    glm::vec3 acceleration;
    float     mass;
    bool      pinned;
    glm::vec2 uv;

    void integrate(float dt, float damping) {
        if (pinned) return;
        glm::vec3 velocity = (position - prev_position) * damping;
        glm::vec3 new_pos  = position + velocity + acceleration * (dt * dt);
        prev_position = position;
        position      = new_pos;
        acceleration  = glm::vec3(0.0f);
    }

    void apply_force(const glm::vec3& force) {
        if (!pinned)
            acceleration += force / mass;
    }
};
