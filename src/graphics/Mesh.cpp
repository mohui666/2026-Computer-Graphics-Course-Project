#include "graphics/Mesh.h"

#include <glm/glm.hpp>
#include <cmath>
#include <cstddef>
#include <array>
#include <algorithm>
#include <utility>

namespace mine {

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices)
    : indexCount_(static_cast<GLsizei>(indices.size())) {
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)), indices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, px)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, nx)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, u)));
    glBindVertexArray(0);
}

Mesh::~Mesh() {
    if (ebo_) glDeleteBuffers(1, &ebo_);
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (vao_) glDeleteVertexArrays(1, &vao_);
}
Mesh::Mesh(Mesh&& other) noexcept
    : vao_(std::exchange(other.vao_, 0)), vbo_(std::exchange(other.vbo_, 0)),
      ebo_(std::exchange(other.ebo_, 0)), indexCount_(std::exchange(other.indexCount_, 0)) {}
Mesh& Mesh::operator=(Mesh&& other) noexcept {
    if (this != &other) {
        if (ebo_) glDeleteBuffers(1, &ebo_);
        if (vbo_) glDeleteBuffers(1, &vbo_);
        if (vao_) glDeleteVertexArrays(1, &vao_);
        vao_ = std::exchange(other.vao_, 0); vbo_ = std::exchange(other.vbo_, 0);
        ebo_ = std::exchange(other.ebo_, 0); indexCount_ = std::exchange(other.indexCount_, 0);
    }
    return *this;
}

void Mesh::draw() const {
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);
}

Mesh Mesh::cube() {
    const std::vector<Vertex> v = {
        {-0.5F,-0.5F, 0.5F, 0,0,1, 0,0},{ 0.5F,-0.5F, 0.5F, 0,0,1, 1,0},{ 0.5F, 0.5F, 0.5F, 0,0,1, 1,1},{-0.5F, 0.5F, 0.5F, 0,0,1, 0,1},
        { 0.5F,-0.5F,-0.5F, 0,0,-1,0,0},{-0.5F,-0.5F,-0.5F,0,0,-1,1,0},{-0.5F,0.5F,-0.5F,0,0,-1,1,1},{0.5F,0.5F,-0.5F,0,0,-1,0,1},
        {-0.5F,-0.5F,-0.5F,-1,0,0,0,0},{-0.5F,-0.5F,0.5F,-1,0,0,1,0},{-0.5F,0.5F,0.5F,-1,0,0,1,1},{-0.5F,0.5F,-0.5F,-1,0,0,0,1},
        {0.5F,-0.5F,0.5F,1,0,0,0,0},{0.5F,-0.5F,-0.5F,1,0,0,1,0},{0.5F,0.5F,-0.5F,1,0,0,1,1},{0.5F,0.5F,0.5F,1,0,0,0,1},
        {-0.5F,0.5F,0.5F,0,1,0,0,0},{0.5F,0.5F,0.5F,0,1,0,1,0},{0.5F,0.5F,-0.5F,0,1,0,1,1},{-0.5F,0.5F,-0.5F,0,1,0,0,1},
        {-0.5F,-0.5F,-0.5F,0,-1,0,0,0},{0.5F,-0.5F,-0.5F,0,-1,0,1,0},{0.5F,-0.5F,0.5F,0,-1,0,1,1},{-0.5F,-0.5F,0.5F,0,-1,0,0,1}};
    std::vector<unsigned int> i;
    for (unsigned int f = 0; f < 6; ++f) {
        const unsigned int b = f * 4;
        i.insert(i.end(), {b,b+1,b+2,b,b+2,b+3});
    }
    return Mesh(v, i);
}

