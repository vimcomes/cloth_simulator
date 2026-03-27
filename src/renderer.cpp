#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "renderer.h"
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cstdio>
#include <vector>

Renderer::Renderer() : vao_(0), vbo_(0), ebo_(0), shader_(0), texture_(0) {}

Renderer::~Renderer() {
    if (vao_)     glDeleteVertexArrays(1, &vao_);
    if (vbo_)     glDeleteBuffers(1, &vbo_);
    if (ebo_)     glDeleteBuffers(1, &ebo_);
    if (shader_)  glDeleteProgram(shader_);
    if (texture_) glDeleteTextures(1, &texture_);
}

bool Renderer::init(const std::string& texture_path) {
    // Shaders – look relative to executable (CMake copies them)
    shader_ = load_program("shaders/vertex.glsl", "shaders/fragment.glsl");
    if (!shader_) return false;

    if (!texture_path.empty())
        texture_ = load_texture(texture_path);
    if (!texture_)
        texture_ = make_checker_texture();

    // Allocate geometry arrays (we'll fill each frame)
    // Just create VAO/VBO/EBO
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);

    return true;
}

void Renderer::build_indices(const Cloth& cloth) {
    indices_.clear();
    for (int r = 0; r < cloth.rows - 1; ++r) {
        for (int c = 0; c < cloth.cols - 1; ++c) {
            unsigned int tl = cloth.index(c,   r);
            unsigned int tr = cloth.index(c+1, r);
            unsigned int bl = cloth.index(c,   r+1);
            unsigned int br = cloth.index(c+1, r+1);
            // Triangle 1
            indices_.push_back(tl);
            indices_.push_back(bl);
            indices_.push_back(tr);
            // Triangle 2
            indices_.push_back(tr);
            indices_.push_back(bl);
            indices_.push_back(br);
        }
    }
    num_indices_ = (int)indices_.size();
}

void Renderer::compute_normals(const Cloth& cloth) {
    // Zero normals
    for (auto& v : vertices_) v.normal = glm::vec3(0.0f);

    // Accumulate face normals
    for (int i = 0; i < num_indices_; i += 3) {
        unsigned int ia = indices_[i], ib = indices_[i+1], ic = indices_[i+2];
        glm::vec3 ab = vertices_[ib].pos - vertices_[ia].pos;
        glm::vec3 ac = vertices_[ic].pos - vertices_[ia].pos;
        glm::vec3 n  = glm::cross(ab, ac);
        vertices_[ia].normal += n;
        vertices_[ib].normal += n;
        vertices_[ic].normal += n;
    }

    for (auto& v : vertices_) {
        float len = glm::length(v.normal);
        if (len > 1e-6f) v.normal /= len;
    }
}

void Renderer::update_vertices(const Cloth& cloth) {
    int n = (int)cloth.particles.size();
    vertices_.resize(n);
    for (int i = 0; i < n; ++i) {
        vertices_[i].pos = cloth.particles[i].position;
        vertices_[i].uv  = cloth.particles[i].uv;
    }
}

