/**
 * fluid_renderer.cpp
 * Real-time 3D Particle Fluid Renderer — OpenGL 4.5 + freeglut + GLEW
 *
 * Build (Linux):
 *   g++ -std=c++17 -O2 -o fluid_renderer fluid_renderer.cpp \
 *       -lGL -lGLEW -lglut -lm
 *
 * Build (macOS — legacy GL):
 *   g++ -std=c++17 -O2 -o fluid_renderer fluid_renderer.cpp \
 *       -framework OpenGL -framework GLUT -lGLEW
 *
 * Build (Windows / MSVC, link: opengl32.lib glew32.lib freeglut.lib)
 *
 * Controls:
 *   Left-mouse drag  — orbit camera (azimuth / elevation)
 *   Right-mouse drag — zoom
 *   Mouse move       — push particles (passive force)
 *   +/-              — increase / decrease max-speed colour range
 *   R                — reset simulation
 *   ESC / Q          — quit
 */

// ─── Platform / include order matters ────────────────────────────────────────
#include <GL/glew.h>          // must come before any gl.h
#ifdef __APPLE__
  #include <GLUT/glut.h>
#else
  #include <GL/freeglut.h>
#endif

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <string>
#include <vector>
#include <algorithm>

// ─── Simulation constants ─────────────────────────────────────────────────────
static constexpr int   N_PARTICLES   = 500'000;   // raise to 2M if VRAM allows
static constexpr float BOX_HALF      = 1.0f;
static constexpr float DAMPING       = 0.85f;
static constexpr float DT            = 0.0035f;
static constexpr float MOUSE_RADIUS  = 0.35f;     // world-space influence radius
static constexpr float MOUSE_STRENGTH= 4.5f;

// ─── Window state ─────────────────────────────────────────────────────────────
static int   g_width  = 1280;
static int   g_height = 720;

// ─── Camera state ─────────────────────────────────────────────────────────────
static float g_azimuth   =  30.0f;   // degrees
static float g_elevation =  20.0f;   // degrees
static float g_distance  =   2.8f;

static int   g_lastMouseX = 0, g_lastMouseY = 0;
static bool  g_leftDown   = false;
static bool  g_rightDown  = false;

// Mouse force (world space delta passed to simulation)
static float g_mouseDX = 0.0f, g_mouseDY = 0.0f;
static float g_mouseNDC_X = 0.0f, g_mouseNDC_Y = 0.0f; // current pos [-1,1]
static float g_maxSpeed = 2.0f;

// ─── OpenGL handles ───────────────────────────────────────────────────────────
static GLuint g_vao          = 0;
static GLuint g_vbo[2]       = {};   // ping-pong position+speed buffers
static GLuint g_velVBO[2]    = {};   // ping-pong velocity buffers
static int    g_pingPong     = 0;

static GLuint g_renderProg   = 0;    // vertex + fragment

// FBO for optional depth-blur pass
static GLuint g_fbo          = 0;
static GLuint g_colorTex     = 0;
static GLuint g_depthTex     = 0;
static GLuint g_blurProg     = 0;
static GLuint g_quadVAO      = 0;
static GLuint g_quadVBO      = 0;

// ─── Shader source strings ────────────────────────────────────────────────────

// ---------- Particle vertex shader -------------------------------------------
static const char* VERT_SRC = R"GLSL(
#version 450 core

layout(location = 0) in vec4 aParticle;   // xyz = position, w = speed

uniform mat4 uMVP;
uniform float uMaxSpeed;

out float vSpeed;
flat out int  vClamped;

void main()
{
    vec3 pos = aParticle.xyz;
    vSpeed = aParticle.w;

    // Flag particles that are at / near the boundary
    vec3 clamped = clamp(pos, -1.0, 1.0);
    vClamped = int(any(notEqual(pos, clamped)));
    pos = clamped;

    gl_Position  = uMVP * vec4(pos, 1.0);

    // Depth-dependent point size: larger when closer
    float eyeZ    = -(uMVP * vec4(pos, 1.0)).w;  // approximate depth
    gl_PointSize  = clamp(2.2 / max(eyeZ * 0.4, 0.1), 1.5, 6.0);
}
)GLSL";

