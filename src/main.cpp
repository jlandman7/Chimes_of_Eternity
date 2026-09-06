#include "window.h"
#include "water_simulation.h"
#include "water_mesh.h"
#include "renderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <chrono>

int main() {
    try {
        // Initialize window in native full screen mode
        Window window(1920, 1080, "Water Surface in the Abyss", true);

        WaterSimulationConfig sim_config;
        sim_config.grid_resolution = 128;
        sim_config.domain_size = 1.0f;

        // 1. Slow down wave propagation speed (was 1.5f)
        sim_config.wave_speed = 0.6f;

        // 2. Increase energy loss per step to smooth high-frequency ringing (was 0.985f)
        sim_config.damping = 0.978f;

        sim_config.simulation_timestep = 0.016f;

        WaterSimulation sim(sim_config);
        sim.initialize();

        // 3. Widen drop radius (0.05f -> 0.15f) for longer wavelengths, lower initial impulse (0.02f -> 0.008f)
        sim.add_drop(0.5f, 0.5f, 0.15f, 0.008f);

        WaterMesh mesh;
        mesh.initialize(sim);

        Renderer renderer;
        if (!renderer.init()) {
            throw std::runtime_error("Failed to initialize OpenGL renderer");
        }

        glEnable(GL_DEPTH_TEST);

        auto last_drop_time = std::chrono::steady_clock::now();

        while (!window.should_close()) {
            window.poll_events();

            // Inside the main loop:
            if (glfwGetKey(window.get_native_window(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                glfwSetWindowShouldClose(window.get_native_window(), true);
            }

            sim.step();
            mesh.update(sim);

            // DEBUG //
            // Periodically trigger subtle test drops
            auto current_time = std::chrono::steady_clock::now();
            float elapsed = std::chrono::duration<float>(current_time - last_drop_time).count();
            if (elapsed > 3.0f) { // Increase interval between drops (was 2.0s)
                float rx = 0.25f + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 0.5f;
                float ry = 0.25f + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 0.5f;
                
                // Smooth, low-frequency swell impulse
                sim.add_drop(rx, ry, 0.14f, 0.006f);
                last_drop_time = current_time;
            }

            int display_w, display_h;
            window.get_framebuffer_size(&display_w, &display_h);
            glViewport(0, 0, display_w, display_h);

            

            

            glClearColor(0.002f, 0.003f, 0.005f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Overhead Camera looking straight down
            glm::vec3 cam_pos(0.0f, 2.0f, 0.0f);
            glm::mat4 view = glm::lookAt(cam_pos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));

            // Orthographic projection fills the window frame edge-to-edge
            glm::mat4 proj = glm::ortho(-0.5f, 0.5f, -0.5f, 0.5f, 0.1f, 10.0f);

            renderer.render(mesh, view, proj, cam_pos);

            window.swap_buffers();
        }
    } 
    catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}