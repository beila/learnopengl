#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

namespace
{
struct glfw {
    static glfw
    instantiate()
    {
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);// needed for mac
        return {};
    }
    ~glfw()
    {
        glfwTerminate();
    }
};
struct window {
    static window
    instantiate()
    {
        auto w = glfwCreateWindow(800, 600, "LearnOpenGL", nullptr, nullptr);
        if (w) {
            glfwMakeContextCurrent(w);
        }
        return {w};
    }
    GLFWwindow *window;
    explicit operator bool() const
    {
        if (window == nullptr) {
            std::cerr << "Failed to create GLFW window" << std::endl;
        }
        return window == nullptr;
    }
};
}// namespace
int
main()
{
    glfw glfw = glfw::instantiate();

    window window = window::instantiate();
    if (window) return -1;

    return 0;
}
