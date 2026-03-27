#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include "cloth.h"
#include "camera.h"
#include <string>
#include <vector>

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool init(const std::string& texture_path);
    void reload_texture(const std::string& path);  // смена текстуры без пересоздания шейдеров
    void render(const Cloth& cloth, const Camera& cam, int win_w, int win_h);

    // Узнать размер изображения не загружая его в GPU (через stbi_info)
    static bool image_size(const std::string& path, int& w, int& h);

private:
    GLuint vao_, vbo_, ebo_, shader_, texture_;
    int    num_indices_ = 0;

    struct Vertex {
        glm::vec3 pos;
        glm::vec2 uv;
        glm::vec3 normal;
    };

    std::vector<Vertex>        vertices_;
    std::vector<unsigned int>  indices_;

    void build_indices(const Cloth& cloth);
    void update_vertices(const Cloth& cloth);
    void compute_normals(const Cloth& cloth);

    GLuint compile_shader(GLenum type, const std::string& src);
    GLuint load_program(const std::string& vert_path, const std::string& frag_path);
    GLuint load_texture(const std::string& path);
    GLuint make_checker_texture();
};
