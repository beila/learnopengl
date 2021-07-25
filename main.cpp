#include <glad/glad.h>

#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>

namespace
{

struct guard {
    guard(std::function<void()> &&closer) : closer(closer) {}// NOLINT(google-explicit-constructor)
    std::function<void()> closer;
    ~guard() { closer(); }
};

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
    const bool valid;
    ~glfw()
    {
        if (valid) {
            glfwTerminate();
        }
    }
};

struct glfw_window {
    [[nodiscard]] static glfw_window
    create(const glfw &)
    {
        return {glfwCreateWindow(800, 600, "LearnOpenGL", nullptr, nullptr)};
    }
    GLFWwindow *const handle;
    [[nodiscard]] bool
    make_current() const
    {
        if (handle) {
            glfwMakeContextCurrent(handle);
        }
        else {
            std::cerr << "Failed to create GLFW window" << std::endl;
        }
        return handle != nullptr;
    }
};

struct glad {
    [[nodiscard]] static glad
    initialise(const glfw &)
    {
        return {static_cast<bool>(gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))};
    }
    const bool valid;
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
    initialise(const glfw_window &w)
    {
        glfwSetFramebufferSizeCallback(w.handle, viewport::framebuffer_size_callback);
        return {w};
    }
    const glfw_window &window;
    static void
    framebuffer_size_callback(GLFWwindow *, int width, int height)
    {
//        std::cout << "glViewport(0, 0, " << width << ", " << height << ");" << std::endl;
        glViewport(0, 0, width, height);
    }
    ~viewport()
    {
        glfwSetFramebufferSizeCallback(window.handle, nullptr);
    }
};

struct vertex_buffer {
    static vertex_buffer
    create(std::vector<const float> &&vertices)
    {
        unsigned int VBO;
        glGenBuffers(1, &VBO);
        return {
            VBO,
            static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
            std::move(vertices),
        };
    }
    const unsigned int id;
    const GLsizeiptr size;
    const std::vector<const float> vertices;
    void
    bind() const
    {
        glBindBuffer(GL_ARRAY_BUFFER, id);
        glBufferData(GL_ARRAY_BUFFER, size, vertices.data(), GL_STATIC_DRAW);
    }
    void
    unbind() const// NOLINT(readability-convert-member-functions-to-static)
    {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
};

struct vertex_array {
    static vertex_array
    create(vertex_buffer &&buffer)
    {
        unsigned int id;
        glGenVertexArrays(1, &id);
        glBindVertexArray(id);
        buffer.bind();
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);
        buffer.unbind();
        glBindVertexArray(0);
        return {id, buffer};
    }
    const unsigned int id;
    const vertex_buffer buffer;
    void
    draw() const
    {
        glBindVertexArray(id);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(buffer.size / 3));
    }
};

struct shader {
    [[nodiscard]] static shader
    create(const std::string& name, GLenum shaderType, const char *const shaderSource)
    {
        unsigned int shader_id = glCreateShader(shaderType);
        glShaderSource(shader_id, 1, &shaderSource, nullptr);
        glCompileShader(shader_id);
        return {shader_id, name};
    }
    const unsigned int shader_id;
    const std::string name;
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
    ~shader()
    {
        glDeleteShader(shader_id);
    }
};

struct shader_program {
    [[nodiscard]] static shader_program
    create(std::vector<const shader> &&shaders)
    {
        unsigned int shaderProgram;
        shaderProgram = glCreateProgram();
        std::for_each(shaders.begin(), shaders.end(), [&](const auto &s) { glAttachShader(shaderProgram, s.shader_id); });
        glLinkProgram(shaderProgram);
        return {shaderProgram, shaders};
    }
    const unsigned int shader_program_id;
    mutable std::vector<const shader> shaders;
    [[nodiscard]] bool
    check() const
    {
        guard _ = std::function([this]() { shaders.clear(); });
        if (std::any_of(shaders.begin(), shaders.end(), [](const shader &s) { return !s.check(); }))
            return false;
        int success;
        char info_log[512];
        glGetProgramiv(shader_program_id, GL_COMPILE_STATUS, &success);
        if (success == 0) {
            glGetProgramInfoLog(shader_program_id, sizeof info_log, nullptr, info_log);
            std::cerr << "ERROR::SHADER::PROGRAM::COMPILATION_FAILED\n"
                      << info_log << std::endl;
        }
        return static_cast<bool>(success);
    }
    void
    use() const
    {
        glUseProgram(shader_program_id);
    }
};

struct triangle {
    [[nodiscard]] static triangle
    create()
    {
        auto vertex_buffer = vertex_buffer::create({-0.5f, -0.5f, 0.0f,
                                                    0.5f, -0.5f, 0.0f,
                                                    0.0f, 0.5f, 0.0f});
        auto vertex_array = vertex_array::create(std::move(vertex_buffer));
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
        auto shader_program = shader_program::create({vertex_shader, fragment_shader});
        return {shader_program, vertex_array};
    }
    const shader_program shader_program;
    const vertex_array vertex_array;
    [[nodiscard]] bool
    check() const
    {
        return shader_program.check();
    }
    void
    draw() const
    {
        shader_program.use();
        vertex_array.draw();
    }
};

void
process_input(const glfw_window &window)
{
    if (glfwGetKey(window.handle, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window.handle, true);
    }
}

void
clear_color_buffer()
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void
render_loop(const glfw_window &window, const triangle &triangle)
{
    while (!glfwWindowShouldClose(window.handle)) {
        process_input(window);

        clear_color_buffer();
        triangle.draw();

        glfwSwapBuffers(window.handle);
        glfwPollEvents();
    }
}

}// namespace

int
main()
{
    auto glfw = glfw::instantiate();

    auto window = glfw_window::create(glfw);
    if (!window.make_current()) return -1;

    auto glad = glad::initialise(glfw);
    if (!glad.check()) return -1;

    viewport::initialise(window);

    auto triangle = triangle::create();
    if (!triangle.check()) return -1;

    render_loop(window, triangle);

    return 0;
}
