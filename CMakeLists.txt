cmake_minimum_required(VERSION 3.10)
project(Chimes_of_Eternity)

# Enforce C++17 for standard library features
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Locate external dependencies
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED) # Change to 'glfw' if your package manager uses the non-versioned name

# Define the executable target and all compiled translation units
add_executable(Chimes
    src/main.cpp
    src/window.cpp
    src/water_simulation.cpp
    src/water_mesh.cpp
    src/renderer.cpp
    src/audio_manager.cpp
)

# Link external libraries to our target
target_link_libraries(Chimes PRIVATE
    OpenGL::GL
    glfw
    "-framework IOKit"
    "-framework CoreFoundation"
)

# On Linux, miniaudio and standard multithreading require explicit system libraries
if(UNIX AND NOT APPLE)
    target_link_libraries(Chimes PRIVATE m pthread dl)
endif()