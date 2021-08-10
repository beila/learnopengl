#include <glad/glad.h>

#include <GLFW/glfw3.h>
#include <iostream>
#include <optional>
#include <utility>
#include <vector>

namespace
{

template<typename T>
concept Callable = requires(T t)
{
    t();
};

template<Callable C, Callable O = void()>
struct guard {
    guard(C &&closer) : closer(closer) {}// NOLINT(google-explicit-constructor)
    guard(O &&opener, C &&closer) : closer(std::move(closer)) { opener(); }
    C closer;
    ~guard() { closer(); }
};

struct non_copyable {
    non_copyable() = default;
    non_copyable(non_copyable const &) = delete;
    non_copyable &
    operator=(non_copyable const &) = delete;
};

struct glfw: private non_copyable {
    static std::optional<glfw>
    instantiate()
    {
        if (glfwInit() == GLFW_TRUE) {
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);// needed for mac
            return std::make_optional<glfw>();
        }
        std::cerr << "Failed to instantiate glfw" << std::endl;
        return std::nullopt;
    }
    ~glfw()
    {
        glfwTerminate();
    }
};

struct glfw_window: private non_copyable {
    static std::optional<glfw_window>
    create(const glfw &)
    {
        GLFWwindow *handle = glfwCreateWindow(800, 600, "LearnOpenGL", nullptr, nullptr);
        if (handle) {
            return std::make_optional<glfw_window>(handle);
        }
        std::cerr << "Failed to create GLFW window" << std::endl;
        return std::nullopt;
    }
    GLFWwindow *const handle;
    explicit glfw_window(GLFWwindow *handle) : handle(handle) {}
    void
    make_current() const
    {
        glfwMakeContextCurrent(handle);
    }
    ~glfw_window()
    {
        glfwDestroyWindow(handle);
    }
};

struct glad {
    static std::optional<glad>
    initialise(const glfw_window &w)
    {
        w.make_current();
        if (gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) return std::make_optional<glad>();
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return std::nullopt;
    }
};

struct viewport: private non_copyable {
    static viewport
    initialise(const glfw_window &w)
    {
        glfwSetFramebufferSizeCallback(w.handle, viewport::framebuffer_size_callback);
        return viewport{w};
    }
    const glfw_window &window;
    explicit viewport(const glfw_window &w) : window(w){};
    static void
    framebuffer_size_callback(GLFWwindow *, int width, int height)
    {
        glViewport(0, 0, width, height);
    }
    ~viewport()
    {
        glfwSetFramebufferSizeCallback(window.handle, nullptr);
    }
};

template<GLenum array_type = GL_ARRAY_BUFFER, typename T = typename std::conditional<array_type == GL_ELEMENT_ARRAY_BUFFER, int, float>::type>
requires(array_type == GL_ARRAY_BUFFER || array_type == GL_ELEMENT_ARRAY_BUFFER) struct buffer {
    static buffer
    create(std::vector<const T> &&data)
    {
        unsigned int BO;
        glGenBuffers(1, &BO);
        return {BO, static_cast<GLsizei>(data.size()), std::move(data)};
    }
    const unsigned int id;
    const GLsizei count;
    const std::vector<const T> data;
    [[nodiscard]] auto
    bind() const
    {
        glBindBuffer(array_type, id);
        glBufferData(array_type, count * sizeof(T), data.data(), GL_STATIC_DRAW);
        return guard{[]() { glBindBuffer(array_type, 0); }};
    }
};
using vertex_buffer = buffer<GL_ARRAY_BUFFER, float>;
using element_buffer = buffer<GL_ELEMENT_ARRAY_BUFFER, int>;

struct vertex_array {
    static vertex_array
    create(std::vector<const float> &&vertices)
    {
        auto b = vertex_buffer::create(std::move(vertices));
        unsigned int id;
        glGenVertexArrays(1, &id);
        auto _a = guard{[id]() { glBindVertexArray(id); }, []() { glBindVertexArray(0); }};
        auto _b = b.bind();
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);
        return {id, b.count};
    }
    const unsigned int id;
    const GLsizei count;
    void
    draw() const
    {
        glBindVertexArray(id);
        glDrawArrays(GL_TRIANGLES, 0, count);
        glBindVertexArray(0);
    }
};

struct element_array {
    static element_array
    create(std::vector<const float> &&vertices, std::vector<const int> &&indices)
    {
        auto vertex_buffer = vertex_buffer::create(std::move(vertices));
        auto index_buffer = element_buffer::create(std::move(indices));
        unsigned int id;
        glGenVertexArrays(1, &id);
        auto _a = guard{[id]() { glBindVertexArray(id); }, []() { glBindVertexArray(0); }};
        auto _v = vertex_buffer.bind();
        auto _i = index_buffer.bind();
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);
        return {id, index_buffer.count};
    }
    const unsigned int id;
    const GLsizei count;
    void
    draw() const
    {
        glBindVertexArray(id);
        glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }
};

