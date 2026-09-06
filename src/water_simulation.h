#pragma once
#include <vector>
#include <algorithm>
#include <cmath>

struct WaterSimulationConfig {
    int grid_resolution = 128;
    float domain_size = 1.0f;
    float wave_speed = 2.0f;
    float damping = 0.995f;
    float simulation_timestep = 0.016f;
};

class WaterSimulation {
public:
    explicit WaterSimulation(const WaterSimulationConfig& config);
    ~WaterSimulation() = default;

    void initialize();
    void step();

    // Creates a smooth ripple perturbation at normalized domain coordinates [0, 1]
    void add_drop(float norm_x, float norm_y, float radius, float strength);

    // Grid queries
    float get_height(int x, int y) const;
    const std::vector<float>& get_heights() const { return heights; }
    int get_resolution() const { return resolution; }
    float get_domain_size() const { return domain_size; }

private:
    WaterSimulationConfig config;
    int resolution;
    float domain_size;
    float wave_speed;
    float damping;
    float dt;

    // Persistent double-buffered height arrays[cite: 21]
    std::vector<float> heights;      // Current timestep (t)
    std::vector<float> heights_prev; // Previous timestep (t - dt)
    std::vector<float> heights_next; // Buffer for (t + dt) computation

    inline int index(int x, int y) const { return y * resolution + x; }
    float laplacian(int x, int y) const;
};