Mesh Mesh::chamferedBox(float bevel) {
    bevel = std::clamp(bevel, 0.01F, 0.22F);
    const float inner = 0.5F - bevel;
    const std::array<float,5> coordinates{{-0.5F, -inner, 0.0F, inner, 0.5F}};
    struct FaceBasis { glm::vec3 normal, tangentU, tangentV; };
    const std::array<FaceBasis,6> faces{{
        {{ 1, 0, 0}, { 0, 1, 0}, { 0, 0, 1}},
        {{-1, 0, 0}, { 0, 1, 0}, { 0, 0,-1}},
        {{ 0, 1, 0}, { 1, 0, 0}, { 0, 0,-1}},
        {{ 0,-1, 0}, { 1, 0, 0}, { 0, 0, 1}},
        {{ 0, 0, 1}, { 1, 0, 0}, { 0, 1, 0}},
        {{ 0, 0,-1}, {-1, 0, 0}, { 0, 1, 0}}
    }};

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    vertices.reserve(150);
    indices.reserve(192);
    for (const FaceBasis& face : faces) {
        const unsigned int base = static_cast<unsigned int>(vertices.size());
        for (int vIndex = 0; vIndex < 5; ++vIndex) {
            for (int uIndex = 0; uIndex < 5; ++uIndex) {
                const glm::vec3 cubePoint = face.normal * 0.5F +
                                            face.tangentU * coordinates[static_cast<std::size_t>(uIndex)] +
                                            face.tangentV * coordinates[static_cast<std::size_t>(vIndex)];
                const glm::vec3 core = glm::clamp(cubePoint, glm::vec3(-inner), glm::vec3(inner));
                const glm::vec3 offset = cubePoint - core;
                const glm::vec3 normal = glm::normalize(offset);
                const glm::vec3 position = core + normal * bevel;
                vertices.push_back({position.x, position.y, position.z,
                                    normal.x, normal.y, normal.z,
                                    static_cast<float>(uIndex) * 0.25F,
                                    static_cast<float>(vIndex) * 0.25F});
            }
        }
        for (unsigned int row = 0; row < 4; ++row) {
            for (unsigned int column = 0; column < 4; ++column) {
                const unsigned int a = base + row * 5 + column;
                const unsigned int b = a + 1;
                const unsigned int d = a + 5;
                const unsigned int c = d + 1;
                indices.insert(indices.end(), {a,b,c,a,c,d});
            }
        }
    }
    return Mesh(std::move(vertices), std::move(indices));
}

Mesh Mesh::cylinder(int segments) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    constexpr float pi = 3.14159265358979323846F;
    for (int s = 0; s <= segments; ++s) {
        const float angle = 2.0F * pi * static_cast<float>(s) / static_cast<float>(segments);
        const float x = std::cos(angle) * 0.5F;
        const float z = std::sin(angle) * 0.5F;
        vertices.push_back({x,-0.5F,z,x,0,z,static_cast<float>(s)/segments,0});
        vertices.push_back({x, 0.5F,z,x,0,z,static_cast<float>(s)/segments,1});
        if (s < segments) {
            const unsigned int b = static_cast<unsigned int>(s * 2);
            indices.insert(indices.end(), {b,b+1,b+2,b+1,b+3,b+2});
        }
    }
    const unsigned int bottom = static_cast<unsigned int>(vertices.size());
    vertices.push_back({0,-0.5F,0,0,-1,0,0.5F,0.5F});
    const unsigned int top = static_cast<unsigned int>(vertices.size());
    vertices.push_back({0,0.5F,0,0,1,0,0.5F,0.5F});
    const unsigned int bottomRim = static_cast<unsigned int>(vertices.size());
    for (int s = 0; s <= segments; ++s) {
        const float angle = 2.0F * pi * static_cast<float>(s) / static_cast<float>(segments);
        const float x = std::cos(angle) * 0.5F;
        const float z = std::sin(angle) * 0.5F;
        vertices.push_back({x,-0.5F,z,0,-1,0,x+0.5F,z+0.5F});
    }
    const unsigned int topRim = static_cast<unsigned int>(vertices.size());
    for (int s = 0; s <= segments; ++s) {
        const float angle = 2.0F * pi * static_cast<float>(s) / static_cast<float>(segments);
        const float x = std::cos(angle) * 0.5F;
        const float z = std::sin(angle) * 0.5F;
        vertices.push_back({x,0.5F,z,0,1,0,x+0.5F,z+0.5F});
    }
    for (int s = 0; s < segments; ++s) {
        const unsigned int a = static_cast<unsigned int>(s);
        indices.insert(indices.end(), {bottom,bottomRim+a,bottomRim+a+1,
                                       top,topRim+a+1,topRim+a});
    }
    return Mesh(vertices, indices);
}