void Renderer::render(const Cloth& cloth, const Camera& cam, int win_w, int win_h) {
    glViewport(0, 0, win_w, win_h);
    glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    // Пересобираем индексный буфер если сетка изменилась
    int expected = (cloth.cols - 1) * (cloth.rows - 1) * 6;
    if (num_indices_ != expected) build_indices(cloth);

    update_vertices(cloth);
    compute_normals(cloth);

    float aspect = (float)win_w / (float)win_h;
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view  = cam.view();
    glm::mat4 proj  = cam.projection(aspect);

    glUseProgram(shader_);
    glUniformMatrix4fv(glGetUniformLocation(shader_, "u_model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shader_, "u_view"),  1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shader_, "u_proj"),  1, GL_FALSE, glm::value_ptr(proj));

    glm::vec3 light_dir = glm::normalize(glm::vec3(1.0f, 2.0f, 3.0f));
    glUniform3fv(glGetUniformLocation(shader_, "u_light_dir"), 1, glm::value_ptr(light_dir));
    glm::vec3 eye = cam.eye();
    glUniform3fv(glGetUniformLocation(shader_, "u_eye_pos"), 1, glm::value_ptr(eye));
    glUniform1i(glGetUniformLocation(shader_, "u_tex"), 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_);

    glBindVertexArray(vao_);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER,
                 (GLsizeiptr)(vertices_.size() * sizeof(Vertex)),
                 vertices_.data(), GL_DYNAMIC_DRAW);

    // pos
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, pos));
    // uv
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, uv));
    // normal
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, normal));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 (GLsizeiptr)(indices_.size() * sizeof(unsigned int)),
                 indices_.data(), GL_STATIC_DRAW);

    glDrawElements(GL_TRIANGLES, num_indices_, GL_UNSIGNED_INT, nullptr);

    glBindVertexArray(0);
}

// ── Shader utilities ────────────────────────────────────────────────────────

GLuint Renderer::compile_shader(GLenum type, const std::string& src) {
    GLuint s = glCreateShader(type);
    const char* c = src.c_str();
    glShaderSource(s, 1, &c, nullptr);
    glCompileShader(s);
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(s, 1024, nullptr, log);
        std::fprintf(stderr, "Shader compile error: %s\n", log);
        glDeleteShader(s);
        return 0;
    }
    return s;
}

GLuint Renderer::load_program(const std::string& vert_path,
                               const std::string& frag_path) {
    auto read_file = [](const std::string& p) -> std::string {
        std::ifstream f(p);
        if (!f) throw std::runtime_error("Cannot open: " + p);
        std::ostringstream ss;
        ss << f.rdbuf();
        return ss.str();
    };

    std::string vert_src, frag_src;
    try {
        vert_src = read_file(vert_path);
        frag_src = read_file(frag_path);
    } catch (std::exception& e) {
        std::fprintf(stderr, "%s\n", e.what());
        return 0;
    }

    GLuint vert = compile_shader(GL_VERTEX_SHADER,   vert_src);
    GLuint frag = compile_shader(GL_FRAGMENT_SHADER, frag_src);
    if (!vert || !frag) return 0;

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);

    GLint ok;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(prog, 1024, nullptr, log);
        std::fprintf(stderr, "Shader link error: %s\n", log);
        glDeleteProgram(prog);
        prog = 0;
    }
    glDeleteShader(vert);
    glDeleteShader(frag);
    return prog;
}

GLuint Renderer::load_texture(const std::string& path) {
    int w, h, ch;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &ch, 4);
    if (!data) {
        std::fprintf(stderr, "Failed to load texture: %s\n", path.c_str());
        return 0;
    }
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(data);
    return tex;
}

GLuint Renderer::make_checker_texture() {
    const int SZ = 64;
    std::vector<unsigned char> data(SZ * SZ * 4);
    for (int y = 0; y < SZ; ++y)
        for (int x = 0; x < SZ; ++x) {
            bool white = ((x / 8) + (y / 8)) % 2 == 0;
            unsigned char c = white ? 220 : 60;
            int idx = (y * SZ + x) * 4;
            data[idx+0] = c;
            data[idx+1] = white ? 220 : 80;
            data[idx+2] = white ? 220 : 60;
            data[idx+3] = 255;
        }
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SZ, SZ, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return tex;
}

// ── Публичные утилиты ────────────────────────────────────────────────────────

void Renderer::reload_texture(const std::string& path) {
    GLuint tex = path.empty() ? 0 : load_texture(path);
    if (!tex) tex = make_checker_texture();
    if (texture_) glDeleteTextures(1, &texture_);
    texture_ = tex;
}

bool Renderer::image_size(const std::string& path, int& w, int& h) {
    int ch;
    return stbi_info(path.c_str(), &w, &h, &ch) != 0;
}
