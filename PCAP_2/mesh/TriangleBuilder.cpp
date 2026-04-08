#include "TriangleBuilder.h"
#include "../core/types.h"
#include <glm/gtc/matrix_transform.hpp>

std::vector<Triangle> buildWorldTriangles(
    const std::vector<Vertex>& vertices,
    const std::vector<unsigned int>& indices,
    const glm::mat4& model
) {
    std::vector<Triangle> tris;

    for (size_t i = 0; i < indices.size(); i += 3) {
        Triangle t;

        Vec3 v0 = vertices[indices[i]].Position;
        Vec3 v1 = vertices[indices[i+1]].Position;
        Vec3 v2 = vertices[indices[i+2]].Position;

        t.v0 = Vec3(model * glm::vec4(v0, 1.0));
        t.v1 = Vec3(model * glm::vec4(v1, 1.0));
        t.v2 = Vec3(model * glm::vec4(v2, 1.0));

        t.normal = glm::normalize(glm::cross(t.v1 - t.v0, t.v2 - t.v0));

        tris.push_back(t);
    }

    return tris;
}