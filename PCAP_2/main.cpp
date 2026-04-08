#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>
#include <cstdlib>

// Core
#include "core/types.h"
#include "config/Parameters.h"

// Mesh
#include "mesh/TriangleBuilder.h"
#include "mesh/MeshLoader.h"

// Physics
#include "physics/Density.h"
#include "physics/Forces.h"
#include "physics/SpatialGrid.h"

// Rendering
#include "rendering/Shader.h"
#include "rendering/Camera.h"
#include "rendering/MeshRenderer.h"
#include "rendering/GridRenderer.h"

// ------------------------------------------------------------
// Configuration
// ------------------------------------------------------------
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// ------------------------------------------------------------
// Global State
// ------------------------------------------------------------
std::vector<Vertex> all_vertices;
std::vector<unsigned int> all_indices;
glm::vec3 meshCenter;

std::vector<Particle> particles;
SpatialGrid grid(0.1f);

Camera camera;
float wingHeight = 0.5f;

// ------------------------------------------------------------
// Particle Initialization
// ------------------------------------------------------------
void initParticles() {
    particles.clear();

    for (int i = 0; i < 2000; i++) {
        Particle p;
        p.pos = Vec3(-2.0f, 0.5f, ((rand() % 100) / 100.0f - 0.5f));
        p.vel = Vec3(5.0f, 0.0f, 0.0f);
        p.density = Params::REST_DENSITY;
        p.pressure = 0.0f;
        particles.push_back(p);
    }
}

// ------------------------------------------------------------
// Physics Step
// ------------------------------------------------------------
Vec3 simulatePhysics(const std::vector<Triangle>& tris) {

    Vec3 totalForce(0.0f);

    grid.build(particles);

    for (int i = 0; i < particles.size(); i++) {

        Particle& p = particles[i];

        p.density = computeDensity(p, particles, grid);
        p.pressure = Params::GAS_CONST * (p.density - Params::REST_DENSITY);

        for (const auto& tri : tris) {
            totalForce += computeMomentumForce(p, tri);
            totalForce += computePressureForce(p, tri);
            totalForce += computeShearForce(p, tri);
        }

        p.pos += p.vel * Params::DT;

        if (p.pos.x > 2.0f) {
            p.pos.x = -2.0f;
        }
    }

    return totalForce;
}

// ------------------------------------------------------------
// Shader Sources
// ------------------------------------------------------------
const char* wingVS = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 1) in vec3 aNormal;\n"
"out vec3 Normal;\n"
"uniform mat4 model; uniform mat4 view; uniform mat4 projection;\n"
"void main() {\n"
"   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
"   Normal = mat3(transpose(inverse(model))) * aNormal;\n"
"}";

const char* wingFS = "#version 330 core\n"
"in vec3 Normal;\n"
"out vec4 FragColor;\n"
"void main() {\n"
"   vec3 lightDir = normalize(vec3(1.0,1.0,1.0));\n"
"   float diff = max(dot(normalize(Normal), lightDir), 0.2);\n"
"   FragColor = vec4(vec3(0.8) * diff, 1.0);\n"
"}";

const char* gridVS = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"uniform mat4 view; uniform mat4 projection;\n"
"void main() {\n"
"   gl_Position = projection * view * vec4(aPos,1.0);\n"
"}";

const char* gridFS = "#version 330 core\n"
"out vec4 FragColor;\n"
"void main() {\n"
"   FragColor = vec4(0.4,0.4,0.4,1.0);\n"
"}";

// ------------------------------------------------------------
// Main
// ------------------------------------------------------------
int main() {

    // Load mesh
    loadMesh("assets/2026_f1_front_wing.glb",
             all_vertices,
             all_indices,
             meshCenter);

    // Init GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Fluid Wing Sim", NULL, NULL);
    if (!window) {
        std::cout << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Init GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD\n";
        return -1;
    }

    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glEnable(GL_DEPTH_TEST);

    // Init systems
    initParticles();

    Shader wingShader(wingVS, wingFS);
    Shader gridShader(gridVS, gridFS);

    MeshRenderer meshRenderer;
    meshRenderer.init(all_vertices, all_indices);

    GridRenderer gridRenderer;
    gridRenderer.init(10.0f, 20);

    // Camera defaults (safe)
    camera.yaw = 0.0f;
    camera.pitch = 0.0f;
    camera.zoom = 5.0f;

    // Model transform
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, wingHeight, 0.0f));
    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1,0,0));
    model = glm::scale(model, glm::vec3(0.01f));
    model = glm::translate(model, -meshCenter);

    // Build triangles once
    auto tris = buildWorldTriangles(all_vertices, all_indices, model);

    int frame = 0;

    // Main loop
    while (!glfwWindowShouldClose(window)) {

        // Input (ESC to close)
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = camera.getView();
        glm::mat4 projection = camera.getProj(SCR_WIDTH, SCR_HEIGHT);

        // Physics
        Vec3 totalForce = simulatePhysics(tris);

        if (frame++ % 60 == 0) {
            std::cout << "Force: "
                      << totalForce.x << ", "
                      << totalForce.y << ", "
                      << totalForce.z << std::endl;
        }

        // Render grid
        gridShader.use();
        glUniformMatrix4fv(glGetUniformLocation(gridShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(gridShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        gridRenderer.draw();

        // Render mesh
        wingShader.use();
        glUniformMatrix4fv(glGetUniformLocation(wingShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(wingShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(wingShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        meshRenderer.draw();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}