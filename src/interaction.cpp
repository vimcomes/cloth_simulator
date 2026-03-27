#include "interaction.h"
#include <glm/glm.hpp>
#include <cmath>

static float point_ray_dist(const glm::vec3& pt,
                             const glm::vec3& origin,
                             const glm::vec3& dir) {
    glm::vec3 v = pt - origin;
    float t = glm::dot(v, dir);
    if (t < 0.0f) t = 0.0f;
    glm::vec3 proj = origin + dir * t;
    return glm::length(pt - proj);
}

void Interaction::on_press(Cloth& cloth, const Camera& cam,
                           float ndc_x, float ndc_y, float aspect) {
    grabbed_idx = -1;

    glm::vec3 origin, dir;
    cam.ray(ndc_x, ndc_y, aspect, origin, dir);

    int   best_idx  = -1;
    float best_dist = RAY_THRESHOLD;

    for (int i = 0; i < (int)cloth.particles.size(); ++i) {
        if (cloth.particles[i].pinned) continue;
        float d = point_ray_dist(cloth.particles[i].position, origin, dir);
        if (d < best_dist) {
            best_dist = d;
            best_idx  = i;
        }
    }

    if (best_idx < 0) return;

    grabbed_idx = best_idx;
    glm::vec3 grabbed_pos = cloth.particles[best_idx].position;

    // Плоскость перетаскивания: перпендикулярна взгляду, проходит через захваченную точку
    drag_plane_normal = -glm::normalize(dir);

    // Вычислить начальную точку пересечения
    float denom = glm::dot(drag_plane_normal, dir);
    float t     = (std::abs(denom) > 1e-6f)
                  ? glm::dot(drag_plane_normal, grabbed_pos - origin) / denom
                  : 0.0f;
    prev_target = origin + dir * t;

    dragging = true;
}

glm::vec3 Interaction::ray_plane_intersect(const glm::vec3& origin,
                                            const glm::vec3& dir) const {
    float denom = glm::dot(drag_plane_normal, dir);
    if (std::abs(denom) < 1e-6f) return prev_target;
    float t = glm::dot(drag_plane_normal, prev_target - origin) / denom;
    return origin + dir * t;
}

void Interaction::on_drag(Cloth& cloth, const Camera& cam,
                          float ndc_x, float ndc_y, float aspect) {
    if (!dragging || grabbed_idx < 0) return;

    glm::vec3 origin, dir;
    cam.ray(ndc_x, ndc_y, aspect, origin, dir);

    glm::vec3 target = ray_plane_intersect(origin, dir);

    Particle& p = cloth.particles[grabbed_idx];

    // Переносим скорость мыши на частицу:
    // prev_position остаётся на предыдущей позиции цели — Verlet получит
    // правильную скорость (target - prev_target) / dt при следующей интеграции.
    p.prev_position = prev_target;
    p.position      = target;

    prev_target = target;
}

void Interaction::on_release() {
    dragging    = false;
    grabbed_idx = -1;
}
