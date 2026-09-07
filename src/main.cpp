#define GL_SILENCE_DEPRECATION
#include "window.h"
#include "water_simulation.h"
#include "water_mesh.h"
#include "renderer.h"
#include "audio_manager.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <cmath>
#include <algorithm>

struct AnimatedLight {
    glm::vec3 position;
    glm::vec3 color;
    float age = 0.0f;
    float decay = 1.0f; // Linked to audio tail        
    float peak_intensity = 0.7f;  

    float get_current_intensity() const {
        // Calculate the exact same curve the audio uses
        float raw_envelope = std::exp(-decay * age);
        
        // Remap it so it hits absolute 0.0 right as the audio hits 0.001
        float light_intensity = std::max(0.0f, (raw_envelope - 0.001f) / 0.999f);
        
        return light_intensity * peak_intensity;
    }
    
    bool is_dead() const { 
        return get_current_intensity() <= 0.0f; 
    }
};

struct ProcessTimer {
    float time_remaining = 0.0f;
    void sample_next(std::mt19937& gen, std::exponential_distribution<float>& dist) {
        time_remaining = dist(gen);
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

        AudioManager audio;
        if (!audio.init()) {
            std::cerr << "Warning: Failed to initialize DSP audio engine." << std::endl;
        }

        glEnable(GL_DEPTH_TEST);
        std::random_device rd;
        std::mt19937 gen(rd());

        std::exponential_distribution<float> arrival_dist(1.0f / 32.0f);
        std::normal_distribution<float> intensity_dist(0.60f, 0.12f);   
        std::uniform_real_distribution<float> pos_dist(-0.45f, 0.45f);
        std::uniform_real_distribution<float> height_dist(1.1f, 1.7f);

        // --- AUDIO DISTRIBUTIONS ---
        std::vector<float> scale = { 220.0f, 261.63f, 293.66f, 329.63f, 392.00f, 
                                     440.0f, 523.25f, 587.33f, 659.25f, 783.99f, 880.0f };
        std::uniform_int_distribution<int> pitch_dist(0, scale.size() - 1);
        std::uniform_real_distribution<float> audio_vol_dist(0.2f, 0.4f);
        std::uniform_real_distribution<float> audio_decay_dist(0.2f, 0.5f);
        std::uniform_int_distribution<int> harmonic_count_dist(1, 3);
        std::uniform_real_distribution<float> harmonic_ratio_dist(2.0f, 5.0f);
        std::uniform_real_distribution<float> harmonic_base_amp_dist(0.02f, 0.08f);

        // --- VISUAL DISTRIBUTIONS ---
        // Centered roughly around the original 0.16f size and 0.011f magnitude
        std::uniform_real_distribution<float> ripple_size_dist(0.10f, 0.25f);
        std::uniform_real_distribution<float> ripple_mag_dist(0.005f, 0.02f);

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

            for (int i = 0; i < NUM_TIMERS; ++i) {
                timers[i].time_remaining -= dt;

                if (timers[i].time_remaining <= 0.0f) {
                    float drop_x = pos_dist(gen);
                    float drop_z = pos_dist(gen);

                    // --- GENERATE AUDIO CHIME ---
                    ChimeConfig chime;
                    chime.pitch = scale[pitch_dist(gen)];
                    chime.volume = audio_vol_dist(gen);
                    chime.decay = audio_decay_dist(gen);
                    chime.pan = std::clamp(drop_x / 0.45f, -1.0f, 1.0f);

                    int num_harmonics = harmonic_count_dist(gen);
                    for(int h = 0; h < num_harmonics; ++h) {
                        float ratio = harmonic_ratio_dist(gen);
                        float amplitude = harmonic_base_amp_dist(gen) / (ratio * 0.5f); 
                        chime.harmonics.push_back({ratio, amplitude});
                    }
                    audio.play_chime(chime);

                    // --- GENERATE RIPPLES ---
                    float current_ripple_size = ripple_size_dist(gen);
                    float current_ripple_mag = ripple_mag_dist(gen);
                    sim.add_drop(drop_x, drop_z, current_ripple_size, current_ripple_mag);

                    // --- GENERATE SYNCHRONIZED LIGHT ---
                    AnimatedLight light;
                    float offset_x = pos_dist(gen) * 0.3f;
                    float offset_z = pos_dist(gen) * 0.3f;
                    
                    light.position = glm::vec3(drop_x + offset_x, height_dist(gen), drop_z + offset_z);
                    light.color = generate_vibrant_color(gen);
                    light.peak_intensity = std::clamp(intensity_dist(gen), 0.15f, 1.0f);
                    light.decay = chime.decay; // Perfectly matches audio tail
                    
                    lights.push_back(light);

                    timers[i].sample_next(gen, arrival_dist);
                }
            }

            std::vector<LightData> active_light_data;
            for (auto it = lights.begin(); it != lights.end(); ) {
                it->age += dt;
                float intensity = it->get_current_intensity();

                if (it->is_dead()) {
                    it = lights.erase(it);
                } else {
                    if (intensity > 0.0f) {
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