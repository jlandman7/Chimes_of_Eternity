#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "water_mesh.h"

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool init();
    void render(const WaterMesh& mesh, const glm::mat4& view, const glm::mat4& projection, const glm::vec3& camera_pos);

private:
    GLuint shader_program = 0;

    GLuint compile_shader(GLenum type, const char* source);
    GLuint link_program(GLuint vs, GLuint fs);
};