#include "renderer.h"
#include <iostream>
#include <algorithm>

#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#else
#ifdef __APPLE__
#include <OpenGL/gl3.h>
#endif
#endif

#ifdef __EMSCRIPTEN__
static const char* vertex_shader_src = R"(
#version 300 es
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 WorldPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    WorldPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * vec4(WorldPos, 1.0);
}
)";

static const char* fragment_shader_src = R"(
#version 300 es
precision highp float;

out vec4 FragColor;

in vec3 WorldPos;
in vec3 Normal;

struct Light {
    vec3 position;
    vec3 color;
    float intensity;
};

const int MAX_LIGHTS = 12;
uniform int u_numLights;
uniform Light u_lights[MAX_LIGHTS];

void main() {
    vec3 N = normalize(Normal);
    vec3 V = vec3(0.0, 1.0, 0.0); // Top-down camera view
    
    // Slight ambient so the water mesh is visible even before drops hit
    vec3 totalLight = vec3(0.015, 0.025, 0.045); 

    for (int i = 0; i < u_numLights; ++i) {
        if (u_lights[i].intensity <= 0.0001) continue;

        // Position light closer to water surface (y = 0.25)
        vec3 lightPos = vec3(u_lights[i].position.x, 0.25, u_lights[i].position.z);
        vec3 lightVec = lightPos - WorldPos;
        float dist = length(lightVec);
        vec3 L = lightVec / max(dist, 0.0001);
        vec3 H = normalize(L + V);

        float NdotL = max(dot(N, L), 0.0);
        float NdotV = max(dot(N, V), 0.0);
        float NdotH = max(dot(N, H), 0.0001);

        float atten = 1.0 / (1.0 + 1.2 * dist + 0.8 * dist * dist);

        // Water Fresnel
        float F0 = 0.04;
        float fresnel = F0 + (1.0 - F0) * pow(clamp(1.0 - NdotV, 0.0, 1.0), 5.0);

        // Strong diffuse splash illumination on ripples
        vec3 diffuse = u_lights[i].color * u_lights[i].intensity * NdotL * atten * 12.0;

        // Specular highlight glint
        float shininess = 32.0;
        float specFactor = pow(NdotH, shininess);
        vec3 spec = u_lights[i].color * u_lights[i].intensity * specFactor * fresnel * atten * 8.0;

        totalLight += diffuse + spec;
    }

    // Tone mapping & gamma correction
    vec3 finalColor = totalLight / (totalLight + vec3(1.0));
    finalColor = pow(max(finalColor, vec3(0.0)), vec3(1.0 / 2.2));

    FragColor = vec4(finalColor, 1.0);
}
)";
#else
static const char* vertex_shader_src = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 WorldPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    WorldPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * vec4(WorldPos, 1.0);
}
)";

static const char* fragment_shader_src = R"(
#version 330 core
out vec4 FragColor;

in vec3 WorldPos;
in vec3 Normal;

struct Light {
    vec3 position;
    vec3 color;
    float intensity;
};

const int MAX_LIGHTS = 12;
uniform int u_numLights;
uniform Light u_lights[MAX_LIGHTS];

void main() {
    vec3 N = normalize(Normal);
    vec3 V = vec3(0.0, 1.0, 0.0);
    
    vec3 totalLight = vec3(0.0);

    for (int i = 0; i < u_numLights; ++i) {
        if (u_lights[i].intensity <= 0.0001) continue;

        vec3 lightVec = u_lights[i].position - WorldPos;
        float dist = length(lightVec);
        vec3 L = lightVec / max(dist, 0.0001);
        vec3 H = normalize(L + V);

        float NdotL = max(dot(N, L), 0.0);
        float NdotV = max(dot(N, V), 0.0);
        float NdotH = max(dot(N, H), 0.0001);

        float atten = 1.0 / (1.0 + 0.8 * dist + 0.4 * dist * dist);

        float F0 = 0.02;
        float fresnel = F0 + (1.0 - F0) * pow(clamp(1.0 - NdotV, 0.0, 1.0), 5.0);

        vec3 diffuse = u_lights[i].color * u_lights[i].intensity * NdotL * atten * 2.5;

        float shininess = 64.0;
        float specFactor = pow(NdotH, shininess);
        vec3 spec = u_lights[i].color * u_lights[i].intensity * specFactor * fresnel * atten * 4.0;

        totalLight += diffuse + spec;
    }

    vec3 ambient = vec3(0.005, 0.01, 0.02);
    vec3 finalColor = ambient + totalLight;

    finalColor = finalColor / (finalColor + vec3(1.0));
    finalColor = pow(max(finalColor, vec3(0.0)), vec3(1.0 / 2.2));

    FragColor = vec4(finalColor, 1.0);
}
)";
#endif

Renderer::Renderer() = default;

Renderer::~Renderer() {
    if (shader_program) glDeleteProgram(shader_program);
}

GLuint Renderer::compile_shader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    return shader;
}

GLuint Renderer::link_program(GLuint vs, GLuint fs) {
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint success;
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(prog, 512, nullptr, infoLog);
        std::cerr << "ERROR::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

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

void Renderer::render(const WaterMesh& mesh, 
                      const glm::mat4& view, 
                      const glm::mat4& projection, 
                      const glm::vec3& camera_pos, 
                      const std::vector<LightData>& active_lights) {
    if (!shader_program) return;

    glUseProgram(shader_program);

    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shader_program, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shader_program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    int num_lights = std::min(static_cast<int>(active_lights.size()), 12);
    glUniform1i(glGetUniformLocation(shader_program, "u_numLights"), num_lights);

    for (int i = 0; i < num_lights; ++i) {
        std::string prefix = "u_lights[" + std::to_string(i) + "].";
        glUniform3fv(glGetUniformLocation(shader_program, (prefix + "position").c_str()), 1, glm::value_ptr(active_lights[i].position));
        glUniform3fv(glGetUniformLocation(shader_program, (prefix + "color").c_str()), 1, glm::value_ptr(active_lights[i].color));
        glUniform1f(glGetUniformLocation(shader_program, (prefix + "intensity").c_str()), active_lights[i].intensity);
    }

    mesh.render();
    glUseProgram(0);
}