// ---------- Particle fragment shader -----------------------------------------
static const char* FRAG_SRC = R"GLSL(
#version 450 core

in  float vSpeed;
flat in int  vClamped;

uniform float uMaxSpeed;

out vec4 fragColor;

void main()
{
    // Circular splat — discard corners of the point quad
    vec2  pc   = gl_PointCoord - vec2(0.5);
    float dist = dot(pc, pc);
    if (dist > 0.25) discard;

    // Cool → warm colour ramp
    float t     = clamp(vSpeed / max(uMaxSpeed, 0.001), 0.0, 1.0);
    vec3  cold  = vec3(0.10, 0.30, 1.00);
    vec3  warm  = vec3(1.00, 0.10, 0.05);
    vec3  col   = mix(cold, warm, t);

    // Clamped particles pulse slightly whiter
    if (vClamped != 0)
        col = mix(col, vec3(1.0), 0.35);

    // Soft glow alpha — additive blending amplifies bright cores
    float alpha = smoothstep(0.5, 0.0, length(gl_PointCoord - vec2(0.5)));
    alpha = pow(alpha, 0.7);   // slightly broaden the glow

    fragColor = vec4(col * alpha, alpha);
}
)GLSL";

// ---------- Compute shader — simulation step ---------------------------------
// Each invocation handles one particle.
// Buffers: binding 0 = read positions (vec4), binding 1 = write positions (vec4)
//          binding 2 = read velocities (vec4), binding 3 = write velocities (vec4)

// ---------- Full-screen quad vertex shader (blur pass) -----------------------
static const char* QUAD_VERT_SRC = R"GLSL(
#version 450 core
layout(location = 0) in vec2 aPos;
out vec2 vUV;
void main() {
    vUV = aPos * 0.5 + 0.5;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)GLSL";

// ---------- Depth-of-field blur fragment shader ------------------------------
static const char* BLUR_FRAG_SRC = R"GLSL(
#version 450 core

in  vec2 vUV;
out vec4 fragColor;

uniform sampler2D uColor;
uniform sampler2D uDepth;
uniform vec2      uTexelSize;   // 1/width, 1/height

// Reconstruct linear depth from nonlinear depth buffer
float linearDepth(float d)
{
    float near = 0.01, far = 10.0;
    return (2.0 * near) / (far + near - d * (far - near));
}

void main()
{
    float rawDepth  = texture(uDepth, vUV).r;
    float linDepth  = linearDepth(rawDepth);

    // Focus plane at 0.3 linear depth (~1.5 world units from camera)
    float coc = clamp(abs(linDepth - 0.18) * 12.0, 0.0, 1.0);
    float radius = coc * 2.5;

    vec4 sum    = vec4(0.0);
    float weight = 0.0;

    const int SAMPLES = 9;
    // 3×3 Poisson-ish offsets
    vec2 offsets[9] = vec2[](
        vec2(-1,-1), vec2(0,-1), vec2(1,-1),
        vec2(-1, 0), vec2(0, 0), vec2(1, 0),
        vec2(-1, 1), vec2(0, 1), vec2(1, 1)
    );
    for (int i = 0; i < SAMPLES; ++i) {
        vec2 uv = vUV + offsets[i] * uTexelSize * radius;
        sum    += texture(uColor, uv);
        weight += 1.0;
    }
    fragColor = sum / weight;
}
)GLSL";

// ─── Utility: compile + link shaders ─────────────────────────────────────────
static GLuint compileShader(GLenum type, const char* src)
{
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, nullptr);
    glCompileShader(sh);
    GLint ok = 0;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[4096];
        glGetShaderInfoLog(sh, sizeof(log), nullptr, log);
        fprintf(stderr, "[GLSL compile error]\n%s\n", log);
        glDeleteShader(sh);
        return 0;
    }
    return sh;
}

