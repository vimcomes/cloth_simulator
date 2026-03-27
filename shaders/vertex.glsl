#version 410 core

layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec3 a_normal;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;

out vec2 v_uv;
out vec3 v_normal;
out vec3 v_world_pos;

void main() {
    vec4 world = u_model * vec4(a_pos, 1.0);
    v_world_pos = world.xyz;
    v_uv        = a_uv;
    v_normal    = mat3(transpose(inverse(u_model))) * a_normal;
    gl_Position = u_proj * u_view * world;
}
