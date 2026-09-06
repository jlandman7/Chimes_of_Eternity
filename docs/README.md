# Final-Project-CS184


# Water Simulator

A physically-based water surface renderer using height field simulation and photon mapping, built as a final project for UC Berkeley's CS184 Computer Graphics course, Summer 2026.

### Group members
- Jacob Landman
- Howard Yao
- Tony Li
- Jordan Velasco

## Building

### macOS

```bash
brew install glfw glm cmake libomp

mkdir build && cd build
cmake ..
cmake --build . --config Release
./bin/water_sim --help
```

### Windows

```bash
# Using vcpkg (recommended)
vcpkg install glfw3:x64-windows glm:x64-windows

mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=<path-to-vcpkg>/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
.\bin\water_sim.exe --help
```

### Linux

```bash
sudo apt-get install libglfw3-dev libglm-dev cmake

mkdir build && cd build
cmake ..
cmake --build . --config Release
./bin/water_sim --help
```

## Usage

### Basic Example

---- make sure the output directory exists, it won't make it for you ----

```bash
./bin/water_sim --output full --output-dir ./output_dir --frames 30 --harmonics 4 --wave-speed 6 --water-init gaussian_drops --verbose --photons 50000 --output full
```

### CLI Flags Reference

#### Output Flags

- `--output <modes>` **[REQUIRED]**
  - Comma-separated list of output modes to generate, especially for debugging
  - Options: `full`, `height`, `photons`
  - Examples: `full`, `full,height`, `full,height,photons`
  - At least one must be specified

- `--output-dir <path>`
  - Directory where output files will be written
  - !!! Must exist; will error if missing !!!
  - Example: `--output-dir ./output`

#### Input Mode

- `--input <mode>` (default: `sim`)
  - Mutually exclusive input source
  - Options:
    - `sim`: Run new simulation
    - `pre-simulated`: Use cached simulation frames
  - Example: `--input pre-simulated`

- `--cache-dir <path>` (required if `--input pre-simulated`)
  - Directory containing pre-simulated height field frames
  - Example: `--cache-dir ./cached_sim`

These parameters were added early on but ended up not being used because computing the height field simulation is peanuts compared to the computational demand of photon mapping.

#### Simulation Parameters

- `--frames <n>` (default: `300`)
  - Number of frames to render
  - Example: `--frames 600`

- `--resolution <WxH>` (default: `1280x720`)
  - Output resolution in pixels
  - Format: `WIDTH` x `HEIGHT` (e.g., `1920x1080`)
  - Window resolution matches output resolution
  - Example: `--resolution 1920x1080`

- `--fps <f>` (default: `30.0`)
  - Output video framerate
  - Independent of simulation timestep
  - Example: `--fps 60`

- `--water-init <f>` (default: `cosine`)
  - Way of initially displacing the water
  - Options:
    - `cosine`, `gaussian_drops`, `directional` (broken)
  - Example: `--water-init gaussian_drops`

- `harmonics <i>` (default: 5)
  - controls number of cosine waves, gaussian drops...
  - Example: `--harmonics 11`

- `wave speed <f>` (default: 1)
  - controls speed of propagation of disturbances
  - Example: `--wave-speed 7`

#### Quality Presets

- `--preset <preset>` (default: `hq`)
  - Pre-configured quality settings
  - Options:
    - `preview`: Fast iteration (10k photons, 3 bounces, low resolution friendly)
    - `hq`: High quality (500k photons, 8 bounces)
  - Individual flags override preset values
  - Example: `--preset preview`

#### Photon Mapping

- `--photons <n>` (default: preset-dependent)
  - Number of photons to trace per frame
  - Overrides preset value
  - Example: `--photons 250000`

#### Window & Visualization

- `--window <mode>` (default: `full`)
  - Display mode for real-time visualization during rendering
  - Options:
    - `none`: No window (render headless)
    - `full`: Show final photon-mapped render
    - `height`: Show height field visualization
    - `photons`: Show photon distribution
  - Example: `--window height`

- `--lighting <f>` (default: `photon-mapping`)
  - Control the lighting mode, useful for debugging
  - Options: 
    - `blinn-phong`, `photon-mapping`

#### Output Files

- `--output-name <name>` (default: `water_sim`)
  - Base filename for output files (without extension)
  - Creates `<name>_full.mp4`, `<name>_height.mp4`, etc.
  - Example: `--output-name cornell_water`

#### Debugging & Logging

- `--verbose`
  - Print configuration summary before rendering
  - Example: `--verbose`

### Usage Examples

**Quick preview render:**
```bash
./bin/water_sim \
    --preset preview \
    --frames 100 \
    --output full \
    --output-dir ./output \
    --window full \
    --verbose
```

**High-quality final render with multiple outputs:**
```bash
./bin/water_sim \
    --preset hq \
    --frames 300 \
    --resolution 1920x1080 \
    --output full,height,photons \
    --output-dir ./output \
    --output-name water_final \
    --window none
```

**Debug height field visualization:**
```bash
./bin/water_sim \
    --preset preview \
    --frames 60 \
    --output height \
    --output-dir ./debug \
    --window height \
    --verbose
```

**Render from cached simulation:**
```bash
./bin/water_sim \
    --input pre-simulated \
    --cache-dir ./cached_sim \
    --output full,photons \
    --output-dir ./output \
    --photons 750000
```

**High framerate output with custom timestep:**
```bash
./bin/water_sim \
    --frames 600 \
    --fps 60 \
    --photons 100000 \
    --output full \
    --output-dir ./output \
    --output-name slowmo
```

### Output Files

After rendering completes:

- **PNG Sequence**: Intermediate frames saved to `<output-dir>/frames/<name>_*.png`
- **MP4 Video**: Final encoded video as `<output-dir>/<name>_<mode>.mp4`
  - `<name>_full.mp4`: Photon-mapped render
  - `<name>_height.mp4`: Height field visualization (if requested)
  - `<name>_photons.mp4`: Photon distribution visualization (if requested)

## Configuration Presets

### Preview Preset (`--preset preview`)

Optimized for fast iteration:
- Photon count: 10,000
- Photon bounces: 3
- Suitable for quick debugging and testing

### High Quality Preset (`--preset hq`)

Optimized for final output:
- Photon count: 500,000
- Photon bounces: 8
- Slower but produces high-fidelity results

All preset values can be overridden with explicit flags.

## Architecture

The project is structured into several core components:

- **Config System** (`config.h/cpp`): CLI parsing and configuration
- **Rendering** (`renderer.h/cpp`): OpenGL renderer and G-buffer management
- **Framebuffer** (`framebuffer.h/cpp`): Multi-attachment HDR framebuffer
- **Scene** (`cornell_box.h/cpp`): Procedural Cornell box and DAE loading
- **I/O** (`image_output.h/cpp`): Tone mapping and PNG/MP4 output
- **Window** (`window.h/cpp`): GLFW window management

Input and output modes are composable, allowing flexible rendering pipelines:
- Render multiple modes in one pass
- Cache intermediate results (simulation) for reuse
- Visualize debugging information independently

## Future Features

- [ ] DAE mesh loading for custom scenes
- [ ] Interactive camera controls
- [ ] Real-time parameter adjustment
- [ ] Advanced water simulation (Navier-Stokes)
- [ ] Caustic effects
- [ ] Volumetric scattering