struct shader: private non_copyable {
    template<GLenum shader_type>
    requires(shader_type == GL_VERTEX_SHADER || shader_type == GL_FRAGMENT_SHADER)
        [[nodiscard]] static std::unique_ptr<shader> create(const std::string &name, const char *const shaderSource)
    {
        unsigned int shader_id = glCreateShader(shader_type);
        glShaderSource(shader_id, 1, &shaderSource, nullptr);
        glCompileShader(shader_id);
        int success;
        char info_log[512];
        glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);
        if (success) return std::make_unique<shader>(shader_id);
        glGetShaderInfoLog(shader_id, sizeof info_log, nullptr, info_log);
        std::cerr << "ERROR::SHADER::" << name << "::COMPILATION_FAILED\n"
                  << info_log << std::endl;
        return {};
    }
    unsigned int shader_id;
    explicit shader(unsigned int shader_id) : shader_id(shader_id) {}
    ~shader()
    {
        glDeleteShader(shader_id);
    }
};

struct shader_pipeline {
    static std::optional<shader_pipeline>
    create(std::vector<std::unique_ptr<const shader>> &&shaders)
    {
        if (std::any_of(shaders.begin(), shaders.end(), [](const auto &s) { return !s; }))
            return std::nullopt;
        unsigned int shader_program_id;
        shader_program_id = glCreateProgram();
        std::for_each(shaders.begin(), shaders.end(), [&](const auto &s) { glAttachShader(shader_program_id, s->shader_id); });
        glLinkProgram(shader_program_id);
        int success;
        char info_log[512];
        glGetProgramiv(shader_program_id, GL_COMPILE_STATUS, &success);
        if (success) return std::make_optional<shader_pipeline>(shader_program_id);
        glGetProgramInfoLog(shader_program_id, sizeof info_log, nullptr, info_log);
        std::cerr << "ERROR::SHADER::PROGRAM::COMPILATION_FAILED\n"
                  << info_log << std::endl;
        return std::nullopt;
    }
    const unsigned int shader_program_id;
    explicit shader_pipeline(const unsigned int shaderProgramId) : shader_program_id(shaderProgramId) {}
    void
    use() const
    {
        glUseProgram(shader_program_id);
    }
};

struct triangle_shader {
    static std::optional<shader_pipeline>
    create()
    {
        std::vector<std::unique_ptr<const shader>> v{};
        v.push_back(shader::create<GL_VERTEX_SHADER>("triangle_vertex", R"(
#version 330 core
layout (location = 0) in vec3 aPos;
void main()
{
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}
        )"));
        v.push_back(shader::create<GL_FRAGMENT_SHADER>("triangle_fragment", R"(
#version 330 core
out vec4 FragColor;
void main()
{
    FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
}
        )"));
        return shader_pipeline::create(std::move(v));
    }
};

template<typename T>
concept Drawable = requires(T t)
{
    t.draw();
};

template<Drawable T>
struct shape {
    const T drawable;
    const shader_pipeline shader_pipeline;
    shape(const T drawable, const struct shader_pipeline shaderPipeline) : drawable(drawable), shader_pipeline(shaderPipeline) {}
    void
    draw() const
    {
        shader_pipeline.use();
        drawable.draw();
    }
};

struct triangle {
    static std::optional<shape<vertex_array>>
    create()
    {
        auto array = vertex_array::create({-0.5f, -0.5f, 0.0f,
                                           0.5f, -0.5f, 0.0f,
                                           0.0f, 0.5f, 0.0f});
        const auto &shader = triangle_shader::create();
        if (shader) return std::make_optional<shape<vertex_array>>(array, *shader);
        return std::nullopt;
    }
};

struct rectangle {
    static std::optional<shape<element_array>>
    create()
    {
        auto array = element_array::create({0.5f, 0.5f, 0.0f,
                                            0.5f, -0.5f, 0.0f,
                                            -0.5f, -0.5f, 0.0f,
                                            -0.5f, 0.5f, 0.0f},
                                           {0, 1, 3,
                                            1, 2, 3});
        const auto &shader = triangle_shader::create();
        if (shader) return std::make_optional<shape<element_array>>(array, *shader);
        return std::nullopt;
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

template<typename T>
void
render_loop(const glfw_window &window, const shape<T> &shape)
{
    while (!glfwWindowShouldClose(window.handle)) {
        process_input(window);

        clear_color_buffer();
        shape.draw();

        glfwSwapBuffers(window.handle);
        glfwPollEvents();
    }
}

}// namespace

int
main()
{
    auto glfw = glfw::instantiate();
    if (!glfw) return -1;

    auto window = glfw_window::create(*glfw);
    if (!window) return -2;

    auto glad = glad::initialise(*window);
    if (!glad) return -3;

    auto viewport = viewport::initialise(*window);

    auto shape = triangle::create();
    if (!shape) return -4;

    render_loop(*window, *shape);

    return 0;
}
