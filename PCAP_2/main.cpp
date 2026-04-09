#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>
#include <cstdlib>
#include <omp.h>

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
#include "rendering/ParticleRenderer.h"

// ------------------------------------------------------------
// Configuration
// ------------------------------------------------------------
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;
int NUM_PARTICLES = 10;
float flowSpeed = 50.0f;
Vec3 flowDir = glm::normalize(Vec3(1.0f, 0.0f, 0.0f)); // diagonal toward wing(X,Y,Z)

// Force force (X, Y, Z) -> (Drag, Lift/Downforce(Should be negative value), Side force)
// Red is high pressure and blue is low pressure
// ------------------------------------------------------------
// Global State
// ------------------------------------------------------------
std::vector<Vertex> all_vertices;
std::vector<unsigned int> all_indices;
glm::vec3 meshCenter;

std::vector<Particle> particles;
SpatialGrid grid(0.1f);

Camera camera;

// ------------------------------------------------------------
// Particle Initialization
// ------------------------------------------------------------
void initParticles() {
    particles.clear();

    int nx = 30;   // width (Z direction)
    int ny = 30;   // height (Y direction)

    float spacing = 0.03f;

    float startX = -2.0f;   // upstream of wing
    float startY = 0.5f;    // center vertically
    float startZ = 0.0f;

    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {

            Particle p;

            p.pos = Vec3(
                startX,
                startY + (j - ny/2) * spacing,
                startZ + (i - nx/2) * spacing
            );

            p.initialPos = p.pos;
            p.vel = flowDir * flowSpeed;  // HIGH SPEED FLOW

            p.density = Params::REST_DENSITY;
            p.pressure = 0.0f;

            particles.push_back(p);
        }
    }
}

// ------------------------------------------------------------
// Physics Step (OPENMP SAFE)
// ------------------------------------------------------------
Vec3 simulatePhysics(std::vector<Triangle>& tris, SpatialGrid& triGrid) {

    Vec3 totalForce(0.0f);

    grid.buildParticles(particles);

    #pragma omp parallel
    {
        Vec3 localForce(0.0f);

        #pragma omp for
        for (int i = 0; i < particles.size(); i++) {

            Particle& p = particles[i];

            p.density = computeDensity(p, particles, grid);
            p.pressure = Params::GAS_CONST * (p.density - Params::REST_DENSITY);

            auto nearby = triGrid.getNearbyTriangles(p.pos);

            for (int idx : nearby) {

                Triangle& tri = tris[idx];   // FIXED (no const_cast)

                Vec3 f(0.0f);

                f += computeMomentumForce(p, tri);
                f += computePressureForce(p, tri);
                f += computeShearForce(p, tri);

                tri.force += f;
                localForce += f;
            }

            // Integrate
            p.pos += p.vel * Params::DT;

            // Reset
            if (glm::dot(p.pos, flowDir) > 2.0f) {
                p.pos = p.initialPos;
                p.pos.x = -2.0f;

                p.vel = flowDir * 15.0f;   // FIXED
            }
        }

        #pragma omp critical
        totalForce += localForce;
    }

    return totalForce;
}

// ------------------------------------------------------------
//  Draw Force Arrows
// ------------------------------------------------------------
void drawForces(const std::vector<Triangle>& tris) {

    glBegin(GL_LINES);

    for (const auto& tri : tris) {

        Vec3 start = tri.center * 1.0f;
        Vec3 end   = tri.center + tri.force * 1.0f;

        glVertex3f(start.x, start.y, start.z);
        glVertex3f(end.x, end.y, end.z);
    }

    glEnd();
}

// ------------------------------------------------------------
// SHADERS
// ------------------------------------------------------------
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

const char* particleVS = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"uniform mat4 view; uniform mat4 projection;\n"
"void main() {\n"
"   gl_Position = projection * view * vec4(aPos,1.0);\n"
"}";

const char* particleFS = "#version 330 core\n"
"out vec4 FragColor;\n"
"void main() {\n"
"   FragColor = vec4(1.0,0.2,0.2,1.0);\n"
"}";

// ------------------------------------------------------------
// MAIN
// ------------------------------------------------------------
int main() {

    // Load mesh
    loadMesh("assets/2026_f1_front_wing.glb",
             all_vertices,
             all_indices,
             meshCenter);

    std::cout << "Vertices: " << all_vertices.size() << std::endl;
    std::cout << "Indices: " << all_indices.size() << std::endl;

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
    Shader particleShader(particleVS, particleFS);

    MeshRenderer meshRenderer;
    meshRenderer.init(all_vertices, all_indices);

    GridRenderer gridRenderer;
    gridRenderer.init(10.0f, 20);

    ParticleRenderer particleRenderer;
    particleRenderer.init();

    // Camera
    glm::mat4 view = glm::lookAt(
        glm::vec3(0, 1, 5),
        glm::vec3(0, 0, 0),
        glm::vec3(0, 1, 0)
    );

    glm::mat4 projection = camera.getProj(SCR_WIDTH, SCR_HEIGHT);

    // MODEL TRANSFORM (your working version)
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(0.02f));
    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1, 0, 0));
    model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0, 1, 0));
    model = glm::translate(model, -meshCenter);
    model = glm::translate(model, glm::vec3(0.0f, -21.5f, 0.0f));

    // Build triangles once
    auto tris = buildWorldTriangles(all_vertices, all_indices, model);

    SpatialGrid triGrid(0.2f);

    for (int i = 0; i < tris.size(); i++) {
        triGrid.insertTriangle(i, tris[i].center);
    }

    int frame = 0;

    while (!glfwWindowShouldClose(window)) {

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for (auto& tri : tris) {
            tri.force = Vec3(0.0f);
        }

        Vec3 totalForce = simulatePhysics(tris, triGrid);

        if (frame++ % 60 == 0) {
            std::cout << "Force: "
                      << totalForce.x << ", "
                      << totalForce.y << ", "
                      << totalForce.z << std::endl;
        }

        // Grid
        gridShader.use();
        glUniformMatrix4fv(glGetUniformLocation(gridShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(gridShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        gridRenderer.draw();

        // Mesh
        wingShader.use();
        glUniformMatrix4fv(glGetUniformLocation(wingShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(wingShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(wingShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        meshRenderer.draw();

        glDisable(GL_DEPTH_TEST);   // TEMPORARY
        glLineWidth(2.0f);
        drawForces(tris);
        glEnable(GL_DEPTH_TEST);

        // Particles
        particleShader.use();
        glUniformMatrix4fv(glGetUniformLocation(particleShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(particleShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        particleRenderer.draw(particles);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}