static GLuint linkProgram(std::initializer_list<GLuint> shaders)
{
    GLuint prog = glCreateProgram();
    for (auto sh : shaders) glAttachShader(prog, sh);
    glLinkProgram(prog);
    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[4096];
        glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
        fprintf(stderr, "[GLSL link error]\n%s\n", log);
        glDeleteProgram(prog);
        return 0;
    }
    for (auto sh : shaders) { glDetachShader(prog, sh); glDeleteShader(sh); }
    return prog;
}

// ─── Matrix helpers (column-major, OpenGL convention) ─────────────────────────
struct Mat4 { float m[16] = {}; };

static Mat4 perspective(float fovY_deg, float aspect, float near, float far)
{
    Mat4 r;
    float f = 1.0f / tanf(fovY_deg * 3.14159265f / 360.0f);
    r.m[0]  =  f / aspect;
    r.m[5]  =  f;
    r.m[10] = (far + near) / (near - far);
    r.m[11] = -1.0f;
    r.m[14] = (2.0f * far * near) / (near - far);
    return r;
}

static Mat4 lookAt(float ex, float ey, float ez,
                   float cx, float cy, float cz,
                   float ux, float uy, float uz)
{
    // forward
    float fx = cx-ex, fy = cy-ey, fz = cz-ez;
    float fl = sqrtf(fx*fx+fy*fy+fz*fz);
    fx/=fl; fy/=fl; fz/=fl;
    // right = forward x up
    float rx = fy*uz-fz*uy, ry = fz*ux-fx*uz, rz = fx*uy-fy*ux;
    float rl = sqrtf(rx*rx+ry*ry+rz*rz);
    rx/=rl; ry/=rl; rz/=rl;
    // corrected up
    float ox = ry*fz-rz*fy, oy = rz*fx-rx*fz, oz = rx*fy-ry*fx;
    Mat4 r;
    r.m[0]=rx; r.m[4]=ry; r.m[8] =rz; r.m[12]=-(rx*ex+ry*ey+rz*ez);
    r.m[1]=ox; r.m[5]=oy; r.m[9] =oz; r.m[13]=-(ox*ex+oy*ey+oz*ez);
    r.m[2]=-fx;r.m[6]=-fy;r.m[10]=-fz;r.m[14]= (fx*ex+fy*ey+fz*ez);
    r.m[15]=1.0f;
    return r;
}

static Mat4 mul(const Mat4& a, const Mat4& b)
{
    Mat4 c;
    for (int row = 0; row < 4; ++row)
        for (int col = 0; col < 4; ++col)
            for (int k = 0; k < 4; ++k)
                c.m[col*4+row] += a.m[k*4+row] * b.m[col*4+k];
    return c;
}

