#pragma once
#include "miniaudio.h"
#include <vector>
#include <mutex>
#include <utility>

struct ChimeConfig {
    float pitch;        
    float volume;       
    float decay;        
    float pan = 0.0f;   // -1.0 (Left) to 1.0 (Right)
    
    std::vector<std::pair<float, float>> harmonics; 
};

// Internal tracker for a chime currently playing
struct ActiveChime {
    ChimeConfig config;
    float time_elapsed = 0.0f;
    bool is_dead = false;
};

class AudioManager {
public:
    AudioManager() = default;
    ~AudioManager();

    bool init();
    void play_chime(const ChimeConfig& config);

private:
    ma_device device;
    bool initialized = false;
    
    std::mutex audio_mutex; // Protects the vector when adding chimes from main thread
    std::vector<ActiveChime> active_chimes;

    // The callback Miniaudio fires continuously to request raw audio samples
    static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
};