Mesh Mesh::billboard() {
    return Mesh({{-0.5F,-0.5F,0,0,0,1,0,0}, {0.5F,-0.5F,0,0,0,1,1,0},
                 {0.5F,0.5F,0,0,0,1,1,1}, {-0.5F,0.5F,0,0,0,1,0,1}},
                {0,1,2,0,2,3});
}

Mesh Mesh::chainLink() {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    constexpr int segments = 16, sides = 6;
    for (int i = 0; i <= segments; ++i) {
        const float a = i * 6.2831853F / segments;
        const glm::vec3 center{std::cos(a)*0.37F, 0, std::sin(a)*0.22F};
        const glm::vec3 radial = glm::normalize(glm::vec3(std::cos(a)/0.37F,0,std::sin(a)/0.22F));
        for (int j = 0; j <= sides; ++j) {
            const float b = j * 6.2831853F / sides;
            const glm::vec3 n = radial * std::cos(b) + glm::vec3(0,1,0)*std::sin(b);
            const glm::vec3 v = center + n*0.075F;
            vertices.push_back({v.x,v.y,v.z,n.x,n.y,n.z,static_cast<float>(i)/segments,static_cast<float>(j)/sides});
            if (i < segments && j < sides) {
                const unsigned int k = i*(sides+1)+j;
                indices.insert(indices.end(),{k,k+1,k+sides+1,k+1,k+sides+2,k+sides+1});
            }
        }
    }
    return Mesh(std::move(vertices),std::move(indices));
}

Mesh Mesh::rock() {
    struct Point { float x, y, z; };
    constexpr float phi = 1.61803398875F;
    std::array<Point, 12> points{{
        {-1, phi, 0}, {1, phi, 0}, {-1,-phi, 0}, {1,-phi, 0},
        {0,-1, phi}, {0, 1, phi}, {0,-1,-phi}, {0, 1,-phi},
        {phi, 0,-1}, {phi, 0, 1}, {-phi,0,-1}, {-phi,0,1}
    }};
    for (std::size_t index = 0; index < points.size(); ++index) {
        auto& p = points[index];
        const float length = std::sqrt(p.x*p.x + p.y*p.y + p.z*p.z);
        const float irregular = 0.46F + 0.045F * std::sin(static_cast<float>(index) * 2.37F);
        p.x = p.x / length * irregular;
        p.y = p.y / length * irregular;
        p.z = p.z / length * irregular;
    }
    constexpr std::array<std::array<int,3>,20> faces{{
        {{0,11,5}}, {{0,5,1}}, {{0,1,7}}, {{0,7,10}}, {{0,10,11}},
        {{1,5,9}}, {{5,11,4}}, {{11,10,2}}, {{10,7,6}}, {{7,1,8}},
        {{3,9,4}}, {{3,4,2}}, {{3,2,6}}, {{3,6,8}}, {{3,8,9}},
        {{4,9,5}}, {{2,4,11}}, {{6,2,10}}, {{8,6,7}}, {{9,8,1}}
    }};
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    vertices.reserve(faces.size() * 3);
    indices.reserve(faces.size() * 3);
    for (const auto& face : faces) {
        const Point& a = points[static_cast<std::size_t>(face[0])];
        const Point& b = points[static_cast<std::size_t>(face[1])];
        const Point& c = points[static_cast<std::size_t>(face[2])];
        const Point ab{b.x-a.x, b.y-a.y, b.z-a.z};
        const Point ac{c.x-a.x, c.y-a.y, c.z-a.z};
        Point n{ab.y*ac.z - ab.z*ac.y,
                ab.z*ac.x - ab.x*ac.z,
                ab.x*ac.y - ab.y*ac.x};
        const float normalLength = std::sqrt(n.x*n.x + n.y*n.y + n.z*n.z);
        n.x /= normalLength; n.y /= normalLength; n.z /= normalLength;
        for (int pointIndex : face) {
            const Point& p = points[static_cast<std::size_t>(pointIndex)];
            indices.push_back(static_cast<unsigned int>(vertices.size()));
            vertices.push_back({p.x,p.y,p.z,n.x,n.y,n.z,p.x+0.5F,p.z+0.5F});
        }
    }
    return Mesh(std::move(vertices), std::move(indices));
}