// ─── FBO setup ───────────────────────────────────────────────────────────────
static void setupFBO(int w, int h)
{
    if (g_fbo) {
        glDeleteFramebuffers(1, &g_fbo);
        glDeleteTextures(1, &g_colorTex);
        glDeleteTextures(1, &g_depthTex);
    }
    glGenFramebuffers(1, &g_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);

    // colour attachment
    glGenTextures(1, &g_colorTex);
    glBindTexture(GL_TEXTURE_2D, g_colorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_HALF_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_colorTex, 0);

    // depth attachment
    glGenTextures(1, &g_depthTex);
    glBindTexture(GL_TEXTURE_2D, g_depthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, w, h, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, g_depthTex, 0);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
        fprintf(stderr, "FBO incomplete: 0x%x\n", status);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// ─── Initialisation ───────────────────────────────────────────────────────────
static void initParticles()
{
    // CPU-side initialisation: random positions + velocities
    std::mt19937                     rng(42);
    std::uniform_real_distribution<float> posD(-0.95f, 0.95f);
    std::uniform_real_distribution<float> velD(-0.4f,  0.4f);

    std::vector<float> positions(N_PARTICLES * 4);
    std::vector<float> velocities(N_PARTICLES * 4, 0.0f);

    for (int i = 0; i < N_PARTICLES; ++i) {
        positions[i*4+0] = posD(rng);
        positions[i*4+1] = posD(rng);
        positions[i*4+2] = posD(rng);
        positions[i*4+3] = 0.0f;  // initial speed magnitude

        velocities[i*4+0] = velD(rng);
        velocities[i*4+1] = velD(rng);
        velocities[i*4+2] = velD(rng);
        velocities[i*4+3] = 0.0f;
    }

    // Create ping-pong position VBOs
    glGenBuffers(2, g_vbo);
    for (int i = 0; i < 2; ++i) {
        glBindBuffer(GL_ARRAY_BUFFER, g_vbo[i]);
        glBufferData(GL_ARRAY_BUFFER,
                     N_PARTICLES * 4 * sizeof(float),
                     i == 0 ? positions.data() : nullptr,
                     GL_DYNAMIC_COPY);
        if (i == 1) // copy initial data to second buffer too
            glBufferData(GL_ARRAY_BUFFER, N_PARTICLES * 4 * sizeof(float),
                         positions.data(), GL_DRAW);
    }

    // Create ping-pong velocity SSBOs
    glGenBuffers(2, g_velVBO);
    for (int i = 0; i < 2; ++i) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, g_velVBO[i]);
        glBufferData(GL_SHADER_STORAGE_BUFFER,
                     N_PARTICLES * 4 * sizeof(float),
                     velocities.data(), GL_DRAW);
    }

    // VAO — always reads from the current "read" VBO (updated in display)
    glGenVertexArrays(1, &g_vao);
    glBindVertexArray(g_vao);
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo[g_pingPong]);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

