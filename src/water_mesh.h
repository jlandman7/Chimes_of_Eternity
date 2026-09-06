#pragma once
#include <vector>
#include <glm/glm.hpp>

// Platform-specific OpenGL headers
#ifdef _WIN32
    #include <GL/gl.h>
#else
    #include <OpenGL/gl3.h>
#endif
#include <GLFW/glfw3.h>

#include "water_simulation.h"

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texcoord;
};

class WaterMesh {
public:
    WaterMesh();
    ~WaterMesh();

    // Allocates GPU buffers and builds grid topology
    void initialize(const WaterSimulation& sim);

    // Updates height positions and normal vectors without heap allocations
    void update(const WaterSimulation& sim);

    // Renders the water grid
    void render() const;

private:
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    int index_count = 0;

    void cleanup();
};