Mesh Mesh::roughPlane(int columns, int rows, int seed) {
    columns = std::max(columns, 2);
    rows = std::max(rows, 2);
    const int vertexColumns = columns + 1;
    const int vertexRows = rows + 1;
    std::vector<float> heights(static_cast<std::size_t>(vertexColumns * vertexRows));
    auto heightAt = [&](int column, int row) -> float& {
        return heights[static_cast<std::size_t>(row * vertexColumns + column)];
    };
    auto hash = [seed](int x, int z) {
        const float value = std::sin(static_cast<float>(x * 127 + z * 311 + seed * 73) * 0.0174533F) * 43758.5453F;
        return (value - std::floor(value)) * 2.0F - 1.0F;
    };
    for (int row = 0; row < vertexRows; ++row) {
        for (int column = 0; column < vertexColumns; ++column) {
            const float x = static_cast<float>(column) / static_cast<float>(columns);
            const float z = static_cast<float>(row) / static_cast<float>(rows);
            const float broad = std::sin((x * 4.8F + seed * 0.13F) * 6.2831853F) * 0.33F +
                                std::sin((z * 3.1F - seed * 0.07F) * 6.2831853F) * 0.24F;
            const float cross = std::sin((x * 11.0F + z * 7.0F + seed * 0.19F) * 3.1415926F) * 0.19F;
            heightAt(column, row) = std::clamp(broad + cross + hash(column, row) * 0.18F, -0.92F, 0.92F);
        }
    }

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    vertices.reserve(heights.size());
    const float dx = 1.0F / static_cast<float>(columns);
    const float dz = 1.0F / static_cast<float>(rows);
    for (int row = 0; row < vertexRows; ++row) {
        for (int column = 0; column < vertexColumns; ++column) {
            const float left = heightAt(std::max(0, column - 1), row);
            const float right = heightAt(std::min(columns, column + 1), row);
            const float down = heightAt(column, std::max(0, row - 1));
            const float up = heightAt(column, std::min(rows, row + 1));
            const glm::vec3 tangentX{column == 0 || column == columns ? dx : 2.0F * dx,
                                     right - left, 0.0F};
            const glm::vec3 tangentZ{0.0F, up - down,
                                     row == 0 || row == rows ? dz : 2.0F * dz};
            const glm::vec3 normal = glm::normalize(glm::cross(tangentZ, tangentX));
            const float u = static_cast<float>(column) / static_cast<float>(columns);
            const float v = static_cast<float>(row) / static_cast<float>(rows);
            vertices.push_back({u - 0.5F, heightAt(column,row), v - 0.5F,
                                normal.x, normal.y, normal.z, u, v});
        }
    }
    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < columns; ++column) {
            const unsigned int a = static_cast<unsigned int>(row * vertexColumns + column);
            const unsigned int b = a + 1;
            const unsigned int d = a + static_cast<unsigned int>(vertexColumns);
            const unsigned int c = d + 1;
            indices.insert(indices.end(), {a,d,b,b,d,c});
        }
    }
    return Mesh(std::move(vertices), std::move(indices));
}

namespace {
void appendTriangle(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices,
                    const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) {
    const glm::vec3 normal = glm::normalize(glm::cross(b - a, c - a));
    for (const glm::vec3& point : {a,b,c}) {
        indices.push_back(static_cast<unsigned int>(vertices.size()));
        vertices.push_back({point.x,point.y,point.z,normal.x,normal.y,normal.z,
                            point.x + 0.5F, point.y + 0.5F});
    }
}

void appendQuad(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices,
                const glm::vec3& a, const glm::vec3& b,
                const glm::vec3& c, const glm::vec3& d) {
    appendTriangle(vertices, indices, a, b, c);
    appendTriangle(vertices, indices, a, c, d);
}
}

