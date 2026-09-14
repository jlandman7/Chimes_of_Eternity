cmake_minimum_required(VERSION 3.14) # Minimum bumped slightly for FetchContent features
project(Chimes_of_Eternity)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)

# ---------------------------------------------------------
# Automatically download dependencies
# ---------------------------------------------------------
include(FetchContent)

# Fetch GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.8
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# Fetch GLM
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 0.9.9.8
)
FetchContent_MakeAvailable(glm)

# Define the executable target
add_executable(chimes_of_eternity
    src/main.cpp
    src/window.cpp
    src/water_simulation.cpp
    src/water_mesh.cpp
    src/renderer.cpp
    src/audio_manager.cpp
)

# Link external libraries
target_link_libraries(chimes_of_eternity PRIVATE
    OpenGL::GL
    glfw
    glm::glm
)

# Link external libraries
target_link_libraries(Chimes_of_Eternity PRIVATE
    OpenGL::GL
    glfw
)

# macOS specific system frameworks required for windowing
if(APPLE)
    target_link_libraries(Chimes_of_Eternity PRIVATE
        "-framework IOKit"
        "-framework CoreFoundation"
    )
endif()

# Linux specific system frameworks required for threading and math
if(UNIX AND NOT APPLE)
    target_link_libraries(Chimes_of_Eternity PRIVATE m pthread dl)
endif()

# ---------------------------------------------------------
# Packaging Instructions
# ---------------------------------------------------------

# Fixed target name from 'your_app_name' to 'Chimes'
install(TARGETS Chimes_of_Eternity DESTINATION bin)

# Uncomment and adjust this once you have actual assets to bundle
# install(DIRECTORY assets/ DESTINATION bin/assets)

set(CPACK_PACKAGE_NAME "Chimes_of_Eternity") # Matched to project name
set(CPACK_PACKAGE_VERSION "1.0.0")

if(WIN32)
    set(CPACK_GENERATOR "ZIP")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop") 
else()
    set(CPACK_GENERATOR "TGZ") 
endif()

include(CPack)