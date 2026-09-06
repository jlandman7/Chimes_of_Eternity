#pragma once

#ifdef _WIN32
    #include <GL/gl.h>
#else
    #include <OpenGL/gl3.h>
#endif
#include <vector>
#include <glm/glm.hpp>
#include "water_simulation.h"

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texcoord;
};

class WaterMesh {
public:
    WaterMesh() = default;
    ~WaterMesh();

    void initialize(const WaterSimulation& sim);
    void update(const WaterSimulation& sim);
    void render() const;

private:
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLsizei index_count = 0;

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};