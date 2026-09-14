#include "water_simulation.h"
#include <algorithm>
#include <cmath>

WaterSimulation::WaterSimulation(const WaterSimulationConfig& config)
    : config(config) {}

void WaterSimulation::initialize() {
    int size = config.grid_resolution * config.grid_resolution;
    heights.assign(size, 0.0f);
    heights_prev.assign(size, 0.0f);
    heights_next.assign(size, 0.0f);
}

float WaterSimulation::laplacian(int x, int y) const {
    int res = config.grid_resolution;
    float center = heights[index(x, y)];

    float left  = (x > 0) ? heights[index(x - 1, y)] : center;
    float right = (x < res - 1) ? heights[index(x + 1, y)] : center;
    float down  = (y > 0) ? heights[index(x, y - 1)] : center;
    float up    = (y < res - 1) ? heights[index(x, y + 1)] : center;

    return left + right + down + up - 4.0f * center;
}

void WaterSimulation::step() {
    float dx = config.domain_size / static_cast<float>(config.grid_resolution - 1);
    
    // CFL stability adaptive sub-stepping[cite: 14]
    float max_safe_dt = 0.4f * dx / config.wave_speed;
    int substeps = std::max(1, static_cast<int>(std::ceil(config.simulation_timestep / max_safe_dt)));

    float sub_dt = config.simulation_timestep / static_cast<float>(substeps);
    float alpha = (config.wave_speed * sub_dt) / dx;
    float alpha_sq = alpha * alpha;
    float sub_damping = std::pow(config.damping, 1.0f / static_cast<float>(substeps));

    // Hoisted invariants out of the inner loop
    float radius_limit_sq = (config.domain_size * 0.48f) * (config.domain_size * 0.48f);
    float half_domain = config.domain_size * 0.5f;

    for (int s = 0; s < substeps; ++s) {
        for (int y = 0; y < config.grid_resolution; ++y) {
            float pz = -half_domain + y * dx;
            float pz_sq = pz * pz;

            for (int x = 0; x < config.grid_resolution; ++x) {
                int idx = index(x, y);

                float px = -half_domain + x * dx;
                
                // Clamp nodes outside circular boundary radius
                if (px * px + pz_sq >= radius_limit_sq) {
                    heights_next[idx] = 0.0f;
                    continue;
                }

                float lap = laplacian(x, y);
                float next_val = 2.0f * heights[idx] - heights_prev[idx] + alpha_sq * lap;
                heights_next[idx] = next_val * sub_damping;
            }
        }
        std::swap(heights_prev, heights);
        std::swap(heights, heights_next);
    }
}

void WaterSimulation::add_drop(float x, float z, float radius, float magnitude) {
    int center_x = static_cast<int>(((x / config.domain_size) + 0.5f) * config.grid_resolution);
    int center_z = static_cast<int>(((z / config.domain_size) + 0.5f) * config.grid_resolution);
    int r_cells = std::max(1, static_cast<int>((radius / config.domain_size) * config.grid_resolution));

    for (int dz = -r_cells; dz <= r_cells; ++dz) {
        for (int dx = -r_cells; dx <= r_cells; ++dx) {
            int gx = center_x + dx;
            int gz = center_z + dz;

            if (gx >= 0 && gx < config.grid_resolution && gz >= 0 && gz < config.grid_resolution) {
                float dist = std::sqrt(static_cast<float>(dx * dx + dz * dz));
                
                if (dist <= r_cells) {
                    float factor = 0.5f * (1.0f + std::cos(3.14159265f * dist / r_cells));
                    heights[gz * config.grid_resolution + gx] -= magnitude * factor;
                }
            }
        }
    }
}

float WaterSimulation::get_height_at(float x, float z) const {
    float half_d = config.domain_size * 0.5f;
    float norm_x = (x + half_d) / config.domain_size;
    float norm_z = (z + half_d) / config.domain_size;

    if (norm_x < 0.0f || norm_x > 1.0f || norm_z < 0.0f || norm_z > 1.0f) return 0.0f;

    float gx = norm_x * (config.grid_resolution - 1);
    float gz = norm_z * (config.grid_resolution - 1);

    int x0 = static_cast<int>(gx);
    int z0 = static_cast<int>(gz);
    int x1 = std::min(x0 + 1, config.grid_resolution - 1);
    int z1 = std::min(z0 + 1, config.grid_resolution - 1);

    float fx = gx - x0;
    float fz = gz - z0;

    float h00 = heights[index(x0, z0)];
    float h10 = heights[index(x1, z0)];
    float h01 = heights[index(x0, z1)];
    float h11 = heights[index(x1, z1)];

    return (1.0f - fx) * (1.0f - fz) * h00 +
           fx * (1.0f - fz) * h10 +
           (1.0f - fx) * fz * h01 +
           fx * fz * h11;
}

glm::vec3 WaterSimulation::get_normal_at(float x, float z) const {
    float eps = config.domain_size / static_cast<float>(config.grid_resolution - 1);
    float h_left  = get_height_at(x - eps, z);
    float h_right = get_height_at(x + eps, z);
    float h_down  = get_height_at(x, z - eps);
    float h_up    = get_height_at(x, z + eps);

    float dh_dx = (h_right - h_left) / (2.0f * eps);
    float dh_dz = (h_up - h_down) / (2.0f * eps);

    return glm::normalize(glm::vec3(-dh_dx, 1.0f, -dh_dz));
}