static void initGL()
{
    // ── Render program ───────────────────────────────────────────────────────
    g_renderProg = linkProgram({
        compileShader(GL_VERTEX_SHADER,   VERT_SRC),
        compileShader(GL_FRAGMENT_SHADER, FRAG_SRC)
    });

    // ── Blur program ─────────────────────────────────────────────────────────
    g_blurProg = linkProgram({
        compileShader(GL_VERTEX_SHADER,   QUAD_VERT_SRC),
        compileShader(GL_FRAGMENT_SHADER, BLUR_FRAG_SRC)
    });

    // Full-screen quad
    float quadVerts[] = { -1,-1,  1,-1,  1,1,  -1,-1,  1,1,  -1,1 };
    glGenVertexArrays(1, &g_quadVAO);
    glGenBuffers(1, &g_quadVBO);
    glBindVertexArray(g_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    initParticles();
    struct cudaGraphicsResource* g_cudaPos[2];
    struct cudaGraphicsResource* g_cudaVel[2];

    for (int i = 0; i < 2; ++i) {
        cudaGraphicsGLRegisterBuffer(&g_cudaPos[i], g_vbo[i],
            cudaGraphicsMapFlagsWriteDiscard);

        cudaGraphicsGLRegisterBuffer(&g_cudaVel[i], g_velVBO[i],
            cudaGraphicsMapFlagsWriteDiscard);
    }

    setupFBO(g_width, g_height);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // additive — luminous fluid
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_POINT_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
    glDisable(GL_DEPTH_TEST);           // additive blending + no depth test = best glow
    glClearColor(0.0f, 0.0f, 0.03f, 1.0f);
}

// ─── Unproject mouse to z=0 plane ────────────────────────────────────────────
static void unprojectMouse(float ndcX, float ndcY, float& wx, float& wy)
{
    // Approximate: use camera distance and FOV to back-project to z=0 plane
    float az  = g_azimuth   * 3.14159265f / 180.0f;
    float el  = g_elevation * 3.14159265f / 180.0f;
    float eyeX = g_distance * cosf(el) * sinf(az);
    float eyeY = g_distance * sinf(el);
    float eyeZ = g_distance * cosf(el) * cosf(az);

    // Very simple: scale NDC by a rough world-space factor
    float scale = g_distance * tanf(30.0f * 3.14159265f / 180.0f);
    wx = ndcX * scale - eyeX * 0.0f;
    wy = ndcY * scale - eyeY * 0.0f;
    // A production renderer would do a proper ray-plane intersection;
    // this approximation is visually convincing for the mouse force field.
    wx = ndcX * 0.9f;
    wy = ndcY * 0.9f;
}

// ─── Simulation step (compute shader) ─────────────────────────────────────────
// Forward declaration (implemented in your CUDA .cu file)
extern void cudaStepSimulation(float time,
                               GLuint posVBO,
                               GLuint velVBO,
                               int    pingPong,
                               float  mouseDX,
                               float  mouseDY,
                               float  mouseNDC_X,
                               float  mouseNDC_Y);

static void stepSimulation(float time)
{
    cudaStepSimulation(time,
                       g_vbo[g_pingPong],
                       g_velVBO[g_pingPong],
                       g_pingPong,
                       g_mouseDX,
                       g_mouseDY,
                       g_mouseNDC_X,
                       g_mouseNDC_Y);

    // flip buffers (CUDA writes into "next")
    g_pingPong = 1 - g_pingPong;

    // decay mouse force
    g_mouseDX *= 0.85f;
    g_mouseDY *= 0.85f;
}

// ─── GLUT callbacks ───────────────────────────────────────────────────────────
static void display()
{
    static float startTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    float time = glutGet(GLUT_ELAPSED_TIME) / 1000.0f - startTime;

    // ── Simulation step ──────────────────────────────────────────────────────
    stepSimulation(time);

    // ── Build MVP ────────────────────────────────────────────────────────────
    float az = g_azimuth   * 3.14159265f / 180.0f;
    float el = g_elevation * 3.14159265f / 180.0f;
    float ex = g_distance * cosf(el) * sinf(az);
    float ey = g_distance * sinf(el);
    float ez = g_distance * cosf(el) * cosf(az);

    Mat4 view = lookAt(ex, ey, ez, 0,0,0, 0,1,0);
    Mat4 proj = perspective(60.0f, (float)g_width / g_height, 0.01f, 10.0f);
    Mat4 mvp  = mul(proj, view);

    // ── Render to FBO ────────────────────────────────────────────────────────
    glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
    glViewport(0, 0, g_width, g_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(g_renderProg);
    glUniformMatrix4fv(glGetUniformLocation(g_renderProg, "uMVP"), 1, GL_FALSE, mvp.m);
    glUniform1f(glGetUniformLocation(g_renderProg, "uMaxSpeed"), g_maxSpeed);

    // Rebind VAO to current read VBO (ping-pong just swapped)
    glBindVertexArray(g_vao);
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo[g_pingPong]);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);

    glDrawArrays(GL_POINTS, 0, N_PARTICLES);
    glBindVertexArray(0);

    // ── Depth-of-field blur pass → default framebuffer ───────────────────────
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, g_width, g_height);
    glClear(GL_COLOR_BUFFER_BIT);

    glDisable(GL_BLEND);  // blur pass: normal alpha

    glUseProgram(g_blurProg);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_colorTex);
    glUniform1i(glGetUniformLocation(g_blurProg, "uColor"), 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, g_depthTex);
    glUniform1i(glGetUniformLocation(g_blurProg, "uDepth"), 1);
    glUniform2f(glGetUniformLocation(g_blurProg, "uTexelSize"),
                1.0f / g_width, 1.0f / g_height);

    glBindVertexArray(g_quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glutSwapBuffers();
    glutPostRedisplay();  // continuous render

    // FPS display in window title
    static int   frames    = 0;
    static float lastPrint = 0.0f;
    ++frames;
    if (time - lastPrint >= 1.0f) {
        char title[128];
        snprintf(title, sizeof(title),
                 "Fluid Renderer  |  %dk particles  |  %.1f FPS  |  maxSpd=%.2f",
                 N_PARTICLES / 1000, frames / (time - lastPrint), g_maxSpeed);
        glutSetWindowTitle(title);
        frames    = 0;
        lastPrint = time;
    }
}

