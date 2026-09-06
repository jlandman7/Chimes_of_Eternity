#include "window.h"
#include "water_simulation.h"
#include "water_mesh.h"
#include "renderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <cmath>
#include <algorithm>

// Gaussian light profile centered at 4.5 seconds (9s total lifetime)
struct AnimatedLight {
    glm::vec3 position;
    glm::vec3 color;
    float age = 0.0f;
    float lifetime = 9.0f;        // Mean duration (~9s)
    float peak_intensity = 0.7f;  // Target max intensity (~70%)
    float peak_time = 4.5f;       // Gaussian bell curve peak at 4.5s
    float sigma = 1.50f;          // Spread (~6-sigma spans 0s to 9s)

    float get_current_intensity() const {
        if (age < 0.0f || age > lifetime) return 0.0f;
        float diff = age - peak_time;
        // Gaussian envelope: I(t) = A * exp(-(t - t_peak)^2 / (2 * sigma^2))
        return peak_intensity * std::exp(-(diff * diff) / (2.0f * sigma * sigma));
    }

    bool is_dead() const { return age >= lifetime; }
};

// Independent stochastic timer using an exponential inter-arrival distribution
struct ProcessTimer {
    float time_remaining = 0.0f;

    void sample_next(std::mt19937& gen, std::exponential_distribution<float>& dist) {
        time_remaining = dist(gen); // Mean interval = 30.0 seconds
    }
};

static glm::vec3 generate_vibrant_color(std::mt19937& gen) {
    std::uniform_real_distribution<float> hue_dist(0.0f, 1.0f);
    std::uniform_real_distribution<float> sat_dist(0.60f, 0.95f);
    std::uniform_real_distribution<float> val_dist(0.75f, 1.00f);

    float h = hue_dist(gen);
    float s = sat_dist(gen);
    float v = val_dist(gen);

    int i = static_cast<int>(h * 6.0f);
    float f = h * 6.0f - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - f * s);
    float t = v * (1.0f - (1.0f - f) * s);

    switch (i % 6) {
        case 0: return glm::vec3(v, t, p);
        case 1: return glm::vec3(q, v, p);
        case 2: return glm::vec3(p, v, t);
        case 3: return glm::vec3(p, q, v);
        case 4: return glm::vec3(t, p, v);
        case 5: return glm::vec3(v, p, q);
    }
    return glm::vec3(1.0f);
}

int main() {
    try {
        Window window(1920, 1080, "Water Surface in the Abyss", true);

        WaterSimulationConfig sim_config;
        sim_config.grid_resolution = 160;
        sim_config.domain_size = 2.4f;
        sim_config.wave_speed = 0.50f;
        sim_config.damping = 0.988f;
        sim_config.simulation_timestep = 0.016f;

        WaterSimulation sim(sim_config);
        sim.initialize();

        WaterMesh mesh;
        mesh.initialize(sim);

        Renderer renderer;
        if (!renderer.init()) {
            throw std::runtime_error("Failed to initialize OpenGL renderer");
        }

        glEnable(GL_DEPTH_TEST);

        // Random Number Generators & Standard Distributions
        std::random_device rd;
        std::mt19937 gen(rd());

        // Poisson process rate lambda = 1.0 / 30.0 (mean interval = 30s per thread)
        std::exponential_distribution<float> arrival_dist(1.0f / 40.0f);
        
        // Gaussian distributions for light properties (centered around 9s total lifetime)
        std::normal_distribution<float> duration_dist(9.0f, 1.2f);      // Mean 9.0s duration
        std::normal_distribution<float> intensity_dist(0.70f, 0.12f);   // Gaussian centered at 70% max
        std::uniform_real_distribution<float> pos_dist(-0.45f, 0.45f);
        std::uniform_real_distribution<float> height_dist(1.1f, 1.7f);

        // 8 parallel independent process timers
        constexpr int NUM_TIMERS = 8;
        ProcessTimer timers[NUM_TIMERS];
        for (int i = 0; i < NUM_TIMERS; ++i) {
            timers[i].sample_next(gen, arrival_dist);
        }

        std::vector<AnimatedLight> lights;
        auto last_frame_time = std::chrono::steady_clock::now();

        while (!window.should_close()) {
            window.poll_events();

            if (glfwGetKey(window.get_native_window(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                glfwSetWindowShouldClose(window.get_native_window(), true);
            }

            auto current_time = std::chrono::steady_clock::now();
            float dt = std::chrono::duration<float>(current_time - last_frame_time).count();
            last_frame_time = current_time;

            int display_w, display_h;
            window.get_framebuffer_size(&display_w, &display_h);
            glViewport(0, 0, display_w, display_h);

            float aspect = static_cast<float>(display_w) / static_cast<float>(display_h > 0 ? display_h : 1);

            // Step process timers
            for (int i = 0; i < NUM_TIMERS; ++i) {
                timers[i].time_remaining -= dt;

                if (timers[i].time_remaining <= 0.0f) {
                    float drop_x = pos_dist(gen);
                    float drop_z = pos_dist(gen);

                    sim.add_drop(drop_x, drop_z, 0.16f, 0.011f);

                    AnimatedLight light;
                    float offset_x = pos_dist(gen) * 0.3f;
                    float offset_z = pos_dist(gen) * 0.3f;
                    
                    light.position = glm::vec3(drop_x + offset_x, height_dist(gen), drop_z + offset_z);
                    light.color = generate_vibrant_color(gen);
                    light.lifetime = std::max(4.0f, duration_dist(gen));
                    light.peak_time = 4.5f;                     // Explicit peak centered at 4.5 seconds
                    light.peak_intensity = std::clamp(intensity_dist(gen), 0.15f, 1.0f);
                    light.sigma = light.lifetime / 6.0f;        // 3-sigma left, 3-sigma right

                    lights.push_back(light);

                    timers[i].sample_next(gen, arrival_dist);
                }
            }

            // Advance lights and calculate Gaussian intensity profile
            std::vector<LightData> active_light_data;
            for (auto it = lights.begin(); it != lights.end(); ) {
                it->age += dt;
                float intensity = it->get_current_intensity();

                if (it->is_dead()) {
                    it = lights.erase(it);
                } else {
                    if (intensity > 0.001f) {
                        active_light_data.push_back({ it->position, it->color, intensity });
                    }
                    ++it;
                }
            }

            sim.step();
            mesh.update(sim);

            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glm::vec3 cam_pos(0.0f, 2.0f, 0.0f);
            glm::mat4 view = glm::lookAt(cam_pos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));

            glm::mat4 proj;
            if (aspect >= 1.0f) {
                proj = glm::ortho(-0.5f * aspect, 0.5f * aspect, -0.5f, 0.5f, 0.1f, 10.0f);
            } else {
                proj = glm::ortho(-0.5f, 0.5f, -0.5f / aspect, 0.5f / aspect, 0.1f, 10.0f);
            }

            renderer.render(mesh, view, proj, cam_pos, active_light_data);

            window.swap_buffers();
        }
    } 
    catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}