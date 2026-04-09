#include "TriangleBuilder.h"
#include "../core/types.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>

std::vector<Triangle> buildWorldTriangles(
    const std::vector<Vertex>& vertices,
    const std::vector<unsigned int>& indices,
    const glm::mat4& model
) {
    std::vector<Triangle> tris;

    for (size_t i = 0; i < indices.size(); i += 30) {

        Triangle tri; 

        // Get vertex positions
        Vec3 v0 = vertices[indices[i]].Position;
        Vec3 v1 = vertices[indices[i+1]].Position;
        Vec3 v2 = vertices[indices[i+2]].Position;

        // Transform to world space
        tri.v0 = Vec3(model * glm::vec4(v0, 1.0f));
        tri.v1 = Vec3(model * glm::vec4(v1, 1.0f));
        tri.v2 = Vec3(model * glm::vec4(v2, 1.0f));

        // Compute normal
        tri.normal = glm::normalize(glm::cross(tri.v1 - tri.v0, tri.v2 - tri.v0));

        // Compute center (for culling optimization)
        tri.center = (tri.v0 + tri.v1 + tri.v2) / 3.0f;

        // Store triangle
        tris.push_back(tri);
    }

    return tris;
}