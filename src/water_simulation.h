#pragma once
#include <vector>
#include <glm/glm.hpp>

struct WaterSimulationConfig {
    int grid_resolution = 160;
    float domain_size = 2.4f;
    float wave_speed = 0.55f;
    float damping = 0.985f;
    float simulation_timestep = 0.016f;
};

class WaterSimulation {
public:
    explicit WaterSimulation(const WaterSimulationConfig& config);
    void initialize();
    void step();
    void add_drop(float center_x, float center_z, float radius, float strength);

    float get_height_at(float x, float z) const;
    glm::vec3 get_normal_at(float x, float z) const;

    int get_resolution() const { return config.grid_resolution; }
    float get_domain_size() const { return config.domain_size; }

private:
    WaterSimulationConfig config;
    std::vector<float> heights;
    std::vector<float> heights_prev;
    std::vector<float> heights_next;

    inline int index(int x, int y) const { return y * config.grid_resolution + x; }
    float laplacian(int x, int y) const;
};