#pragma once

#ifdef _WIN32
    #include <GL/gl.h>
#else
    #include <OpenGL/gl3.h>
#endif
#include <GLFW/glfw3.h>
#include <string>

class Window {
public:
    Window(int width, int height, const std::string& title, bool fullscreen = false);
    ~Window();

    // Prevent copying
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool should_close() const;
    void swap_buffers();
    void poll_events();
    void get_framebuffer_size(int* width, int* height) const;

    GLFWwindow* get_native_window() const { return window_handle; }

private:
    GLFWwindow* window_handle = nullptr;
};