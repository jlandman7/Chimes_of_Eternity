#include "water_simulation.h"

WaterSimulation::WaterSimulation(const WaterSimulationConfig& config)
    : config(config),
      resolution(config.grid_resolution),
      domain_size(config.domain_size),
      wave_speed(config.wave_speed),
      damping(config.damping),
      dt(config.simulation_timestep) {
    
    size_t total_cells = static_cast<size_t>(resolution * resolution);
    heights.assign(total_cells, 0.0f);
    heights_prev.assign(total_cells, 0.0f);
    heights_next.assign(total_cells, 0.0f);
}

void WaterSimulation::initialize() {
    std::fill(heights.begin(), heights.end(), 0.0f);
    std::fill(heights_prev.begin(), heights_prev.end(), 0.0f);
    std::fill(heights_next.begin(), heights_next.end(), 0.0f);
}

void WaterSimulation::add_drop(float norm_x, float norm_y, float radius, float strength) {
    int center_x = static_cast<int>(norm_x * (resolution - 1));
    int center_y = static_cast<int>(norm_y * (resolution - 1));
    int cell_radius = static_cast<int>(radius * resolution);

    int min_x = std::max(0, center_x - cell_radius);
    int max_x = std::min(resolution - 1, center_x + cell_radius);
    int min_y = std::max(0, center_y - cell_radius);
    int max_y = std::min(resolution - 1, center_y + cell_radius);

    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {
            float dx = (static_cast<float>(x) / (resolution - 1)) - norm_x;
            float dy = (static_cast<float>(y) / (resolution - 1)) - norm_y;
            float dist = std::sqrt(dx * dx + dy * dy);

            if (dist < radius) {
                // Smooth cosine curve ensures zero derivative at drop radius boundary
                float falloff = 0.5f * (1.0f + std::cos(3.14159265f * dist / radius));
                heights[index(x, y)] += strength * falloff;
            }
        }
    }
}

float WaterSimulation::laplacian(int x, int y) const {
    float center = heights[index(x, y)];
    
    // Neumann boundary conditions (zero-gradient at edges)
    float left  = (x > 0) ? heights[index(x - 1, y)] : center;
    float right = (x < resolution - 1) ? heights[index(x + 1, y)] : center;
    float down  = (y > 0) ? heights[index(x, y - 1)] : center;
    float up    = (y < resolution - 1) ? heights[index(x, y + 1)] : center;

    return left + right + down + up - 4.0f * center;
}

void WaterSimulation::step() {
    float dx = domain_size / static_cast<float>(resolution - 1);
    
    // Enforce CFL stability limit: (c * dt_sub / dx) <= 0.4
    float max_safe_dt = 0.4f * dx / wave_speed;
    int substeps = static_cast<int>(std::ceil(dt / max_safe_dt));
    substeps = std::max(1, substeps);
    
    float sub_dt = dt / static_cast<float>(substeps);
    float alpha = (wave_speed * sub_dt) / dx;
    float alpha_sq = alpha * alpha;
    float sub_damping = std::pow(damping, 1.0f / static_cast<float>(substeps));

    for (int s = 0; s < substeps; ++s) {
        for (int y = 0; y < resolution; ++y) {
            for (int x = 0; x < resolution; ++x) {
                int idx = index(x, y);
                float lap = laplacian(x, y);
                
                float next_val = 2.0f * heights[idx] - heights_prev[idx] + alpha_sq * lap;
                heights_next[idx] = next_val * sub_damping;
            }
        }
        std::swap(heights_prev, heights);
        std::swap(heights, heights_next);
    }
}

float WaterSimulation::get_height(int x, int y) const {
    if (x < 0 || x >= resolution || y < 0 || y >= resolution) {
        return 0.0f;
    }
    return heights[index(x, y)];
}