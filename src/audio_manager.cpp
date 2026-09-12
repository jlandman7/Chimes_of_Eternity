#define MINIAUDIO_IMPLEMENTATION
#include "audio_manager.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

AudioManager::~AudioManager() {
    if (initialized) {
        ma_device_uninit(&device);
    }
}

bool AudioManager::init() {
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = ma_format_f32; 
    config.playback.channels = 2;             
    config.sampleRate        = 44100;         
    config.dataCallback      = data_callback;
    config.pUserData         = this;          

    if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
        return false;
    }

    if (ma_device_start(&device) != MA_SUCCESS) {
        ma_device_uninit(&device);
        return false;
    }

    initialized = true;
    return true;
}

void AudioManager::play_chime(const ChimeConfig& config) {
    std::lock_guard<std::mutex> lock(audio_mutex);
    active_chimes.push_back({config, 0.0, false});
}

void AudioManager::data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    AudioManager* manager = static_cast<AudioManager*>(pDevice->pUserData);
    float* pOutputF32 = static_cast<float*>(pOutput);
    double dt = 1.0 / static_cast<double>(pDevice->sampleRate);
    
    for (ma_uint32 i = 0; i < frameCount * pDevice->playback.channels; ++i) {
        pOutputF32[i] = 0.0f;
    }

    std::lock_guard<std::mutex> lock(manager->audio_mutex);
    
    for (auto& chime : manager->active_chimes) {
        if (chime.is_dead) continue;

        float pan_mapped = (chime.config.pan + 1.0f) * 0.5f; 
        float pan_angle = pan_mapped * (M_PI / 2.0f);
        float left_gain = std::cos(pan_angle);
        float right_gain = std::sin(pan_angle);

        double t = chime.time_elapsed;

        for (ma_uint32 i = 0; i < frameCount; ++i) {
            double master_envelope = std::exp(-static_cast<double>(chime.config.decay) * t);
            
            if (master_envelope < 0.001) {
                chime.is_dead = true;
                break; 
            }

            // High-precision phase math
            double raw_waveform = std::sin(2.0 * M_PI * static_cast<double>(chime.config.pitch) * t);
            
            for (const auto& h : chime.config.harmonics) {
                raw_waveform += static_cast<double>(h.second) * 
                                std::sin(2.0 * M_PI * static_cast<double>(chime.config.pitch * h.first) * t);
            }

            // Cast back down to float only at the final amplification stage
            float final_sample = static_cast<float>(raw_waveform * master_envelope * chime.config.volume);

            pOutputF32[i * 2 + 0] += final_sample * left_gain;  
            pOutputF32[i * 2 + 1] += final_sample * right_gain; 

            t += dt;
        }
        
        chime.time_elapsed = t;
    }

    manager->active_chimes.erase(
        std::remove_if(manager->active_chimes.begin(), manager->active_chimes.end(),
            [](const ActiveChime& c) { return c.is_dead; }),
        manager->active_chimes.end()
    );
}