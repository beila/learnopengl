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
    [[nodiscard]] static window
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
    [[nodiscard]] static glad
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

struct vertices_buffer_object {
    static vertices_buffer_object
    create(int size_vertices, float *vertices)
    {
        unsigned int VBO;
        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, size_vertices, vertices, GL_STATIC_DRAW);
        return {};
    }
};

struct shader {
    [[nodiscard]] static shader
    create(const char *const name, GLenum shaderType, const char *const shaderSource)
    {
        unsigned int shader_id = glCreateShader(shaderType);
        glShaderSource(shader_id, 1, &shaderSource, nullptr);
        glCompileShader(shader_id);
        return {shader_id, name};
    }
    unsigned int shader_id;
    const char *const name;
    [[nodiscard]] bool
    check() const
    {
        int success;
        char info_log[512];
        glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader_id, sizeof info_log, nullptr, info_log);
            std::cerr << "ERROR::SHADER::" << name << "::COMPILATION_FAILED\n"
                      << info_log << std::endl;
        }
        return static_cast<bool>(success);
    }
};

struct triangle {
    [[nodiscard]] static triangle
    create()
    {
        float vertices[] = {
            -0.5f, -0.5f, 0.0f,
            0.5f, -0.5f, 0.0f,
            0.0f, 0.5f, 0.0f};
        vertices_buffer_object::create(sizeof vertices, vertices);
        auto vertex_shader = shader::create("triangle_vertex", GL_VERTEX_SHADER, R"(
#version 330 core
layout (location = 0) in vec3 aPos;

void main()
{
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}
)");
        auto fragment_shader = shader::create("triangle_fragment", GL_FRAGMENT_SHADER, R"(
#version 330 core
out vec4 FragColor;

void main()
{
    FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
}
)");
        return {vertex_shader, fragment_shader};
    }
    shader vertex_shader;
    shader fragment_shader;
    [[nodiscard]] bool
    check() const
    {
        return vertex_shader.check() && fragment_shader.check();
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

    auto triangle = triangle::create();
    if (!triangle.check()) return -1;

    render_loop(window);

    return 0;
}
