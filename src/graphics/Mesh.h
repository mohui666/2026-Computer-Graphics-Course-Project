#pragma once

#include <glad/gl.h>
#include <vector>

namespace mine {

struct Vertex {
    float px, py, pz;
    float nx, ny, nz;
    float u, v;
};

class Mesh {
public:
    Mesh() = default;
    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices);
    ~Mesh();
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    void draw() const;
    [[nodiscard]] static Mesh cube();
    [[nodiscard]] static Mesh chamferedBox(float bevel = 0.075F);
    [[nodiscard]] static Mesh cylinder(int segments = 24);
    [[nodiscard]] static Mesh rock();
    [[nodiscard]] static Mesh chainLink();
    [[nodiscard]] static Mesh billboard();
    [[nodiscard]] static Mesh roughPlane(int columns, int rows, int seed);
    [[nodiscard]] static Mesh supportShield();
    [[nodiscard]] static Mesh helicalDrumVanes(int segments = 34, float turns = 1.4F,
                                               int blades = 2);

private:
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLuint ebo_ = 0;
    GLsizei indexCount_ = 0;
};

} // namespace mine
