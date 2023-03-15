// dear imgui: standalone example application for GLFW + OpenGL 3, using
// programmable pipeline If you are new to dear imgui, see examples/README.txt
// and documentation at the top of imgui.cpp. (GLFW is a cross-platform general
// purpose library for handling windows, inputs, OpenGL/Vulkan graphics context
// creation, etc.)

#include <cmath>
#include <iostream>
#include <stdio.h>

#include "imgui.h"
#include "imgui_impl/imgui_impl_glfw.h"
#include "imgui_impl/imgui_impl_opengl3.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"

#include <glad/glad.h>

#include <GLFW/glfw3.h> // Include glfw3.h after our OpenGL definitions
#include <spdlog/spdlog.h>
#include <stb_image.h>

#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

#include "utils/dd.h"
#include "utils/debug_draw.hpp"

#include "graphics/camera.h"
#include "graphics/entity.h"
#include "graphics/light.h"
#include "graphics/model.h"
#include "graphics/shader.h"
#include "graphics/skybox.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define SPOT_DEPTH_MAP_COUNT 10
#define DIRECTIONAL_DEPTH_MAP_COUNT 10
#define POINT_DEPTH_MAP_COUNT 10

#include "utils/gui.h"

#include "ImGuizmo.h"

// settings
int screenWidth = 1600;
int screenHeight = 900;

static ImGuizmo::OPERATION currentGizmoOperation(ImGuizmo::TRANSLATE);
static ImGuizmo::MODE currentGizmoMode(ImGuizmo::LOCAL);

void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

// camera
Camera camera(glm::vec3(-7.0f, 4.0f, 0.0f));
float lastX = screenWidth / 2.0f;
float lastY = screenHeight / 2.0f;
bool firstMouse = true;
bool cameraMouseControl = false;
bool drawDebugLights = true;

float viewportWidth = 0.0f;
float viewportHeight = 0.0f;
float resolutionScale = 2.0f;
float ambientIntensity = 1.0f;

// timing
float deltaTime = 0.0f; // time between current frame and last frame
float lastFrame = 0.0f;

bool useFxaa = false;
bool fxaaDebugDraw = false;
float lumaThreshold = 0.5f;

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    } else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

class MyRenderInterface : public dd::RenderInterface {
public:
    Shader shader;

    MyRenderInterface()
        : shader("debug.vert", "debug.frag")
    {
    }

    void drawLineList(const dd::DrawVertex* lines, int count, bool depthEnabled) override
    {
        shader.use();
        shader.setMat4("projection", camera.getProjectionMatrix(viewportWidth, viewportHeight));
        shader.setMat4("view", camera.getViewMatrix());
        shader.setMat4("model", glm::mat4(1.0f));

        unsigned int VBO, VAO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, count * sizeof(dd::DrawVertex), lines, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(dd::DrawVertex), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(dd::DrawVertex), (void*)(3 * sizeof(float)));

        glDrawArrays(GL_LINES, 0, count);

        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }
};

int main(int, char**)
{
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);

    if (!glfwInit())
        return 1;

        // Decide GL+GLSL versions
#if __APPLE__
    // GL 4.1 + GLSL 410
    const char* glsl_version = "#version 410";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Required on Mac
#else
    // GL 4.3 + GLSL 430
    const char* glsl_version = "#version 430";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // 3.0+ only
