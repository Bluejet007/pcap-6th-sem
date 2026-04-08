#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>
#include <vector>

// --- Configuration & Globals ---
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
};

// Interaction state
bool firstMouse = true;
bool leftMouseButtonPressed = false;
double lastX, lastY;
float yaw = 0.0f;   
float pitch = 0.0f; 
float zoom = 5.0f;  
float wingHeight = 0.5f; // Vertical offset from ground grid

// Mesh data storage
std::vector<Vertex> all_vertices;
std::vector<unsigned int> all_indices;
unsigned int VAO, VBO, EBO;
glm::vec3 meshCenter = glm::vec3(0.0f);

// --- Input Callbacks ---

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            leftMouseButtonPressed = true;
            firstMouse = true; 
        } else if (action == GLFW_RELEASE) {
            leftMouseButtonPressed = false;
        }
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (!leftMouseButtonPressed) return;

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    double xoffset = xpos - lastX;
    double yoffset = lastY - ypos; 
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.01f; 
    yaw   += xoffset * sensitivity;
    pitch += yoffset * sensitivity;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    zoom -= (float)yoffset;
    if (zoom < 1.0f) zoom = 1.0f;   
    if (zoom > 50.0f) zoom = 50.0f; 
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// --- Shader Sources ---

const char* vShader = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec3 aNormal;\n"
    "out vec3 Normal;\n"
    "uniform mat4 model; uniform mat4 view; uniform mat4 projection;\n"
    "void main() {\n"
    "   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
    "   Normal = mat3(transpose(inverse(model))) * aNormal;\n"
    "}\0";

const char* fShader = "#version 330 core\n"
    "in vec3 Normal;\n"
    "out vec4 FragColor;\n"
    "void main() {\n"
    "   vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));\n"
    "   float diff = max(dot(normalize(Normal), lightDir), 0.2);\n"
    "   FragColor = vec4(vec3(0.8) * diff, 1.0);\n"
    "}\n\0";

const char* gridVShader = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "uniform mat4 view; uniform mat4 projection;\n"
    "void main() {\n"
    "   gl_Position = projection * view * vec4(aPos, 1.0);\n"
    "}\0";

const char* gridFShader = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main() {\n"
    "   FragColor = vec4(0.4, 0.4, 0.4, 1.0);\n"
    "}\n\0";

// --- Procedural Geometry ---

std::vector<float> generateGrid(float size, int divisions) {
    std::vector<float> vertices;
    float step = size / divisions;
    for (int i = 0; i <= divisions; ++i) {
        float pos = -size / 2.0f + i * step;
        vertices.push_back(pos); vertices.push_back(0.0f); vertices.push_back(-size / 2.0f);
        vertices.push_back(pos); vertices.push_back(0.0f); vertices.push_back(size / 2.0f);
        vertices.push_back(-size / 2.0f); vertices.push_back(0.0f); vertices.push_back(pos);
        vertices.push_back(size / 2.0f); vertices.push_back(0.0f); vertices.push_back(pos);
    }
    return vertices;
}

// --- Model Processing ---

void processMesh(aiMesh *mesh, const aiScene *scene, glm::vec3& minB, glm::vec3& maxB) {
    unsigned int indexOffset = all_vertices.size();
    for(unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        vertex.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        
        // Update bounding box for centering
        minB = glm::min(minB, vertex.Position);
        maxB = glm::max(maxB, vertex.Position);

        if(mesh->HasNormals())
            vertex.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        all_vertices.push_back(vertex);
    }
    for(unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for(unsigned int j = 0; j < face.mNumIndices; j++)
            all_indices.push_back(indexOffset + face.mIndices[j]);
    }
}

void processNode(aiNode *node, const aiScene *scene, glm::vec3& minB, glm::vec3& maxB) {
    for(unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        processMesh(mesh, scene, minB, maxB);
    }
    for(unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene, minB, maxB);
    }
}

int main() {
    // 1. Context Initialization
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "2026 F1 Wing Viewer", NULL, NULL);
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glEnable(GL_DEPTH_TEST);

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // 2. Asset Loading & Re-centering Logic
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile("2026_f1_front_wing.glb", aiProcess_Triangulate | aiProcess_GenSmoothNormals);
    
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return -1;
    }

    glm::vec3 minBound(1e10f), maxBound(-1e10f);
    processNode(scene->mRootNode, scene, minBound, maxBound);
    meshCenter = (minBound + maxBound) / 2.0f;

    // 3. Wing Buffer Setup
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, all_vertices.size() * sizeof(Vertex), &all_vertices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, all_indices.size() * sizeof(unsigned int), &all_indices[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    glEnableVertexAttribArray(1);

    // 4. Grid Buffer Setup
    std::vector<float> gridVerts = generateGrid(10.0f, 20);
    unsigned int gridVAO, gridVBO;
    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);
    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, gridVerts.size() * sizeof(float), &gridVerts[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // 5. Shader Compilation
    auto compileShader = [](const char* source, GLenum type) {
        unsigned int s = glCreateShader(type);
        glShaderSource(s, 1, &source, NULL);
        glCompileShader(s);
        return s;
    };

    unsigned int wingProg = glCreateProgram();
    glAttachShader(wingProg, compileShader(vShader, GL_VERTEX_SHADER));
    glAttachShader(wingProg, compileShader(fShader, GL_FRAGMENT_SHADER));
    glLinkProgram(wingProg);

    unsigned int gridProg = glCreateProgram();
    glAttachShader(gridProg, compileShader(gridVShader, GL_VERTEX_SHADER));
    glAttachShader(gridProg, compileShader(gridFShader, GL_FRAGMENT_SHADER));
    glLinkProgram(gridProg);

    // 6. Main Render Loop
    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        processInput(window);

        // Orbital Camera math
        if (pitch > 1.5f) pitch = 1.5f;
        if (pitch < -1.5f) pitch = -1.5f;

        float camX = zoom * cos(pitch) * sin(yaw);
        float camY = (zoom * sin(pitch)) + wingHeight; 
        float camZ = zoom * cos(pitch) * cos(yaw);

        glm::mat4 view = glm::lookAt(glm::vec3(camX, camY, camZ), glm::vec3(0.0f, wingHeight, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

        // Render Ground Grid
        glUseProgram(gridProg);
        glUniformMatrix4fv(glGetUniformLocation(gridProg, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(gridProg, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glBindVertexArray(gridVAO);
        glDrawArrays(GL_LINES, 0, gridVerts.size() / 3);

        // Render Wing with Pivot Correction
        glUseProgram(wingProg);
        glm::mat4 model = glm::mat4(1.0f);
        // Step 3: Raise wing to display height
        model = glm::translate(model, glm::vec3(0.0f, wingHeight, 0.0f)); 
        // Step 2: Inversion correction (standard for CAD -> OpenGL)
        model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        // Step 1: Normalize scale and center geometry on origin (0,0,0)
        model = glm::scale(model, glm::vec3(0.01f, 0.01f, 0.01f)); 
        model = glm::translate(model, -meshCenter); 

        glUniformMatrix4fv(glGetUniformLocation(wingProg, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(wingProg, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(wingProg, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, all_indices.size(), GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}