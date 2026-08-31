#include "graphics/Shader.h"

#include <glm/gtc/type_ptr.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace mine {
namespace {
std::string readText(const std::string& path) {
    std::ifstream file(std::filesystem::u8path(path), std::ios::binary);
    if (!file) throw std::runtime_error("Unable to open shader: " + path);
    std::ostringstream stream;
    stream << file.rdbuf();
    return stream.str();
}

GLuint compile(GLenum type, const std::string& source, const std::string& path) {
    const GLuint shader = glCreateShader(type);
    const char* code = source.c_str();
    glShaderSource(shader, 1, &code, nullptr);
    glCompileShader(shader);
    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        std::string log(static_cast<std::size_t>(std::max(length, 1)), '\0');
        glGetShaderInfoLog(shader, length, nullptr, log.data());
        glDeleteShader(shader);
        throw std::runtime_error("Shader compilation failed (" + path + "): " + log);
    }
    return shader;
}
}

Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath) { load(vertexPath, fragmentPath); }
Shader::~Shader() { if (id_) glDeleteProgram(id_); }
Shader::Shader(Shader&& other) noexcept : id_(std::exchange(other.id_, 0)) {}
Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) { if (id_) glDeleteProgram(id_); id_ = std::exchange(other.id_, 0); }
    return *this;
}

void Shader::load(const std::string& vertexPath, const std::string& fragmentPath) {
    const GLuint vertex = compile(GL_VERTEX_SHADER, readText(vertexPath), vertexPath);
    const GLuint fragment = compile(GL_FRAGMENT_SHADER, readText(fragmentPath), fragmentPath);
    const GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLint length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        std::string log(static_cast<std::size_t>(std::max(length, 1)), '\0');
        glGetProgramInfoLog(program, length, nullptr, log.data());
        glDeleteProgram(program);
        throw std::runtime_error("Shader link failed: " + log);
    }
    if (id_) glDeleteProgram(id_);
    id_ = program;
}

void Shader::use() const { glUseProgram(id_); }
void Shader::set(const char* name, bool value) const { glUniform1i(glGetUniformLocation(id_, name), value); }
void Shader::set(const char* name, int value) const { glUniform1i(glGetUniformLocation(id_, name), value); }
void Shader::set(const char* name, float value) const { glUniform1f(glGetUniformLocation(id_, name), value); }
void Shader::set(const char* name, const glm::vec2& value) const { glUniform2fv(glGetUniformLocation(id_, name), 1, glm::value_ptr(value)); }
void Shader::set(const char* name, const glm::vec3& value) const { glUniform3fv(glGetUniformLocation(id_, name), 1, glm::value_ptr(value)); }
void Shader::set(const char* name, const glm::mat3& value) const { glUniformMatrix3fv(glGetUniformLocation(id_, name), 1, GL_FALSE, glm::value_ptr(value)); }
void Shader::set(const char* name, const glm::mat4& value) const { glUniformMatrix4fv(glGetUniformLocation(id_, name), 1, GL_FALSE, glm::value_ptr(value)); }

} // namespace mine
