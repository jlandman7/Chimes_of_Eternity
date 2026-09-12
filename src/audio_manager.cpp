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
    config.playback.format   = ma_format_f32; // Floating point audio (-1.0 to 1.0)
    config.playback.channels = 2;             // Stereo
    config.sampleRate        = 44100;         // CD-quality sample rate
    config.dataCallback      = data_callback;
    config.pUserData         = this;          // Pass our class instance into the C-callback

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
    active_chimes.push_back({config, 0.0f, false});
}

void AudioManager::data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    AudioManager* manager = static_cast<AudioManager*>(pDevice->pUserData);
    float* pOutputF32 = static_cast<float*>(pOutput);
    float sample_rate = static_cast<float>(pDevice->sampleRate);
    
    for (ma_uint32 i = 0; i < frameCount * pDevice->playback.channels; ++i) {
        pOutputF32[i] = 0.0f;
    }

    std::lock_guard<std::mutex> lock(manager->audio_mutex);
    
    for (auto& chime : manager->active_chimes) {
        if (chime.is_dead) continue;

        // --- CONSTANT POWER PANNING MATH ---
        // Convert pan from [-1.0, 1.0] to a [0.0, 1.0] scale
        float pan_mapped = (chime.config.pan + 1.0f) * 0.5f; 
        
        // Map to a quarter circle (0 to PI/2 radians)
        float pan_angle = pan_mapped * (M_PI / 2.0f);
        
        // Cosine/Sine ensures total acoustic energy remains constant
        float left_gain = std::cos(pan_angle);
        float right_gain = std::sin(pan_angle);
        // -----------------------------------

        for (ma_uint32 i = 0; i < frameCount; ++i) {
            float t = chime.time_elapsed;
            
            // 1. Calculate ONE master envelope for the entire chime
            float master_envelope = std::exp(-chime.config.decay * t);
            
            // 2. Gate the chime based ONLY on the master envelope
            if (master_envelope < 0.001f) {
                chime.is_dead = true;
                break; 
            }

            // 3. Sum the raw continuous waveforms
            float raw_waveform = std::sin(2.0f * M_PI * chime.config.pitch * t);
            
            for (const auto& h : chime.config.harmonics) {
                raw_waveform += h.second * std::sin(2.0f * M_PI * (chime.config.pitch * h.first) * t);
            }

            // 4. Apply the envelope and volume to the summed wave
            float final_sample = raw_waveform * master_envelope * chime.config.volume;

            // Apply spatial gains to left and right channels
            pOutputF32[i * 2 + 0] += final_sample * left_gain;  // Left 
            pOutputF32[i * 2 + 1] += final_sample * right_gain; // Right 

            chime.time_elapsed += 1.0f / sample_rate;
        }
    }

    manager->active_chimes.erase(
        std::remove_if(manager->active_chimes.begin(), manager->active_chimes.end(),
            [](const ActiveChime& c) { return c.is_dead; }),
        manager->active_chimes.end()
    );
}