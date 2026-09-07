#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

namespace mine {

class Shader {
public:
    Shader() = default;
    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    ~Shader();
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    void load(const std::string& vertexPath, const std::string& fragmentPath);
    void use() const;
    void set(const char* name, bool value) const;
    void set(const char* name, int value) const;
    void set(const char* name, float value) const;
    void set(const char* name, const glm::vec2& value) const;
    void set(const char* name, const glm::vec3& value) const;
    void set(const char* name, const glm::mat3& value) const;
    void set(const char* name, const glm::mat4& value) const;
    [[nodiscard]] GLuint id() const { return id_; }

private:
    GLint location(const char* name) const;
    mutable std::unordered_map<std::string, GLint> locations_;
    GLuint id_ = 0;
};

} // namespace mine