#endif

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(
        screenWidth, screenHeight, "Dear ImGui GLFW+OpenGL3 example", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetKeyCallback(window, key_callback);

    // Initialize OpenGL loader
    bool err = !gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    if (err) {
        spdlog::error("Failed to initialize OpenGL loader!");
        return 1;
    }
    spdlog::info("Successfully initialized OpenGL loader!");

    // Setup Dear ImGui binding
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform Windows
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Setup style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsClassic();
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;

    bool show_demo_window = false;
    ImVec4 clear_color = ImVec4(0.1f, 0.1f, 0.1f, 1.00f);

    stbi_set_flip_vertically_on_load(false);

    camera.ProcessMouseMovement(90.0f / SENSITIVITY, -15.0f / SENSITIVITY);

    Entity scene_root = Entity("Scene Root");

    float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
        // positions   // texCoords
        -1.0f, 1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
        1.0f, -1.0f, 1.0f, 0.0f,

        -1.0f, 1.0f, 0.0f, 1.0f,
        1.0f, -1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f
    };
    // screen quad VAO
    unsigned int quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    Shader pbrAmbientShader("pbr/pbr.vert", "pbr/pbr_ambient.frag");
    Shader pbrDirectionalShader("pbr/pbr.vert", "pbr/pbr_directional.frag");
    Shader pbrPointShader("pbr/pbr.vert", "pbr/pbr_point.frag");
    Shader pbrSpotlightShader("pbr/pbr.vert", "pbr/pbr_spotlight.frag");

    Shader shadowMapShader("shadow_map.vert", "shadow_map.frag");
    Shader pointShadowMapShader("point_shadow_map.vert", "point_shadow_map.frag", "point_shadow_map.geom");

    Shader postprocessShader("postprocess.vert", "postprocess.frag");

    Shader instancedShader("instanced.vert", "instanced.frag");
    Shader terrainShader("terrain.vert", "terrain.frag", "terrain.geom");

    Shader backgroundShader("background.vert", "background.frag");

    // scene_root.addChild(std::make_unique<Model>("Sponza", "resources/models/bistro/bistro.gltf"));
    scene_root.addChild(std::make_unique<Model>("Sponza", "resources/models/sponza/Sponza.gltf"));
    Entity* sponza = scene_root.children.back().get();
    sponza->transform.scale = { 0.01, 0.01, 0.01 };

    scene_root.addChild(std::make_unique<Model>("Boombox", "resources/models/boombox/Boombox.gltf"));
    Entity* boombox = scene_root.children.back().get();
    boombox->transform.pos = { 1.4, 0.46, 0.87 };
    boombox->transform.scale = { 50.0, 50.0, 50.0 };
    scene_root.addChild(std::make_unique<Model>("Helmet", "resources/models/damaged_helmet/DamagedHelmet.gltf"));
    Entity* helmet = scene_root.children.back().get();
    helmet->transform.pos = { 1.8, 1.25, -1.0 };
    helmet->transform.orient = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * helmet->transform.orient;
    helmet->transform.orient = glm::angleAxis(glm::radians(-45.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * helmet->transform.orient;
    helmet->transform.scale = { 0.5, 0.5, 0.5 };

    scene_root.addChild(std::make_unique<Entity>("Lights"));
    Entity* lights = scene_root.children.back().get();

    // Spawn 16 lights in circle
    float scale = 5.0;
    int lightCount = 2;
    lights->transform.pos = { 0, 3.25, 0 };
    glm::vec3 lightColors1 = { 0.0, 0.0, 1.0 };
    glm::vec3 lightColors2 = { 0.0, 1.0, 0.0 };
    for (int i = 0; i < lightCount; i++) {
        lights->addChild(std::make_unique<Light>(("Light " + std::to_string(i)).c_str(), LightType::POINT));
        Light* light = (Light*)lights->children.back().get();
        light->transform.pos = { cos(i * 2 * M_PI / lightCount) * scale, 0, sin(i * 2 * M_PI / lightCount) * scale };
        light->intensity = 50;
        light->color = (i % 2 == 0) ? lightColors1 : lightColors2;
        light->transform.orient = glm::angleAxis(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * light->transform.orient;
    }

    scene_root.addChild(std::make_unique<Light>("Light", LightType::DIRECTIONAL));
    Light* light = (Light*)scene_root.children.back().get();
    light->transform.pos = { 0, 14, 0 };
    light->transform.orient = glm::angleAxis(glm::radians(-85.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * light->transform.orient;

    scene_root.updateSelfAndChildren();

    postprocessShader.use();
    postprocessShader.setInt("screenTexture", 0);
    postprocessShader.setVec2("u_texelStep", 1.0f / screenWidth, 1.0f / screenHeight);
    postprocessShader.setBool("u_fxaaOn", useFxaa);
    postprocessShader.setBool("u_showEdges", fxaaDebugDraw);
    postprocessShader.setFloat("u_lumaThreshold", 0.5f);
    postprocessShader.setFloat("u_mulReduce", 1.0f / 8.0f);
    postprocessShader.setFloat("u_minReduce", 1.0f / 128.0f);
    postprocessShader.setFloat("u_maxSpan", 8.0f);

    pbrAmbientShader.use();
    pbrAmbientShader.setInt("irradianceMap", 0);
    pbrAmbientShader.setInt("prefilterMap", 1);
    pbrAmbientShader.setInt("brdfLUT", 2);

    pbrAmbientShader.setInt("albedo_map", 3);
    pbrAmbientShader.setInt("normal_map", 4);
    pbrAmbientShader.setInt("metallic_map", 5);
    pbrAmbientShader.setInt("roughness_map", 6);
    pbrAmbientShader.setInt("ao_map", 7);
    pbrAmbientShader.setInt("emission_map", 8);

    pbrDirectionalShader.use();
    pbrDirectionalShader.setInt("albedo_map", 3);
    pbrDirectionalShader.setInt("normal_map", 4);
    pbrDirectionalShader.setInt("metallic_map", 5);
    pbrDirectionalShader.setInt("roughness_map", 6);
    pbrDirectionalShader.setInt("ao_map", 7);
    for (int i = 0; i < DIRECTIONAL_DEPTH_MAP_COUNT; i++) {
        pbrDirectionalShader.setInt("shadow_maps[" + std::to_string(i) + "]", 9 + i);
    }

    pbrPointShader.use();
    pbrPointShader.setInt("albedo_map", 3);
    pbrPointShader.setInt("normal_map", 4);
    pbrPointShader.setInt("metallic_map", 5);
    pbrPointShader.setInt("roughness_map", 6);
    pbrPointShader.setInt("ao_map", 7);
    for (int i = 0; i < POINT_DEPTH_MAP_COUNT; i++) {
        pbrPointShader.setInt("shadow_maps[" + std::to_string(i) + "]", 9 + i);
    }

    pbrSpotlightShader.use();
    pbrSpotlightShader.setInt("albedo_map", 3);
    pbrSpotlightShader.setInt("normal_map", 4);
    pbrSpotlightShader.setInt("metallic_map", 5);
    pbrSpotlightShader.setInt("roughness_map", 6);
    pbrSpotlightShader.setInt("ao_map", 7);
    pbrSpotlightShader.setInt("shadow_map", 9);
    for (int i = 0; i < SPOT_DEPTH_MAP_COUNT; i++) {
        pbrSpotlightShader.setInt("shadow_maps[" + std::to_string(i) + "]", 9 + i);
    }

    backgroundShader.use();
    backgroundShader.setInt("environmentMap", 0);

    stbi_set_flip_vertically_on_load(true);

    // pbr: setup cubemap, irradiance map and prefilter map
    // ----------------------------------------------------
    Skybox skybox = Skybox("resources/textures/hdr/blaubeuren_church_square_4k.hdr");
    skybox.generateCubemap(2048);
    skybox.generateIrradianceMap();

    // load PBR material textures
    // --------------------------
    // rusted iron
    unsigned int ironAlbedoMap = loadTexture("resources/textures/pbr/rusted_iron/albedo.png");
    unsigned int ironNormalMap = loadTexture("resources/textures/pbr/rusted_iron/normal.png");
    unsigned int ironMetallicMap = loadTexture("resources/textures/pbr/rusted_iron/metallic.png");
    unsigned int ironRoughnessMap = loadTexture("resources/textures/pbr/rusted_iron/roughness.png");
    unsigned int ironAOMap = loadTexture("resources/textures/pbr/rusted_iron/ao.png");

    // load cube model
    // ------------------------------------------------------------------
    Model box_textured("Box Textured", "resources/models/box_textured/BoxTextured.gltf");

    // generate a large list of semi-random model transformation matrices
    // ------------------------------------------------------------------
    unsigned int amount = 1000000;
    glm::mat4* modelMatrices;
    modelMatrices = new glm::mat4[amount];
    srand(static_cast<unsigned int>(glfwGetTime())); // initialize random seed
    float radius = 100.0;
    float offset = 25.0f;
    for (unsigned int i = 0; i < amount; i++) {
        glm::mat4 model = glm::mat4(1.0f);
        // 1. translation: displace along circle with 'radius' in range [-offset, offset]
        float angle = (float)i / (float)amount * 360.0f;
        float displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float x = sin(angle) * radius + displacement;
        displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float y = displacement * 0.4f; // keep height of asteroid field smaller compared to width of x and z
        displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float z = cos(angle) * radius + displacement;
        model = glm::translate(model, glm::vec3(x, y, z));

        // 2. scale: Scale between 0.05 and 0.25f
        float scale = static_cast<float>((rand() % 20) / 100.0 + 0.05) * 0.5f;
        model = glm::scale(model, glm::vec3(scale));

        // 3. rotation: add random rotation around a (semi)randomly picked rotation axis vector
        float rotAngle = static_cast<float>((rand() % 360));
        model = glm::rotate(model, rotAngle, glm::vec3(0.4f, 0.6f, 0.8f));

        // 4. now add to list of matrices
        modelMatrices[i] = model;
    }

    // terrain
    int heightmapTexture = loadTexture("resources/textures/heightmap.png");
    Model terrain("Terrain", "resources/models/plane.gltf");
    terrainShader.use();
    terrainShader.setInt("heightMap", 0);

    // configure instanced array
    // -------------------------
    unsigned int buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, amount * sizeof(glm::mat4), &modelMatrices[0], GL_STATIC_DRAW);

    // set transformation matrices as an instance vertex attribute (with divisor 1)
    // note: we're cheating a little by taking the, now publicly declared, VAO of the model's mesh(es) and adding new vertexAttribPointers
    // normally you'd want to do this in a more organized fashion, but for learning purposes this will do.
    // -----------------------------------------------------------------------------------------------------------------------------------
    for (unsigned int i = 0; i < box_textured.meshes.size(); i++) {
        unsigned int VAO = box_textured.meshes[i].VAO;
        glBindVertexArray(VAO);
        // set attribute pointers for matrix (4 times vec4)
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)0);
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4)));
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(2 * sizeof(glm::vec4)));
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(3 * sizeof(glm::vec4)));

        glVertexAttribDivisor(3, 1);
        glVertexAttribDivisor(4, 1);
        glVertexAttribDivisor(5, 1);
        glVertexAttribDivisor(6, 1);

        glBindVertexArray(0);
    }

    unsigned int fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    ImGuiWindowFlags viewportWindowFlags = 0;

    DDRenderInterfaceCoreGL renderIface;
    dd::initialize(&renderIface);

    unsigned int depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);
    const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;

    std::vector<int> directionalDepthMaps;
    std::vector<int> pointDepthMaps;
    std::vector<int> spotDepthMaps;

    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };

    for (int i = 0; i < DIRECTIONAL_DEPTH_MAP_COUNT; i++) {
        unsigned int directionalDepthMap;
        glGenTextures(1, &directionalDepthMap);
        glBindTexture(GL_TEXTURE_2D, directionalDepthMap);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
            SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
        directionalDepthMaps.push_back(directionalDepthMap);
    }

    for (int i = 0; i < SPOT_DEPTH_MAP_COUNT; i++) {
        unsigned int spotlightDepthMap;
        glGenTextures(1, &spotlightDepthMap);
        glBindTexture(GL_TEXTURE_2D, spotlightDepthMap);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
            SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
        spotDepthMaps.push_back(spotlightDepthMap);
    }

    for (int i = 0; i < POINT_DEPTH_MAP_COUNT; i++) {
        unsigned int pointDepthMap;
        glGenTextures(1, &pointDepthMap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, pointDepthMap);
        for (unsigned int i = 0; i < 6; ++i)
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT,
                SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        pointDepthMaps.push_back(pointDepthMap);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, directionalDepthMaps[0], 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to
        // tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data
        // to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input
        // data to your main application. Generally you may always pass all
        // inputs to dear imgui, and hide them from your application based on
        // those two flags.
        glfwPollEvents();

        glfwGetWindowSize(window, &screenWidth, &screenHeight);

        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        lights->transform.orient = glm::angleAxis(glm::radians(-45.0f) * deltaTime * 0.75f, glm::vec3(0.0f, 1.0f, 0.0f)) * lights->transform.orient;
        lights->transform.isDirty = true;

        processInput(window);

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in
        // ImGui::ShowDemoWindow()! You can browse its code to learn more about
        // Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        Entity* last_selected = nullptr;

        bool dockspace_open = true;
        ShowExampleAppDockSpace(&dockspace_open);

        // 2. Show a simple window that we create ourselves. We use a Begin/End
        // pair to created a named window.
        {
            static float f = 0.0f;

            ImGui::Begin("Scene Graph");

            static ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
            static bool align_label_with_current_x_position = false;
            static bool test_drag_and_drop = false;

            // 'selection_mask' is dumb representation of what may be user-side selection state.
            //  You may retain selection state inside or outside your objects in whatever format you see fit.
            // 'node_clicked' is temporary storage of what node we have clicked to process selection at the end
            /// of the loop. May be a pointer to your own node type, etc.
            static std::vector<Entity*> selectedEntities = std::vector<Entity*>();
            Entity* entity_clicked = nullptr;

            Entity* root = &scene_root;
            entityEntry(root, &entity_clicked, &selectedEntities);

            if (entity_clicked != nullptr) {
                // Update selection state
                // (process outside of tree loop to avoid visual inconsistencies during the clicking frame)
                if (ImGui::GetIO().KeyCtrl)
                    if (std::find(selectedEntities.begin(), selectedEntities.end(), entity_clicked) != selectedEntities.end())
                        selectedEntities.erase(std::remove(selectedEntities.begin(), selectedEntities.end(), entity_clicked), selectedEntities.end());
                    else
                        selectedEntities.push_back(entity_clicked);
                else {
                    selectedEntities.clear();
                    selectedEntities.push_back(entity_clicked);
                }
            }
            if (selectedEntities.size() > 0)
                last_selected = selectedEntities[selectedEntities.size() - 1];

            ImGui::End();
        }

        {
            ImGui::Begin("Inspector");

            if (last_selected != nullptr) {
                ImGui::Text("Selected: %s", last_selected->name.c_str());
                static char buffer[50];
                strcpy(buffer, last_selected->name.c_str());
                ImGui::InputText("Name", buffer, 50);
                last_selected->name = buffer;
                if (Model* model = dynamic_cast<Model*>(last_selected)) {
                    ImGui::Checkbox("Is Refractive", &model->isRefractive);
                }
                glm::vec3 rot = glm::eulerAngles(last_selected->transform.orient);
                rot = glm::degrees(rot);
                ImGui::Text("Transform");
                if (ImGui::DragFloat3("Position", &last_selected->transform.pos.x, 0.1f)
                    // || ImGui::DragFloat4("Orientation", &last_selected->transform.orient.x, 0.1f)
                    || ImGui::DragFloat3("Rotation", &rot.x, 0.1f)
                    || ImGui::DragFloat3("Scale", &last_selected->transform.scale.x, 0.1f)) {
                    // TODO: Wrap rotation around
                    last_selected->transform.orient = glm::quat(glm::radians(rot));
                    last_selected->transform.isDirty = true;
                }
                if (Light* light = dynamic_cast<Light*>(last_selected)) {
                    ImGui::Text("Light properties");
                    const char* items[] = { "Directional", "Point", "Spotlight" };
                    if (ImGui::BeginCombo("Type", items[static_cast<int>(light->type)])) {
                        for (int n = 0; n < IM_ARRAYSIZE(items); n++) {
                            bool is_selected = (static_cast<int>(light->type) == n);
                            if (ImGui::Selectable(items[n], is_selected)) {
                                light->type = static_cast<LightType>(n);
                            }
                            if (is_selected)
                                ImGui::SetItemDefaultFocus();
                        }

                        ImGui::EndCombo();
                    }
                    ImGui::ColorPicker3("Color", &light->color.x);
                    ImGui::DragFloat("Intensity", &light->intensity, 0.1f, 0.0f);
                    if (light->type == LightType::SPOT) {
                        ImGui::DragFloat("Spotlight Inner Angle", &light->innerAngle, 0.1f, 0.0f, 180.0f);
                        ImGui::DragFloat("Spotlight Outer Angle", &light->outerAngle, 0.1f, 0.0f, 180.0f);
                        // ImGui::DragFloat("Spotlight Falloff", &light->cutOffDistance, 0.1f, 0.0f, 20.0f);
                    }
                    if (light->type == LightType::POINT) {
                        // ImGui::DragFloat("Radius", &light->radius, 0.1f, 0.0f, 20.0f);
                    }
                }
            }

            ImGui::End();
        }

        {
            ImGui::Begin("Misc");

            ImGui::DragFloat("Resolution Scale", &resolutionScale, 0.01f, 0.25f, 4.0f, "%.2f");

            ImGui::DragFloat("Ambient Intensity", &ambientIntensity, 0.01f, 0.0f, 1.0f, "%.2f");

            ImGui::Checkbox("FXAA", &useFxaa);
            ImGui::Checkbox("FXAA Debug Draw", &fxaaDebugDraw);
            ImGui::DragFloat("LUMA Threshold", &lumaThreshold, 0.01f, 0.0f, 1.0f, "%.2f");

            ImGui::Checkbox("Draw Debug Lights", &drawDebugLights);
            static bool drawGrid = false;
            ImGui::Checkbox("Draw Grid", &drawGrid);
            if (drawGrid) {
                dd::xzSquareGrid(-10.0f, 10.0f, 0.0f, 1.0f, dd::colors::Gray);
            }

            ImGui::Checkbox("Demo Window",
                &show_demo_window); // Edit bools storing our window
                                    // open/close state

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);

            ImGui::End();
        }

        {
            ImGui::Begin("Gizmo");

            ImGui::Columns(2, "mycolumns2", false);

            ImGui::Text("Operation");
            ImGui::RadioButton("Position", (int*)&currentGizmoOperation, (int)ImGuizmo::TRANSLATE);
            ImGui::RadioButton("Rotation", (int*)&currentGizmoOperation, (int)ImGuizmo::ROTATE);
            ImGui::RadioButton("Scale", (int*)&currentGizmoOperation, (int)ImGuizmo::SCALE);

            ImGui::NextColumn();

            ImGui::Text("Mode");
            ImGui::RadioButton("Local", (int*)&currentGizmoMode, (int)ImGuizmo::LOCAL);
            ImGui::RadioButton("World", (int*)&currentGizmoMode, (int)ImGuizmo::WORLD);

            ImGui::End();
        }

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        GLuint renderTexture;
        GLuint postprocessTexture;
        unsigned int rbo;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        ImGui::Begin("Viewport", 0, viewportWindowFlags);
        {
            ImVec2 content_size = ImGui::GetContentRegionAvail();
            viewportWidth = content_size.x * resolutionScale;
            viewportHeight = content_size.y * resolutionScale;

            glm::mat4 projection = camera.getProjectionMatrix(viewportWidth, viewportHeight);
            glm::mat4 view = camera.getViewMatrix();

            glGenTextures(1, &renderTexture);
            glBindTexture(GL_TEXTURE_2D, renderTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, viewportWidth, viewportHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glBindTexture(GL_TEXTURE_2D, 0);

            glGenTextures(1, &postprocessTexture);
            glBindTexture(GL_TEXTURE_2D, postprocessTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, viewportWidth, viewportHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glBindTexture(GL_TEXTURE_2D, 0);

            // attach it to currently bound framebuffer object
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderTexture, 0);

            glGenRenderbuffers(1, &rbo);
            glBindRenderbuffer(GL_RENDERBUFFER, rbo);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, viewportWidth, viewportHeight);
            glBindRenderbuffer(GL_RENDERBUFFER, 0);

            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

            glViewport(0, 0, viewportWidth, viewportHeight);

            glEnable(GL_DEPTH_TEST);
            // set depth function to less than AND equal for skybox depth trick.
            glDepthFunc(GL_LEQUAL);
            // enable seamless cubemap sampling for lower mip levels in the pre-filter map.
            glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

            // Clear screen
            glClearColor(clear_color.x, clear_color.y, clear_color.z,
                clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            int display_w, display_h;
            glfwMakeContextCurrent(window);
            glfwGetFramebufferSize(window, &display_w, &display_h);

            // Set shader uniforms
            pbrDirectionalShader.use();
            pbrDirectionalShader.setMat4("projection", projection);
            pbrDirectionalShader.setMat4("view", view);
            pbrDirectionalShader.setVec3("camPos", camera.Position);

            pbrPointShader.use();
            pbrPointShader.setMat4("projection", projection);
            pbrPointShader.setMat4("view", view);
            pbrPointShader.setVec3("camPos", camera.Position);

            pbrSpotlightShader.use();
            pbrSpotlightShader.setMat4("projection", projection);
            pbrSpotlightShader.setMat4("view", view);
            pbrSpotlightShader.setVec3("camPos", camera.Position);

            pbrAmbientShader.use();
            pbrAmbientShader.setMat4("projection", projection);
            pbrAmbientShader.setMat4("view", view);
            pbrAmbientShader.setVec3("camPos", camera.Position);

            pbrAmbientShader.setFloat("ambientIntensity", ambientIntensity);

            // bind pre-computed IBL data
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.getIrradianceMap());
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.getPrefilterMap());
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, skybox.getBrdfLUTTexture());

            // rusted iron
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, ironAlbedoMap);
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, ironNormalMap);
            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_2D, ironMetallicMap);
            glActiveTexture(GL_TEXTURE6);
            glBindTexture(GL_TEXTURE_2D, ironRoughnessMap);
            glActiveTexture(GL_TEXTURE7);
            glBindTexture(GL_TEXTURE_2D, ironAOMap);

            glm::mat4 model = glm::mat4(1.0f);

            std::vector<Model*> models;
            std::vector<Light*> lights;
            std::vector<Entity*> misc_entities;

            // FIFO container
            std::vector<Entity*> entityQueue;
            entityQueue.push_back(&scene_root);
            while (entityQueue.size()) {
                Entity* entity = entityQueue.back();
                entityQueue.pop_back();

                if (Model* model = dynamic_cast<Model*>(entity)) {
                    models.push_back(model);
                } else if (Light* light = dynamic_cast<Light*>(entity)) {
                    lights.push_back(light);
                } else {
                    misc_entities.push_back(entity);
                }

                for (auto& child : entity->children) {
                    entityQueue.push_back(child.get());
                }
            }

            int directionalLightCount = 0;
            int pointLightCount = 0;
            int spotLightCount = 0;

            std::vector<glm::mat4> spotLightSpaceMatrices;
            std::vector<glm::mat4> directionalLightSpaceMatrices;

            for (auto& light : lights) {
                glm::vec3 lightPos = glm::vec3(light->transform.modelMatrix[3]);
                if (light->type == LightType::DIRECTIONAL) {
                    glm::vec3 lightDir = glm::vec3(light->transform.modelMatrix * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
                    glm::vec3 to = lightPos + lightDir;
                    if (drawDebugLights)
                        dd::arrow(&lightPos[0], &to[0], &light->color[0], 0.25f);

                    pbrDirectionalShader.use();
                    pbrDirectionalShader.setVec3("lights[" + std::to_string(directionalLightCount) + "].direction", -lightDir);
                    pbrDirectionalShader.setVec3("lights[" + std::to_string(directionalLightCount) + "].color", light->color);
                    pbrDirectionalShader.setFloat("lights[" + std::to_string(directionalLightCount) + "].intensity", light->intensity);

                    if (directionalLightCount >= POINT_DEPTH_MAP_COUNT) {
                        directionalLightCount++;
                        continue;
                    }

                    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
                    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, directionalDepthMaps[directionalLightCount], 0);
                    glClear(GL_DEPTH_BUFFER_BIT);
                    glCullFace(GL_FRONT);

                    float nearPlane = 1.0f, farPlane = 20.0f;
                    float size = 20.0f;
                    glm::mat4 lightProjection = glm::ortho(-size, size, -size, size, nearPlane, farPlane);
                    glm::mat4 lightView = glm::lookAt(lightPos, lightPos + lightDir, glm::vec3(0.0f, 1.0f, 0.0f));
                    glm::mat4 lightSpaceMatrix = lightProjection * lightView;
                    directionalLightSpaceMatrices.push_back(lightSpaceMatrix);
                    shadowMapShader.use();
                    shadowMapShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
                    for (auto& model : models) {
                        glm::mat4 modelMatrix = model->transform.modelMatrix;
                        shadowMapShader.setMat4("model", modelMatrix);
                        model->Draw(shadowMapShader);
                    }

                    glCullFace(GL_BACK);

                    directionalLightCount++;
                } else if (light->type == LightType::POINT) {
                    if (drawDebugLights)
                        dd::cross(&lightPos[0], 0.5f);
                    // dd::sphere(&lightPos[0], &light->color[0], 0.25f);

                    pbrPointShader.use();
                    pbrPointShader.setVec3("lights[" + std::to_string(pointLightCount) + "].position", lightPos);
                    pbrPointShader.setVec3("lights[" + std::to_string(pointLightCount) + "].color", light->color);
                    pbrPointShader.setFloat("lights[" + std::to_string(pointLightCount) + "].intensity", light->intensity);

                    if (pointLightCount >= POINT_DEPTH_MAP_COUNT) {
                        pointLightCount++;
                        continue;
                    }

                    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
                    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
                    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, pointDepthMaps[pointLightCount], 0);
                    glDrawBuffer(GL_NONE);
                    glReadBuffer(GL_NONE);

                    glClear(GL_DEPTH_BUFFER_BIT);
                    glCullFace(GL_FRONT);

                    float aspect = (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT;
                    float near_plane = 1.0f;
                    float far_plane = 25.0f;
                    glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), aspect, near_plane, far_plane);

                    std::vector<glm::mat4> shadowTransforms;

                    shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
                    shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
                    shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)));
                    shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0)));
                    shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0)));
                    shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0)));

                    pointShadowMapShader.use();

                    for (unsigned int i = 0; i < 6; ++i)
                        pointShadowMapShader.setMat4("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);

                    pointShadowMapShader.setFloat("far_plane", far_plane);
                    pointShadowMapShader.setVec3("lightPos", lightPos);

                    for (auto& model : models) {
                        glm::mat4 modelMatrix = model->transform.modelMatrix;
                        pointShadowMapShader.setMat4("model", modelMatrix);
                        model->Draw(pointShadowMapShader);
                    }

                    pointLightCount++;
                } else if (light->type == LightType::SPOT) {
                    glm::vec3 lightDir = glm::vec3(light->transform.modelMatrix * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)) * light->intensity;
                    float baseRadius = tanf(glm::radians(light->outerAngle)) * light->intensity;
                    if (drawDebugLights)
                        dd::cone(&lightPos[0], &lightDir[0], &light->color[0], baseRadius, 0.0f);

                    pbrSpotlightShader.use();
                    pbrSpotlightShader.setVec3("lights[" + std::to_string(spotLightCount) + "].position", lightPos);
                    pbrSpotlightShader.setVec3("lights[" + std::to_string(spotLightCount) + "].direction", glm::normalize(lightDir));
                    pbrSpotlightShader.setVec3("lights[" + std::to_string(spotLightCount) + "].color", light->color);
                    pbrSpotlightShader.setFloat("lights[" + std::to_string(spotLightCount) + "].intensity", light->intensity);
                    pbrSpotlightShader.setFloat("lights[" + std::to_string(spotLightCount) + "].innerAngle", glm::cos(glm::radians(light->innerAngle)));
                    pbrSpotlightShader.setFloat("lights[" + std::to_string(spotLightCount) + "].outerAngle", glm::cos(glm::radians(light->outerAngle)));

                    if (spotLightCount >= SPOT_DEPTH_MAP_COUNT) {
                        spotLightCount++;
                        continue;
                    }

                    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
                    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, spotDepthMaps[spotLightCount], 0);
                    glClear(GL_DEPTH_BUFFER_BIT);
                    glCullFace(GL_FRONT);

                    float nearPlane = 0.1f, farPlane = 20.0f;
                    glm::mat4 lightProjection = glm::perspective(glm::radians(90.0f), 1.0f, nearPlane, farPlane);
                    glm::mat4 lightView = glm::lookAt(lightPos, lightPos + glm::normalize(lightDir), glm::vec3(0.0f, 1.0f, 0.0f));
                    glm::mat4 lightSpaceMatrix = lightProjection * lightView;
                    spotLightSpaceMatrices.push_back(lightSpaceMatrix);
                    shadowMapShader.use();
                    shadowMapShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
                    for (auto& model : models) {
                        glm::mat4 modelMatrix = model->transform.modelMatrix;
                        shadowMapShader.setMat4("model", modelMatrix);
                        model->Draw(shadowMapShader);
                    }

                    glCullFace(GL_BACK);

                    spotLightCount++;
                }
            }

            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            glViewport(0, 0, viewportWidth, viewportHeight);

            pbrDirectionalShader.use();
            pbrDirectionalShader.setInt("lightCount", directionalLightCount);

            pbrPointShader.use();
            pbrPointShader.setInt("lightCount", pointLightCount);

            pbrSpotlightShader.use();
            pbrSpotlightShader.setInt("lightCount", spotLightCount);

            for (auto& model : models) {
                pbrAmbientShader.use();
                pbrAmbientShader.setMat4("model", model->transform.modelMatrix);
                model->Draw(pbrAmbientShader);

                if (model->isRefractive) {
                    continue;
                }

                glEnable(GL_BLEND);
                glBlendFunc(GL_ONE, GL_ONE);
                glDepthMask(GL_FALSE);
                glDepthFunc(GL_EQUAL);

                if (directionalLightCount > 0) {
                    pbrDirectionalShader.use();
                    pbrDirectionalShader.setMat4("model", model->transform.modelMatrix);
                    for (int i = 0; i < directionalLightCount; i++) {
                        pbrDirectionalShader.setMat4("lightSpaceMatrices[" + std::to_string(i) + "]", directionalLightSpaceMatrices[i]);
                        glActiveTexture(GL_TEXTURE9 + i);
                        glBindTexture(GL_TEXTURE_2D, directionalDepthMaps[i]);
                    }
                    model->Draw(pbrDirectionalShader);
                }

                if (pointLightCount > 0) {
                    pbrPointShader.use();
                    pbrPointShader.setMat4("model", model->transform.modelMatrix);
                    pbrPointShader.setVec3("viewPos", camera.Position);
                    for (int i = 0; i < pointLightCount; i++) {
                        glActiveTexture(GL_TEXTURE9 + i);
                        glBindTexture(GL_TEXTURE_CUBE_MAP, pointDepthMaps[i]);
                    }
                    model->Draw(pbrPointShader);
                }

                if (spotLightCount > 0) {
                    pbrSpotlightShader.use();
                    pbrSpotlightShader.setMat4("model", model->transform.modelMatrix);
                    for (int i = 0; i < spotLightCount; i++) {
                        pbrSpotlightShader.setMat4("lightSpaceMatrices[" + std::to_string(i) + "]", spotLightSpaceMatrices[i]);
                        glActiveTexture(GL_TEXTURE9 + i);
                        glBindTexture(GL_TEXTURE_2D, spotDepthMaps[i]);
                    }
                    model->Draw(pbrSpotlightShader);
                }

                glDepthMask(GL_TRUE);
                glDepthFunc(GL_LEQUAL);
                glDisable(GL_BLEND);
            }

            instancedShader.use();
            instancedShader.setMat4("projection", projection);
            instancedShader.setMat4("view", view);
            instancedShader.setInt("texture_diffuse1", 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, box_textured.textures_loaded[0].id);
            for (unsigned int i = 0; i < box_textured.meshes.size(); i++) {
                glBindVertexArray(box_textured.meshes[i].VAO);
                glDrawElementsInstanced(GL_TRIANGLES, static_cast<unsigned int>(box_textured.meshes[i].indices.size()), GL_UNSIGNED_INT, 0, amount);
                glBindVertexArray(0);
            }

            glm::mat4 terrainMatrix = glm::mat4(1.0f);
            terrainMatrix = glm::translate(terrainMatrix, glm::vec3(0.0f, -20.0f, 0.0f));
            terrainMatrix = glm::scale(terrainMatrix, glm::vec3(50.0f, 50.0f, 50.0f));

            terrainShader.use();
            terrainShader.setMat4("projection", projection);
            terrainShader.setMat4("view", view);
            terrainShader.setMat4("model", terrainMatrix);
            terrainShader.setInt("heightMap", 0);
            terrainShader.setFloat("time", glfwGetTime());
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, heightmapTexture);
            glBindVertexArray(terrain.meshes[0].VAO);
            glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(terrain.meshes[0].indices.size()), GL_UNSIGNED_INT, 0);

            for (auto& entity : misc_entities) {
                const ddMat4x4 transform = {
                    entity->transform.modelMatrix[0][0],
                    entity->transform.modelMatrix[0][1],
                    entity->transform.modelMatrix[0][2],
                    entity->transform.modelMatrix[0][3],
                    entity->transform.modelMatrix[1][0],
                    entity->transform.modelMatrix[1][1],
                    entity->transform.modelMatrix[1][2],
                    entity->transform.modelMatrix[1][3],
                    entity->transform.modelMatrix[2][0],
                    entity->transform.modelMatrix[2][1],
                    entity->transform.modelMatrix[2][2],
                    entity->transform.modelMatrix[2][3],
                    entity->transform.modelMatrix[3][0],
                    entity->transform.modelMatrix[3][1],
                    entity->transform.modelMatrix[3][2],
                    entity->transform.modelMatrix[3][3],
                };
                dd::axisTriad(transform, 0.05f, 0.5f);
            }

            // render skybox (render as last to prevent overdraw)
            backgroundShader.use();
            backgroundShader.setMat4("view", view);
            backgroundShader.setMat4("projection", projection);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.getEnvCubemap());
            renderCube();

            // attach it to currently bound framebuffer object
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, postprocessTexture, 0);

            glDisable(GL_DEPTH_TEST);
            glClearColor(clear_color.x, clear_color.y, clear_color.z, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            postprocessShader.use();
            postprocessShader.setBool("u_fxaaOn", useFxaa);
            postprocessShader.setVec2("u_texelStep", 1.0f / viewportWidth, 1.0f / viewportHeight);
            postprocessShader.setBool("u_showEdges", fxaaDebugDraw);
            postprocessShader.setFloat("u_lumaThreshold", lumaThreshold);
            postprocessShader.setFloat("u_mulReduce", 1.0f / 8.0f);
            postprocessShader.setFloat("u_minReduce", 1.0f / 128.0f);
            postprocessShader.setFloat("u_maxSpan", 8.0f);
            glBindVertexArray(quadVAO);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, renderTexture);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            dd::flush();

            ImGui::Image((ImTextureID)postprocessTexture, content_size, ImVec2(0, 1), ImVec2(1, 0));

            ImGuiIO& io = ImGui::GetIO();
            ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, content_size.x, content_size.y);
            ImGuizmo::SetDrawlist();

            ImGuiWindow* imgui_window = ImGui::GetCurrentWindow();

            viewportWindowFlags = ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(imgui_window->InnerRect.Min, imgui_window->InnerRect.Max) ? ImGuiWindowFlags_NoMove : 0;

            if (ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(imgui_window->InnerRect.Min, imgui_window->InnerRect.Max)) {
                if (io.MouseClicked[1]) {
                    cameraMouseControl = true;
                    firstMouse = true;
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                }
            }
            if (io.MouseReleased[1]) {
                cameraMouseControl = false;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }

            if (last_selected) {
                glm::mat4 tempMatrix = last_selected->transform.modelMatrix;

                if (ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), currentGizmoOperation, currentGizmoMode, glm::value_ptr(tempMatrix), NULL, NULL)) {
                    // Gizmo handles our final world transform, but we need to update our local transform (pos, orient, scale)
                    // In order to do that, we extract the local transform from the world transform, by multiplying by the inverse of the parent's world transform
                    // From there, we can decompose the local transform into its components (pos, orient, scale)
                    glm::vec3 skew;
                    glm::vec4 perspective;
                    if (last_selected->parent)
                        glm::decompose(glm::inverse(last_selected->parent->transform.modelMatrix) * tempMatrix, last_selected->transform.scale, last_selected->transform.orient, last_selected->transform.pos, skew, perspective);
                    else
                        glm::decompose(tempMatrix, last_selected->transform.scale, last_selected->transform.orient, last_selected->transform.pos, skew, perspective);

                    last_selected->transform.isDirty = true;
                }
            }
        }
        ImGui::End();

        scene_root.updateSelfAndChildren();

        ImGui::PopStyleVar(1);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Rendering
        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glDeleteTextures(1, &renderTexture);
        glDeleteTextures(1, &postprocessTexture);
        glDeleteRenderbuffers(1, &rbo);

        glfwMakeContextCurrent(window);
        glfwSwapBuffers(window);
    }

    dd::shutdown();

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_T && action == GLFW_PRESS && !cameraMouseControl) {
        if (currentGizmoMode == ImGuizmo::LOCAL)
            currentGizmoMode = ImGuizmo::WORLD;
        else
            currentGizmoMode = ImGuizmo::LOCAL;
    }
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    ImGuiIO& io = ImGui::GetIO();
    if (!cameraMouseControl) {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            currentGizmoOperation = ImGuizmo::TRANSLATE;
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            currentGizmoOperation = ImGuizmo::ROTATE;
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
            currentGizmoOperation = ImGuizmo::SCALE;
    }

    if ((io.WantCaptureKeyboard || io.WantCaptureMouse) && !cameraMouseControl) {
        return;
    }

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        deltaTime *= 6.0f;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, deltaTime);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    if (!cameraMouseControl) {
        return;
    }

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    static int ignoreTimes = 2;

    if (firstMouse) {
        firstMouse = false;
        ignoreTimes = 2;
    }

    if (ignoreTimes > 0) {
        lastX = xpos;
        lastY = ypos;
        ignoreTimes--;
        return;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}
