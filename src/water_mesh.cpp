#include "water_mesh.h"
#include <iostream>

WaterMesh::WaterMesh() = default;

WaterMesh::~WaterMesh() {
    cleanup();
}

void WaterMesh::cleanup() {
    if (vbo) { glDeleteBuffers(1, &vbo); vbo = 0; }
    if (ebo) { glDeleteBuffers(1, &ebo); ebo = 0; }
    if (vao) { glDeleteVertexArrays(1, &vao); vao = 0; }
}

void WaterMesh::initialize(const WaterSimulation& sim) {
    cleanup();

    int res = sim.get_resolution();
    float domain = sim.get_domain_size();
    float half_domain = domain * 0.5f;

    vertices.resize(res * res);

    // 1. Generate Vertex Positions & UVs
    for (int y = 0; y < res; ++y) {
        for (int x = 0; x < res; ++x) {
            int idx = y * res + x;

            float norm_x = static_cast<float>(x) / (res - 1);
            float norm_z = static_cast<float>(y) / (res - 1);

            float pos_x = norm_x * domain - half_domain;
            float pos_z = norm_z * domain - half_domain;
            float pos_y = sim.get_height(x, y);

            vertices[idx].position = glm::vec3(pos_x, pos_y, pos_z);
            vertices[idx].normal   = glm::vec3(0.0f, 1.0f, 0.0f);
            vertices[idx].texcoord = glm::vec2(norm_x, norm_z);
        }
    }

    // 2. Generate Triangle Index Buffer Topology
    indices.clear();
    indices.reserve((res - 1) * (res - 1) * 6);

    for (int y = 0; y < res - 1; ++y) {
        for (int x = 0; x < res - 1; ++x) {
            unsigned int top_left     = y * res + x;
            unsigned int top_right    = top_left + 1;
            unsigned int bottom_left  = (y + 1) * res + x;
            unsigned int bottom_right = bottom_left + 1;

            // First Triangle
            indices.push_back(top_left);
            indices.push_back(bottom_left);
            indices.push_back(top_right);

            // Second Triangle
            indices.push_back(top_right);
            indices.push_back(bottom_left);
            indices.push_back(bottom_right);
        }
    }

    index_count = static_cast<int>(indices.size());

    // 3. Setup OpenGL State & Buffers
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    // Allocate dynamic VBO
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_DYNAMIC_DRAW);

    // Allocate static EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Position Attribute (Location 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

    // Normal Attribute (Location 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

    // TexCoord Attribute (Location 2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texcoord));

    glBindVertexArray(0);
}

void WaterMesh::update(const WaterSimulation& sim) {
    int res = sim.get_resolution();
    float domain = sim.get_domain_size();
    float spacing = domain / static_cast<float>(res - 1);

    for (int y = 0; y < res; ++y) {
        for (int x = 0; x < res; ++x) {
            int idx = y * res + x;
            vertices[idx].position.y = sim.get_height(x, y);

            float h_left  = sim.get_height(std::max(0, x - 1), y);
            float h_right = sim.get_height(std::min(res - 1, x + 1), y);
            float h_down  = sim.get_height(x, std::max(0, y - 1));
            float h_up    = sim.get_height(x, std::min(res - 1, y + 1));

            // Central difference slope
            float dh_dx = (h_right - h_left) / (2.0f * spacing);
            float dh_dz = (h_up - h_down) / (2.0f * spacing);

            vertices[idx].normal = glm::normalize(glm::vec3(-dh_dx, 1.0f, -dh_dz));
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(Vertex), vertices.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void WaterMesh::render() const {
    if (vao == 0 || index_count == 0) return;

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}