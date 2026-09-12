#include "window.h"
#include <stdexcept>

Window::Window(int width, int height, const std::string& title, bool fullscreen) {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GL_TRUE);
#endif

    GLFWmonitor* monitor = nullptr;

    if (fullscreen) {
        monitor = glfwGetPrimaryMonitor();
        if (monitor) {
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            
            // Force an exact match to the monitor's native mode. 
            // This tells macOS to bypass the desktop compositor and drop all shadows.
            glfwWindowHint(GLFW_RED_BITS, mode->redBits);
            glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
            glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
            glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
            glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);
            
            width = mode->width;
            height = mode->height;
        }
    }

    // Passing the monitor here guarantees the menu bar is hidden via native Spaces
    window_handle = glfwCreateWindow(width, height, title.c_str(), monitor, nullptr);
    if (!window_handle) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(window_handle);
    glfwSetInputMode(window_handle, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    glfwSwapInterval(1);
}

Window::~Window() {
    if (window_handle) {
        glfwDestroyWindow(window_handle);
    }
    glfwTerminate();
}

bool Window::should_close() const {
    return glfwWindowShouldClose(window_handle);
}

void Window::swap_buffers() {
    glfwSwapBuffers(window_handle);
}

void Window::poll_events() {
    glfwPollEvents();
}

void Window::get_framebuffer_size(int* width, int* height) const {
    glfwGetFramebufferSize(window_handle, width, height);
}