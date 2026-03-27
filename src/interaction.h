#pragma once
#include "cloth.h"
#include "camera.h"

class Interaction {
public:
    static constexpr float RAY_THRESHOLD = 0.18f; // max dist from ray to particle

    bool dragging    = false;
    int  grabbed_idx = -1;  // публичный — main.cpp читает для временного пина

    void on_press(Cloth& cloth, const Camera& cam,
                  float ndc_x, float ndc_y, float aspect);

    void on_drag(Cloth& cloth, const Camera& cam,
                 float ndc_x, float ndc_y, float aspect);

    void on_release();

private:
    glm::vec3 drag_plane_normal;
    glm::vec3 prev_target;   // позиция цели на прошлом кадре — нужна для переноса скорости мыши

    glm::vec3 ray_plane_intersect(const glm::vec3& origin, const glm::vec3& dir) const;
};
