#pragma once
#include "cloth.h"
#include "camera.h"

class Interaction {
public:
    static constexpr float RAY_THRESHOLD = 0.18f;

    bool dragging = false;

    void on_press(Cloth& cloth, const Camera& cam,
                  float ndc_x, float ndc_y, float aspect);

    void on_drag(Cloth& cloth, const Camera& cam,
                 float ndc_x, float ndc_y, float aspect);

    void on_release(Cloth& cloth);

private:
    glm::vec3 drag_plane_normal;
    glm::vec3 prev_target;

    glm::vec3 ray_plane_intersect(const glm::vec3& origin, const glm::vec3& dir) const;
};