static void reshape(int w, int h)
{
    g_width  = w;
    g_height = std::max(h, 1);
    glViewport(0, 0, g_width, g_height);
    setupFBO(g_width, g_height);
}

static void mouseButton(int button, int state, int x, int y)
{
    g_lastMouseX = x;
    g_lastMouseY = y;
    if (button == GLUT_LEFT_BUTTON)
        g_leftDown  = (state == GLUT_DOWN);
    if (button == GLUT_RIGHT_BUTTON)
        g_rightDown = (state == GLUT_DOWN);
}

static void mouseMotion(int x, int y)
{
    int dx = x - g_lastMouseX;
    int dy = y - g_lastMouseY;

    if (g_leftDown) {
        g_azimuth   += dx * 0.4f;
        g_elevation += dy * 0.4f;
        g_elevation  = std::max(-89.0f, std::min(89.0f, g_elevation));
    }
    if (g_rightDown) {
        g_distance += dy * 0.01f;
        g_distance = std::max(0.5f, std::min(8.0f, g_distance));
    }

    // Mouse force: convert pixel delta to world-space impulse
    g_mouseDX += dx * (2.0f / g_width)  * 2.0f;
    g_mouseDY -= dy * (2.0f / g_height) * 2.0f;

    g_mouseNDC_X = (x / (float)g_width)  * 2.0f - 1.0f;
    g_mouseNDC_Y = 1.0f - (y / (float)g_height) * 2.0f;

    g_lastMouseX = x;
    g_lastMouseY = y;
}

static void passiveMotion(int x, int y)
{
    // Track mouse position even without buttons held (for force field)
    int dx = x - g_lastMouseX;
    int dy = y - g_lastMouseY;

    g_mouseDX += dx * (2.0f / g_width)  * 0.5f;
    g_mouseDY -= dy * (2.0f / g_height) * 0.5f;

    g_mouseNDC_X = (x / (float)g_width)  * 2.0f - 1.0f;
    g_mouseNDC_Y = 1.0f - (y / (float)g_height) * 2.0f;

    g_lastMouseX = x;
    g_lastMouseY = y;
}

static void keyboard(unsigned char key, int /*x*/, int /*y*/)
{
    switch (key) {
        case 27: case 'q': case 'Q':
            exit(0);
        case '+': case '=':
            g_maxSpeed = std::min(g_maxSpeed + 0.1f, 10.0f);
            break;
        case '-': case '_':
            g_maxSpeed = std::max(g_maxSpeed - 0.1f, 0.1f);
            break;
        case 'r': case 'R':
            // Re-initialise particles (reset simulation)
            initParticles();
            g_pingPong = 0;
            break;
    }
}

// ─── Entry point ─────────────────────────────────────────────────────────────
int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(g_width, g_height);
    glutInitContextVersion(4, 5);
    glutInitContextProfile(GLUT_CORE_PROFILE);
    glutCreateWindow("Fluid Renderer — initialising...");

    GLenum err = glewInit();
    if (err != GLEW_OK) {
        fprintf(stderr, "GLEW init failed: %s\n", glewGetErrorString(err));
        return 1;
    }

    // Verify we have the GL version we need
    const char* glver = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    printf("OpenGL: %s\n", glver);

    initGL();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouseButton);
    glutMotionFunc(mouseMotion);
    glutPassiveMotionFunc(passiveMotion);
    glutKeyboardFunc(keyboard);

    printf("Controls:\n"
           "  Left-drag   : orbit camera\n"
           "  Right-drag  : zoom\n"
           "  Mouse move  : push particles\n"
           "  +/-         : adjust colour speed range\n"
           "  R           : reset simulation\n"
           "  ESC/Q       : quit\n"
           "Particles: %d  (%.1f M)\n",
           N_PARTICLES, N_PARTICLES / 1e6f);

    glutMainLoop();
    return 0;
}
//)GLSL";
