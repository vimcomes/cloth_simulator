#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    float yaw   = 0.0f;
    float pitch = 0.0f;
    float dist  = 2.5f;
    glm::vec3 target = glm::vec3(0.95f, -0.95f, 0.0f); // center of 40x40 cloth at 0.05 spacing

    glm::mat4 view() const {
        glm::vec3 pos = eye();
        return glm::lookAt(pos, target, glm::vec3(0, 1, 0));
    }

    glm::mat4 projection(float aspect) const {
        return glm::perspective(glm::radians(45.0f), aspect, 0.01f, 100.0f);
    }

    glm::vec3 eye() const {
        float cy = std::cos(glm::radians(yaw));
        float sy = std::sin(glm::radians(yaw));
        float cp = std::cos(glm::radians(pitch));
        float sp = std::sin(glm::radians(pitch));
        return target + dist * glm::vec3(sy * cp, sp, cy * cp);
    }

    // Ray from screen pixel (NDC)
    void ray(float ndc_x, float ndc_y, float aspect,
             glm::vec3& origin, glm::vec3& dir) const {
        glm::mat4 inv = glm::inverse(projection(aspect) * view());
        glm::vec4 near4 = inv * glm::vec4(ndc_x, ndc_y, -1.0f, 1.0f);
        glm::vec4 far4  = inv * glm::vec4(ndc_x, ndc_y,  1.0f, 1.0f);
        glm::vec3 near3 = glm::vec3(near4) / near4.w;
        glm::vec3 far3  = glm::vec3(far4)  / far4.w;
        origin = near3;
        dir    = glm::normalize(far3 - near3);
    }
};
