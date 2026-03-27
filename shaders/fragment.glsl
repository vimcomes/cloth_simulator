#version 410 core

in vec2 v_uv;
in vec3 v_normal;
in vec3 v_world_pos;

uniform sampler2D u_tex;
uniform vec3      u_light_dir;
uniform vec3      u_eye_pos;

out vec4 frag_color;

void main() {
    vec3 N = normalize(v_normal);
    vec3 L = normalize(u_light_dir);
    vec3 V = normalize(u_eye_pos - v_world_pos);
    vec3 H = normalize(L + V);

    // Two-sided lighting
    float diff = max(dot(N, L), 0.0) + max(dot(-N, L), 0.0) * 0.4;
    float spec = pow(max(dot(N, H), 0.0), 32.0) * 0.4;

    vec4 tex_color = texture(u_tex, v_uv);
    vec3 ambient   = tex_color.rgb * 0.25;
    vec3 diffuse   = tex_color.rgb * diff * 0.75;
    vec3 specular  = vec3(0.5) * spec;

    frag_color = vec4(ambient + diffuse + specular, tex_color.a);
}
