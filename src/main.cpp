#include "window.h"
#include "water_simulation.h"
#include "water_mesh.h"
#include "renderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <chrono>
#include <vector>
#include <cstdlib>
#include <cmath>

struct AnimatedLight {
    glm::vec3 position;
    glm::vec3 color;
    float peak_intensity = 1.0f;
    float age = 0.0f;
    float lifetime = 5.0f;
    float rise_time = 1.2f;
    float decay_time = 2.0f;

    float get_current_intensity() const {
        if (age < 0.0f || age > lifetime) return 0.0f;
        if (age < rise_time) {
            float t = age / rise_time;
            return peak_intensity * (t * t * (3.0f - 2.0f * t)); // Smoothstep rise
        } else if (age > (lifetime - decay_time)) {
            float t = (lifetime - age) / decay_time;
            return peak_intensity * (t * t * (3.0f - 2.0f * t)); // Smoothstep decay
        }
        return peak_intensity;
    }

    bool is_dead() const { return age >= lifetime; }
};

// Helper to generate rich random colors
glm::vec3 generate_vibrant_color() {
    float h = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    float s = 0.65f + 0.35f * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX));
    float v = 0.85f + 0.15f * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX));

    // HSV to RGB conversion
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

        std::vector<AnimatedLight> lights;

        auto last_frame_time = std::chrono::steady_clock::now();
        float next_event_timer = 1.0f; // Initial delay before first drop

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

            // Update drop event scheduler
            next_event_timer -= dt;
            if (next_event_timer <= 0.0f) {
                bool is_dark_period = (rand() % 100) < 25; // 25% chance of silence/darkness period

                if (is_dark_period) {
                    // Extended delay with no new drops/lights
                    next_event_timer = 7.0f + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 5.0f;
                } else {
                    // Spawn 1 light (70% chance) or 2-3 simultaneous lights (30% chance)
                    int count = ((rand() % 100) < 30) ? (2 + (rand() % 2)) : 1;

                    for (int k = 0; k < count; ++k) {
                        float drop_x = -0.4f + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 0.8f;
                        float drop_z = -0.4f + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 0.8f;

                        sim.add_drop(drop_x, drop_z, 0.16f, 0.010f);

                        AnimatedLight light;
                        // Position light above the drop area at a non-grazing angle
                        float offset_x = -0.15f + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 0.30f;
                        float offset_z = -0.15f + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 0.30f;
                        float height   = 1.2f + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 0.6f;

                        light.position = glm::vec3(drop_x + offset_x, height, drop_z + offset_z);
                        light.color = generate_vibrant_color();
                        light.peak_intensity = 1.2f + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 0.8f;
                        light.lifetime = 4.5f + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 2.5f;
                        light.rise_time = 1.0f + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 0.6f;
                        light.decay_time = 1.8f + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 1.0f;

                        lights.push_back(light);
                    }

                    // Lower frequency delay between active events (4.5s to 8.0s)
                    next_event_timer = 4.5f + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 3.5f;
                }
            }

            // Update active lights
            std::vector<LightData> active_light_data;
            for (auto it = lights.begin(); it != lights.end(); ) {
                it->age += dt;
                float current_intensity = it->get_current_intensity();

                if (it->is_dead()) {
                    it = lights.erase(it);
                } else {
                    if (current_intensity > 0.0001f) {
                        active_light_data.push_back({ it->position, it->color, current_intensity });
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