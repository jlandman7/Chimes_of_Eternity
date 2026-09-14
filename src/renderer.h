#pragma once

#include <GLFW/glfw3.h>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "water_mesh.h"

struct LightData {
    glm::vec3 position;
    glm::vec3 color;
    float intensity;
};

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool init();
    void render(const WaterMesh& mesh, 
                const glm::mat4& view, 
                const glm::mat4& projection, 
                const glm::vec3& camera_pos, 
                const std::vector<LightData>& active_lights);

private:
    GLuint shader_program = 0;

    GLuint compile_shader(GLenum type, const char* source);
    GLuint link_program(GLuint vs, GLuint fs);
};