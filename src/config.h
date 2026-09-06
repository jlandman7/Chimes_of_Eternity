// config.h
#pragma once
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include "water_simulation.h"

// Helper to generate timestamped output directory
inline std::string get_timestamped_output_dir(const std::string& base_dir) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm* timeinfo = std::localtime(&time);
    
    std::ostringstream oss;
    oss << base_dir << "/wpm_" 
        << std::put_time(timeinfo, "%Y-%m-%d_%H-%M-%S");
    return oss.str();
}

enum class PresetQuality { Preview, HighQuality };

// Input: mutually exclusive
enum class InputMode { Sim, PreSimulated };

// Output: bitmask (combinable)
enum class OutputMode {
    Full        = 1 << 0,   // Photon-mapped render
    Height      = 1 << 1,   // Height field visualization
    Photons     = 1 << 2    // Photon distribution visualization
};

enum class LightingMode {
    BlinnPhong,
    PhotonMapping
};

// Window: which visualization to display (if any)
enum class WindowVisualization { None, Full, Height, Photons };

struct SimulationConfig {
    float timestep;
    int sim_steps_per_frame;
    float heightfield_variance;
    int heightfield_resolution;
};

struct PhotonMappingConfig {
    int photon_count;
    int photon_bounces;
    float photon_gather_radius;
};

struct RenderingConfig {
    int output_width;
    int output_height;
    float output_fps;
    bool enable_gamma_correction;
    bool enable_tone_mapping;
};

struct VideoOutputConfig {
    std::string output_directory;      // Must exist; we error if missing
    std::string output_filename_base;  // e.g., "water_sim" -> creates water_sim_full.mp4, water_sim_height.mp4, etc.
    bool save_png_sequence;
};

struct DebugConfig {
    WindowVisualization window_mode;   // What to show in window (None = no window)
    LightingMode lighting_mode;
    bool verbose_logging;
};

struct AppConfig {
    InputMode input_mode;
    int output_modes;                  // Bitmask of OutputMode
    
    PresetQuality preset;
    int frame_count;
    std::string dae_file;              // Optional; empty = default scene
    std::string pre_simulated_cache;   // Only used if input_mode == PreSimulated
    
    LightingMode lighting;
    WaterSimulationConfig water;
    SimulationConfig sim;
    PhotonMappingConfig photons;
    RenderingConfig render;
    VideoOutputConfig video;
    DebugConfig debug;
    
    void print() const;
};

AppConfig parse_cli(int argc, char* argv[]);