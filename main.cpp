#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

namespace
{

struct glfw {
    static glfw
    instantiate()
    {
        bool success = glfwInit() == GLFW_TRUE;
        if (success) {
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);// needed for mac
        }
        return {success};
    }
    bool valid;
    ~glfw()
    {
        if (valid) {
            glfwTerminate();
        }
    }
};

struct window {
    static window
    create()
    {
        auto w = glfwCreateWindow(800, 600, "LearnOpenGL", nullptr, nullptr);
        if (w) {
            glfwMakeContextCurrent(w);
        }
        return {w, 800, 600};
    }
    GLFWwindow *window;
    int width;
    int height;
    [[nodiscard]] bool
    make_current() const
    {
        if (window) {
            glfwMakeContextCurrent(window);
        }
        else {
            std::cerr << "Failed to create GLFW window" << std::endl;
        }
        return window != nullptr;
    }
};

struct glad {
    static glad
    initialise()
    {
        return {static_cast<bool>(gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))};
    }
    bool valid;
    [[nodiscard]] bool
    check() const
    {
        if (!valid) {
            std::cerr << "Failed to initialize GLAD" << std::endl;
        }
        return valid;
    }
};

struct viewport {
    static viewport
    initialise(const window &w)
    {
        glViewport(0, 0, w.width, w.height);
        glfwSetFramebufferSizeCallback(w.window, viewport::framebuffer_size_callback);
        return {};
    }
    static void
    framebuffer_size_callback(GLFWwindow *, int width, int height)
    {
        glViewport(0, 0, width, height);
    }
};

void
process_input(const window &w)
{
    if (glfwGetKey(w.window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(w.window, true);
    }
}

void
clear_color_buffer()
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void
render_loop(const window &w)
{
    while (!glfwWindowShouldClose(w.window)) {
        process_input(w);

        clear_color_buffer();

        glfwSwapBuffers(w.window);
        glfwPollEvents();
    }
}

}// namespace

int
main()
{
    auto glfw = glfw::instantiate();

    auto window = window::create();
    if (!window.make_current()) return -1;

    auto glad = glad::initialise();
    if (!glad.check()) return -1;

    viewport::initialise(window);

    render_loop(window);

    return 0;
}