Mesh Mesh::supportShield() {
    const std::array<glm::vec2,8> outline{{
        {-0.42F,-0.50F}, {0.42F,-0.50F}, {0.50F,-0.35F}, {0.48F,0.37F},
        {0.35F,0.50F}, {-0.35F,0.50F}, {-0.48F,0.37F}, {-0.50F,-0.35F}
    }};
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    for (std::size_t index = 1; index + 1 < outline.size(); ++index) {
        const glm::vec3 a{outline[0].x, outline[0].y, 0.5F};
        const glm::vec3 b{outline[index].x, outline[index].y, 0.5F};
        const glm::vec3 c{outline[index + 1].x, outline[index + 1].y, 0.5F};
        appendTriangle(vertices, indices, a, b, c);
        appendTriangle(vertices, indices, {a.x,a.y,-0.5F}, {c.x,c.y,-0.5F}, {b.x,b.y,-0.5F});
    }
    for (std::size_t index = 0; index < outline.size(); ++index) {
        const glm::vec2 a2 = outline[index];
        const glm::vec2 b2 = outline[(index + 1) % outline.size()];
        appendQuad(vertices, indices,
                   {a2.x,a2.y,0.5F}, {a2.x,a2.y,-0.5F},
                   {b2.x,b2.y,-0.5F}, {b2.x,b2.y,0.5F});
    }
    return Mesh(std::move(vertices), std::move(indices));
}

Mesh Mesh::helicalDrumVanes(int segments, float turns, int blades) {
    segments = std::max(segments, 8);
    blades = std::max(blades, 1);
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    constexpr float pi = 3.14159265358979323846F;
    constexpr float innerRadius = 0.255F;
    constexpr float outerRadius = 0.485F;
    constexpr float halfThickness = 0.022F;
    auto point = [](float radius, float angle, float axial) {
        return glm::vec3{radius * std::cos(angle), axial, radius * std::sin(angle)};
    };
    for (int blade = 0; blade < blades; ++blade) {
        const float phase = static_cast<float>(blade) * 2.0F * pi / static_cast<float>(blades);
        for (int segment = 0; segment < segments; ++segment) {
            const float t0 = static_cast<float>(segment) / static_cast<float>(segments);
            const float t1 = static_cast<float>(segment + 1) / static_cast<float>(segments);
            const float a0 = phase + turns * 2.0F * pi * t0;
            const float a1 = phase + turns * 2.0F * pi * t1;
            const float y0 = t0 - 0.5F;
            const float y1 = t1 - 0.5F;
            const glm::vec3 i0a = point(innerRadius,a0,y0 - halfThickness);
            const glm::vec3 o0a = point(outerRadius,a0,y0 - halfThickness);
            const glm::vec3 i1a = point(innerRadius,a1,y1 - halfThickness);
            const glm::vec3 o1a = point(outerRadius,a1,y1 - halfThickness);
            const glm::vec3 i0b = point(innerRadius,a0,y0 + halfThickness);
            const glm::vec3 o0b = point(outerRadius,a0,y0 + halfThickness);
            const glm::vec3 i1b = point(innerRadius,a1,y1 + halfThickness);
            const glm::vec3 o1b = point(outerRadius,a1,y1 + halfThickness);
            appendQuad(vertices, indices, i0a, o0a, o1a, i1a);
            appendQuad(vertices, indices, i0b, i1b, o1b, o0b);
            appendQuad(vertices, indices, o0a, o0b, o1b, o1a);
            appendQuad(vertices, indices, i0a, i1a, i1b, i0b);
            if (segment == 0) appendQuad(vertices, indices, i0a, i0b, o0b, o0a);
            if (segment == segments - 1) appendQuad(vertices, indices, i1a, o1a, o1b, i1b);
        }
    }
    return Mesh(std::move(vertices), std::move(indices));
}

} // namespace mine
