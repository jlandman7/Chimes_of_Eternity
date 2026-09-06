#include "renderer.h"
#include <iostream>

static const char* vertex_shader_src = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 WorldPos;
out vec3 Normal;
out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    WorldPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
    gl_Position = projection * view * vec4(WorldPos, 1.0);
}
)";

static const char* fragment_shader_src = R"(
#version 330 core
out vec4 FragColor;

in vec3 WorldPos;
in vec3 Normal;
in vec2 TexCoord;

uniform vec3 cameraPos;
uniform vec3 lightDir;

void main() {
    vec3 N = normalize(Normal);
    vec3 V = normalize(cameraPos - WorldPos);
    vec3 L = normalize(-lightDir);
    vec3 H = normalize(L + V);

    // Fresnel Reflection
    float F0 = 0.04; 
    float NdotV = max(dot(N, V), 0.0);
    float fresnel = F0 + (1.0 - F0) * pow(1.0 - NdotV, 3.0);

    // Specular Glint
    float NdotH = max(dot(N, H), 0.0);
    float specSharp = pow(NdotH, 128.0) * 1.8;
    vec3 specular = vec3(0.9, 0.95, 1.0) * specSharp;

    // Dark Obsidian Palette (Replaces Saturated Cyan)
    vec3 deepVoid = vec3(0.002, 0.003, 0.005);
    vec3 waveShallow = vec3(0.015, 0.025, 0.035);

    float heightSignal = clamp(WorldPos.y * 20.0 + 0.2, 0.0, 1.0);
    vec3 bodyColor = mix(deepVoid, waveShallow, heightSignal);

    // Subtle edge rim light
    float rim = pow(1.0 - NdotV, 2.0) * 0.08;
    vec3 rimColor = vec3(0.1, 0.15, 0.2) * rim;

    vec3 finalColor = bodyColor + specular + rimColor;

    FragColor = vec4(finalColor, 1.0);
}
)";

Renderer::Renderer() = default;

Renderer::~Renderer() {
    if (shader_program) glDeleteProgram(shader_program);
}

GLuint Renderer::compile_shader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    return shader;
}

GLuint Renderer::link_program(GLuint vs, GLuint fs) {
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

bool Renderer::init() {
    GLuint vs = compile_shader(GL_VERTEX_SHADER, vertex_shader_src);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fragment_shader_src);
    shader_program = link_program(vs, fs);
    return shader_program != 0;
}

void Renderer::render(const WaterMesh& mesh, const glm::mat4& view, const glm::mat4& projection, const glm::vec3& camera_pos) {
    if (!shader_program) return;

    glUseProgram(shader_program);

    glm::mat4 model = glm::mat4(1.0f);
    // Light positioned almost straight overhead with a subtle offset to catch slopes
    glm::vec3 light_dir = glm::normalize(glm::vec3(-0.1f, -1.0f, -0.1f));

    glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shader_program, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shader_program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3fv(glGetUniformLocation(shader_program, "cameraPos"), 1, glm::value_ptr(camera_pos));
    glUniform3fv(glGetUniformLocation(shader_program, "lightDir"), 1, glm::value_ptr(light_dir));

    mesh.render();
    glUseProgram(0);
}