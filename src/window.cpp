#include "window.h"
#include <stdexcept>

Window::Window(int width, int height, const std::string& title, bool fullscreen) {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWmonitor* monitor = nullptr;

    if (fullscreen) {
        monitor = glfwGetPrimaryMonitor();
        if (monitor) {
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            if (mode) {
                width = mode->width;
                height = mode->height;
            }
        }
    }

    window_handle = glfwCreateWindow(width, height, title.c_str(), monitor, nullptr);
    if (!window_handle) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(window_handle);
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