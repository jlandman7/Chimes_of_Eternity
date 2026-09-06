#include "water_mesh.h"
#include <cmath>

WaterMesh::~WaterMesh() {
    if (vao) glDeleteVertexArrays(1, &vao);
    if (vbo) glDeleteBuffers(1, &vbo);
    if (ebo) glDeleteBuffers(1, &ebo);
}

void WaterMesh::initialize(const WaterSimulation& sim) {
    vertices.clear();
    indices.clear();

    float radius = sim.get_domain_size() * 0.48f;
    int rings = 96;
    int slices = 160;

    // Center vertex
    Vertex center_vert{};
    center_vert.position = glm::vec3(0.0f, 0.0f, 0.0f);
    center_vert.normal = glm::vec3(0.0f, 1.0f, 0.0f);
    center_vert.texcoord = glm::vec2(0.5f, 0.5f);
    vertices.push_back(center_vert);

    // Polar rings
    for (int r = 1; r <= rings; ++r) {
        float current_r = radius * (static_cast<float>(r) / static_cast<float>(rings));
        for (int s = 0; s < slices; ++s) {
            float theta = 2.0f * 3.14159265f * static_cast<float>(s) / static_cast<float>(slices);
            float x = current_r * std::cos(theta);
            float z = current_r * std::sin(theta);

            Vertex v{};
            v.position = glm::vec3(x, 0.0f, z);
            v.normal = glm::vec3(0.0f, 1.0f, 0.0f);
            v.texcoord = glm::vec2((x / (2.0f * radius)) + 0.5f, (z / (2.0f * radius)) + 0.5f);
            vertices.push_back(v);
        }
    }

    // Center fan indices
    for (int s = 0; s < slices; ++s) {
        int next_s = (s + 1) % slices;
        indices.push_back(0);
        indices.push_back(1 + s);
        indices.push_back(1 + next_s);
    }

    // Ring quad indices
    for (int r = 0; r < rings - 1; ++r) {
        int inner_start = 1 + r * slices;
        int outer_start = 1 + (r + 1) * slices;

        for (int s = 0; s < slices; ++s) {
            int next_s = (s + 1) % slices;

            int i0 = inner_start + s;
            int i1 = inner_start + next_s;
            int i2 = outer_start + s;
            int i3 = outer_start + next_s;

            indices.push_back(i0);
            indices.push_back(i2);
            indices.push_back(i1);

            indices.push_back(i1);
            indices.push_back(i2);
            indices.push_back(i3);
        }
    }

    index_count = static_cast<GLsizei>(indices.size());

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texcoord));

    glBindVertexArray(0);
}

void WaterMesh::update(const WaterSimulation& sim) {
    for (auto& v : vertices) {
        v.position.y = sim.get_height_at(v.position.x, v.position.z);
        v.normal = sim.get_normal_at(v.position.x, v.position.z);
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(Vertex), vertices.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void WaterMesh::render() const {
    if (!vao) return;
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}