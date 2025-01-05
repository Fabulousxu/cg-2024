#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/noise.hpp>

#include "shader.h"
#include "camera.h"
#include "model.h"

#include <iostream>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// 雪花粒子
struct SnowParticle {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 color;  // 雪花颜色，介于白色和浅蓝色之间
};

// 光粒子
struct LightParticle {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 color;
    float flashDelTime;
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void initSnowParticle(SnowParticle& particle);
void initLightParticle(LightParticle& particle);
glm::vec3 bezier2(const glm::vec3 &p0, const glm::vec3 &p1, const glm::vec3 &p2, float t);
glm::vec3 bezier3(const glm::vec3 &p0, const glm::vec3 &p1, const glm::vec3 &p2, const glm::vec3 &p3, float t);

// 窗口设置
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// 摄像机设置
Camera camera(glm::vec3(0.0f, 0.3f, 3.3f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// 时间设置
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// 光照设置
glm::vec3 lightPos(0.0f, 0.75f, 1.65f);
glm::vec3 cubePos(0.0f, 0.3f, 2.0f);
glm::vec3 areaLightPos(0.0f, 0.79f, 2.0f);
glm::vec3 areaLightColor(0.2f, 0.066f, 0.009f);

// 风车填充颜色
float windmillColor[] = {
    0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f,
};

bool windmillAppear = false; // 风车是否出现
bool windmillColorful = false; // 风车是否填充颜色
bool windmillRotate = false; // 风车是否旋转
float windmillSpeed = 90.0f; // 风车旋转速度
float windmillAngle = 0.0f; // 风车当前角度
bool snowAppear = false;     // 是否有雪花效果
unsigned int snowParticleCount = 400; // 雪花粒子数量
bool isLightOn = false; // 圣诞树是否亮灯
unsigned int lightParticleCount = 100; // 光粒子数量
bool blackboardDisplay = false; // 是否显示黑板
bool tableDisplay = false; // 是否显示桌子

// 管道材质设置
float pipeMetallic = 0.9;
float pipeRoughness = 0.1;
float pipeSpecular = 0.8;
float pipeColorR = 0.72;
float pipeColorG = 0.58;
float pipeColorB = 0.09;
float pipeColorA = 1.0;
int pipeMaterialSelect = 1;

int main()
{
    // 初始化并配置glfw
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw创建窗口
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    // 告诉 GLFW 捕获我们的鼠标
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad：加载所有 OpenGL 函数指针
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // 配置全局 OpenGL 状态
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_POINT_SPRITE);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 编译shader操作
    // ------------------------------------
    Shader lightingShader("shaders/lighting.vs.glsl", "shaders/lighting.fs.glsl");
    Shader lightCubeShader("shaders/lightcube.vs.glsl", "shaders/lightcube.fs.glsl");
    Shader christmasTreeShader("shaders/christmas_tree.vs.glsl", "shaders/christmas_tree.fs.glsl");
    Shader terrainShader("shaders/terrain.vert.glsl", "shaders/terrain.frag.glsl", "shaders/terrain.tesc.glsl", "shaders/terrain.tese.glsl", "shaders/terrain.gs.glsl");
    Shader snowShader("shaders/snow.vs.glsl", "shaders/snow.fs.glsl");
    Shader lightPointShader("shaders/lightpoint.vs.glsl", "shaders/lightpoint.fs.glsl");
    Shader areaLightCubeShader("shaders/arealightcube.vs.glsl", "shaders/arealightcube.fs.glsl");
    Shader areaLightingShader("shaders/arealighting.vs.glsl", "shaders/arealighting.fs.glsl");

    Model christmasTreeModel("models/obj/christmas_tree/christmas_tree.obj");
    Model tableModel("models/obj/table/table.obj");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    int inner = 1;
    int outer = 1;

    // 统一设置用到的坐标信息(每一行前三个数字为点的坐标，后三个为法向量)
    // ------------------------------------------------------------------
    float vertices[] = {
        // 立方体顶点坐标
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f,  1.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, -1.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, -1.0f,

        -0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

         0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
         0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  1.0f,  0.0f,

        -0.5f,  0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

        // 风车顶点坐标
          0.0f,   0.0f,   0.5f,   0.0f,   0.0f,   1.0f,
          0.5f,   0.0f,   0.5f,   0.0f,   0.0f,   1.0f,
          0.5f,   0.5f,   0.5f,   0.0f,   0.0f,   1.0f,

          0.0f,   0.0f,   0.5f,   0.0f,   0.0f,   1.0f,
         0.25f,  0.25f,   0.5f,   0.0f,   0.0f,   1.0f,
          0.0f,   0.5f,   0.5f,   0.0f,   0.0f,   1.0f,
    };

    //获取各个平面的数据
    // ------------------------------------------------------------------
    float CeilingVertices[36];
    std::copy(vertices + 180, vertices + 216, CeilingVertices);
    float FloorVertices[36];
    std::copy(vertices + 144, vertices + 180, FloorVertices);
    float LWallVertices[36];
    std::copy(vertices + 72, vertices + 108, LWallVertices);
    float RWallVertices[36];
    std::copy(vertices + 108, vertices + 144, RWallVertices);
    float FWallVertices[36];
    std::copy(vertices + 0, vertices + 36, FWallVertices);
    float BWallVertices[36];
    std::copy(vertices + 36, vertices + 72, BWallVertices);
    float Windmill1Vertices[18];
    std::copy(vertices + 216, vertices + 234, Windmill1Vertices);
    float Windmill2Vertices[18];
    std::copy(vertices + 234, vertices + 252, Windmill2Vertices);


    // 载入天花板的顶点信息
    // ------------------------------------------------------------------
    unsigned int VBO1, CeilingVAO;
    {
        glGenVertexArrays(1, &CeilingVAO);
        glGenBuffers(1, &VBO1);

        glBindBuffer(GL_ARRAY_BUFFER, VBO1);
        glBufferData(GL_ARRAY_BUFFER, sizeof(CeilingVertices), CeilingVertices, GL_STATIC_DRAW);

        glBindVertexArray(CeilingVAO);

        // 载入位置
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(0 * sizeof(float)));
        glEnableVertexAttribArray(0);
        // 载入法向量
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    //载入地板的顶点信息
    // ------------------------------------------------------------------
    unsigned int VBO2, FloorVAO;
    {
        glGenVertexArrays(1, &FloorVAO);
        glGenBuffers(1, &VBO2);

        glBindBuffer(GL_ARRAY_BUFFER, VBO2);
        glBufferData(GL_ARRAY_BUFFER, sizeof(FloorVertices), FloorVertices, GL_STATIC_DRAW);

        glBindVertexArray(FloorVAO);

        // 载入位置
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(0 * sizeof(float)));
        glEnableVertexAttribArray(0);
        // 载入法向量
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    //载入左墙的顶点信息
    // ------------------------------------------------------------------
    unsigned int VBO3, LWallVAO;
    {
        glGenVertexArrays(1, &LWallVAO);
        glGenBuffers(1, &VBO3);

        glBindBuffer(GL_ARRAY_BUFFER, VBO3);
        glBufferData(GL_ARRAY_BUFFER, sizeof(LWallVertices), LWallVertices, GL_STATIC_DRAW);

        glBindVertexArray(LWallVAO);

        // 载入位置
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(0 * sizeof(float)));
        glEnableVertexAttribArray(0);
        // 载入法向量
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    //载入右墙的顶点信息
    // ------------------------------------------------------------------
    unsigned int VBO4, RWallVAO;
    {
        glGenVertexArrays(1, &RWallVAO);
        glGenBuffers(1, &VBO4);

        glBindBuffer(GL_ARRAY_BUFFER, VBO4);
        glBufferData(GL_ARRAY_BUFFER, sizeof(RWallVertices), RWallVertices, GL_STATIC_DRAW);

        glBindVertexArray(RWallVAO);

        // 载入位置
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(0 * sizeof(float)));
        glEnableVertexAttribArray(0);
        // 载入法向量
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    //载入前墙的顶点信息
    // ------------------------------------------------------------------
    unsigned int VBO5, FWallVAO;
    {
        glGenVertexArrays(1, &FWallVAO);
        glGenBuffers(1, &VBO5);

        glBindBuffer(GL_ARRAY_BUFFER, VBO5);
        glBufferData(GL_ARRAY_BUFFER, sizeof(FWallVertices), FWallVertices, GL_STATIC_DRAW);

        glBindVertexArray(FWallVAO);

        // 载入位置
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(0 * sizeof(float)));
        glEnableVertexAttribArray(0);
        // 载入法向量
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    // 载入方块灯的顶点信息
    unsigned int VBO6, lightCubeVAO;
    {
        glGenVertexArrays(1, &lightCubeVAO);
        glGenBuffers(1, &VBO6);

        glBindBuffer(GL_ARRAY_BUFFER, VBO6);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindVertexArray(lightCubeVAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO6);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
    }

    //载入后墙的顶点信息
    //  ------------------------------------------------------------------
    unsigned int VBO7, BWallVAO;
    {
        glGenVertexArrays(1, &BWallVAO);
        glGenBuffers(1, &VBO7);

        glBindBuffer(GL_ARRAY_BUFFER, VBO7);
        glBufferData(GL_ARRAY_BUFFER, sizeof(BWallVertices), BWallVertices, GL_STATIC_DRAW);

        glBindVertexArray(BWallVAO);

        // 载入位置
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(0 * sizeof(float)));
        glEnableVertexAttribArray(0);
        // 载入法向量
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    // 载入风车面1的顶点信息
    unsigned int VBO8, Windmill1VAO;
    {
        glGenVertexArrays(1, &Windmill1VAO);
        glGenBuffers(1, &VBO8);

        glBindBuffer(GL_ARRAY_BUFFER, VBO8);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Windmill1Vertices), Windmill1Vertices, GL_DYNAMIC_DRAW);

        glBindVertexArray(Windmill1VAO);

        // 载入位置
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(0 * sizeof(float)));
        glEnableVertexAttribArray(0);
        // 载入法向量
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    // 载入风车面2的顶点信息
    unsigned int VBO9, Windmill2VAO;
    {
        glGenVertexArrays(1, &Windmill2VAO);
        glGenBuffers(1, &VBO9);

        glBindBuffer(GL_ARRAY_BUFFER, VBO9);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Windmill2Vertices), Windmill2Vertices, GL_DYNAMIC_DRAW);

        glBindVertexArray(Windmill2VAO);

        // 载入位置
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(0 * sizeof(float)));
        glEnableVertexAttribArray(0);
        // 载入法向量
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    std::vector<float> terrainVertices;
    std::vector<unsigned int> terrainIndices;
    const unsigned int terrainWidth = 64;

    // 生成原始地形顶点信息，并映射成圆
    for (int z = 0; z < terrainWidth; ++z) {
        for (int x = 0; x < terrainWidth; ++x) {
            float x_r = static_cast<float>(x) / terrainWidth * 2 - 1.0f;
            float z_r = static_cast<float>(z) / terrainWidth * 2 - 1.0f;

            float x_c = x_r * glm::sqrt(1 - z_r * z_r / 2.0f) * 0.5f;
            float z_c = z_r * glm::sqrt(1 - x_r * x_r / 2.0f) * 0.5f;

            float y = glm::perlin(glm::vec2(x * 0.1f, z * 0.1f)) / 2 + 0.5f;

            terrainVertices.push_back(x_c);
            terrainVertices.push_back(y);
            terrainVertices.push_back(z_c);
        }
    }
    
    // 生成地形索引
    for (int z = 0; z < terrainWidth - 1; ++z) {
        for (int x = 0; x < terrainWidth - 1; ++x) {
            unsigned int topLeft = z * terrainWidth + x;
            unsigned int topRight = topLeft + 1;
            unsigned int bottomLeft = topLeft + terrainWidth;
            unsigned int bottomRight = bottomLeft + 1;

            terrainIndices.push_back(topLeft);
            terrainIndices.push_back(bottomLeft);
            terrainIndices.push_back(topRight);

            terrainIndices.push_back(topRight);
            terrainIndices.push_back(bottomLeft);
            terrainIndices.push_back(bottomRight);
        }
    }

    // 平滑滤波地形图
    {
        std::vector<float> tmpVertices(terrainVertices.size() / 3, 0.0f);

        for (int z = 0; z < terrainWidth; ++z) {
            for (int x = 0; x < terrainWidth; ++x) {
                float sum = 0.0f;
                int count = 0;

                for (int dz = -1; dz <= 1; ++dz) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        int nx = x + dx;
                        int nz = z + dz;

                        if (nx >= 0 && nx < terrainWidth && nz >= 0 && nz < terrainWidth) {
                            int neighborIndex = nz * terrainWidth + nx;
                            sum += terrainVertices[neighborIndex * 3 + 1];
                            ++count;
                        }
                    }
                }

                tmpVertices[z * terrainWidth + x] = sum / count;
            }
        }

        for (size_t i = 0; i < tmpVertices.size(); ++i)
            terrainVertices[i * 3 + 1] = tmpVertices[i];
    }

    // 载入地形顶点信息
    unsigned int VBO10, terrainVAO, terrainEBO;
    {
        glGenVertexArrays(1, &terrainVAO);
        glGenBuffers(1, &VBO10);
        glGenBuffers(1, &terrainEBO);
        glBindVertexArray(terrainVAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO10);
        glBufferData(GL_ARRAY_BUFFER, terrainVertices.size() * sizeof(float), terrainVertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrainEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, terrainIndices.size() * sizeof(unsigned int), terrainIndices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)(0 * sizeof(float)));
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    
    std::vector<float> platformVertices;
    std::vector<unsigned int> platformIndices;

    // 生成平台侧面顶点
    for (int x = 0, z = 0; x < terrainWidth; x++) {
        platformVertices.push_back(terrainVertices[(z * terrainWidth + x) * 3]);
        platformVertices.push_back(-0.5f);
        platformVertices.push_back(terrainVertices[(z * terrainWidth + x) * 3 + 2]);

        platformVertices.push_back(terrainVertices[(z * terrainWidth + x) * 3]);
        platformVertices.push_back(0.0f);
        platformVertices.push_back(terrainVertices[(z * terrainWidth + x) * 3 + 2]);

        platformVertices.push_back(terrainVertices[(z * terrainWidth + x) * 3]);
        platformVertices.push_back(terrainVertices[(z * terrainWidth + x) * 3 + 1]);
        platformVertices.push_back(terrainVertices[(z * terrainWidth + x) * 3 + 2]);

        platformVertices.push_back(terrainVertices[(z * terrainWidth + x) * 3]);
        platformVertices.push_back(0.0f);
        platformVertices.push_back(terrainVertices[(z * terrainWidth + x) * 3 + 2]);
    }

    for (int x = terrainWidth - 1, z = 0; z < terrainWidth; z++) {
        platformVertices.push_back(terrainVertices[(z * terrainWidth + x) * 3]);
        platformVertices.push_back(-0.5f);
        platformVertices.push_back(terrainVertices[(z * terrainWidth + x) * 3 + 2]);

        platformVertices.push_back(terrainVertices[(z * terrainWidth + x) * 3]);
        platformVertices.push_back(0.0f);
        platformVertices.push_back(terrainVertices[(z * terrainWidth + x) * 3 + 2]);

        platformVertices.push_back(terrainVertices[(z * terrainWidth + x) * 3]);
        platformVertices.push_back(terrainVertices[(z * terrainWidth + x) * 3 + 1]);
        platformVertices.push_back(terrainVertices[(z * terrainWidth + x) * 3 + 2]);

        platformVertices.push_back(terrainVertices[(z * terrainWidth + x) * 3]);
        platformVertices.push_back(0.0f);
        platformVertices.push_back(terrainVertices[(z * terrainWidth + x) * 3 + 2]);
    }

    for (int x = terrainWidth - 1, z = terrainWidth - 1; x > 0; x--) {
        platformVertices.push_back(terrainVertices[(z * terrainWidth + x) * 3]);
        platformVertices.push_back(-0.5f);
        platformVertices.push_back(terrainVertices[(z * terrainWidth + x) * 3 + 2]);

        platformVertices.push_back(terrainVertices[(z * terrainWidth + x) * 3]);
        platformVertices.push_back(0.0f);
        platformVertices.push_back(terrainVertices[(z * terrainWidth + x) * 3 + 2]);

        platformVertices.push_back(terrainVertices[(z * terrainWidth + x) * 3]);
        platformVertices.push_back(terrainVertices[(z * terrainWidth + x) * 3 + 1]);
        platformVertices.push_back(terrainVertices[(z * terrainWidth + x) * 3 + 2]);

        platformVertices.push_back(terrainVertices[(z * terrainWidth + x) * 3]);
        platformVertices.push_back(0.0f);
        platformVertices.push_back(terrainVertices[(z * terrainWidth + x) * 3 + 2]);
    }

    for (int x = 0, z = terrainWidth - 1; z > 0; z--) {
        platformVertices.push_back(terrainVertices[(z * terrainWidth + x) * 3]);
        platformVertices.push_back(-0.5f);
        platformVertices.push_back(terrainVertices[(z * terrainWidth + x) * 3 + 2]);

        platformVertices.push_back(terrainVertices[(z * terrainWidth + x) * 3]);
        platformVertices.push_back(0.0f);
        platformVertices.push_back(terrainVertices[(z * terrainWidth + x) * 3 + 2]);

        platformVertices.push_back(terrainVertices[(z * terrainWidth + x) * 3]);
        platformVertices.push_back(terrainVertices[(z * terrainWidth + x) * 3 + 1]);
        platformVertices.push_back(terrainVertices[(z * terrainWidth + x) * 3 + 2]);

        platformVertices.push_back(terrainVertices[(z * terrainWidth + x) * 3]);
        platformVertices.push_back(0.0f);
        platformVertices.push_back(terrainVertices[(z * terrainWidth + x) * 3 + 2]);
    }

    // 生成平台索引
    for (int i = 0; i < platformVertices.size() / 12; ++i) {
        platformIndices.push_back(i * 2);
        platformIndices.push_back((i * 2 + 1) % (platformVertices.size() / 6));
        platformIndices.push_back((i * 2 + 3) % (platformVertices.size() / 6));

        platformIndices.push_back(i * 2);
        platformIndices.push_back((i * 2 + 3) % (platformVertices.size() / 6));
        platformIndices.push_back((i * 2 + 2) % (platformVertices.size() / 6));
    }
    
    // 载入平台顶点信息
    unsigned int VBO11, platformVAO, platformEBO;
    {
        glGenVertexArrays(1, &platformVAO);
        glGenBuffers(1, &VBO11);
        glGenBuffers(1, &platformEBO);
        glBindVertexArray(platformVAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO11);
        glBufferData(GL_ARRAY_BUFFER, platformVertices.size() * sizeof(float), platformVertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, platformEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, platformIndices.size() * sizeof(unsigned int), platformIndices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(0 * sizeof(float)));
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }


    // 雪花粒子
    std::vector<SnowParticle> snowParticles;

    // 初始化雪花粒子
    for (unsigned int i = 0; i < snowParticleCount; ++i) {
        SnowParticle particle;
        initSnowParticle(particle);
        snowParticles.push_back(particle);
    }

    unsigned int VBO12, snowVAO;
    {
        glGenVertexArrays(1, &snowVAO);
        glGenBuffers(1, &VBO12);

        glBindBuffer(GL_ARRAY_BUFFER, VBO12);
        glBufferData(GL_ARRAY_BUFFER, snowParticles.size() * sizeof(SnowParticle), snowParticles.data(), GL_DYNAMIC_DRAW);

        glBindVertexArray(snowVAO);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SnowParticle), (void*)offsetof(SnowParticle, position));
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(SnowParticle), (void*)offsetof(SnowParticle, color));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    // 显示圣诞树包围盒
    float boxUpCenter = 1.18f;
    float boxDownCenter = 0.12f;
    float boxRadius = 0.40f;

    float christmasTreeBoxVertices[] = {
        -boxRadius, boxDownCenter, 0.0f, 1.0f, 1.0f, 1.0f,
        0.0f, boxUpCenter, 0.0f, 1.0f, 1.0f, 1.0f,
        boxRadius, boxDownCenter, 0.0f, 1.0f, 1.0f, 1.0f,
        0.0f, boxDownCenter, -boxRadius, 1.0f, 1.0f, 1.0f,
        0.0f, boxUpCenter, 0.0f, 1.0f, 1.0f, 1.0f,
        0.0f, boxDownCenter, boxRadius, 1.0f, 1.0f, 1.0f
    };

    unsigned int VBO13, christmaxTreeBoxVAO;
    {
        glGenVertexArrays(1, &christmaxTreeBoxVAO);
        glGenBuffers(1, &VBO13);

        glBindBuffer(GL_ARRAY_BUFFER, VBO13);
        glBufferData(GL_ARRAY_BUFFER, sizeof(christmasTreeBoxVertices), christmasTreeBoxVertices, GL_STATIC_DRAW);

        glBindVertexArray(christmaxTreeBoxVAO);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(0 * sizeof(float)));
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }


    // 光粒子
    std::vector<LightParticle> lightParticles;

    // 初始化光粒子
    for (unsigned int i = 0; i < lightParticleCount; ++i) {
        LightParticle particle;
        initLightParticle(particle);
        lightParticles.push_back(particle);
    }

    unsigned int VBO14, lightPointVAO;
    {
        glGenVertexArrays(1, &lightPointVAO);
        glGenBuffers(1, &VBO14);

        glBindBuffer(GL_ARRAY_BUFFER, VBO14);
        glBufferData(GL_ARRAY_BUFFER, lightParticles.size() * sizeof(LightParticle), lightParticles.data(), GL_DYNAMIC_DRAW);

        glBindVertexArray(lightPointVAO);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(LightParticle), (void*)offsetof(LightParticle, position));
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(LightParticle), (void*)offsetof(LightParticle, color));
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(LightParticle), (void*)offsetof(LightParticle, flashDelTime));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    // 加载光粒子材质
    unsigned int glowTexture;
    {
        glGenTextures(1, &glowTexture);
        glBindTexture(GL_TEXTURE_2D, glowTexture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        int width, height, nrChannels;
        unsigned char *data = stbi_load("textures/glow.png", &width, &height, &nrChannels, 4);
        if (data) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
        } else {
            std::cout << "Failed to load texture" << std::endl;
        }
        stbi_image_free(data);
    }

    // 加载面光源
    std::vector<glm::vec3> areaLightPosArray;
    const int lightNum = 10;
    for (int i = 0; i < lightNum; i++) {
        for (int j = 0; j < lightNum; j++) {
             glm::vec3 offset = glm::vec3(i, 0, j);
             offset *= 0.30f / lightNum;
             offset -= glm::vec3(0.15f, 0, 0.15f);
             areaLightPosArray.push_back(areaLightPos + offset);
        }
    }

    // 管道
    glm::vec3 controlPoints[] = {
        {-0.5f,  0.0f,  0.0f},
        {-0.2f, -1.0f, -1.0f},
        { 0.2f,  1.0f,  1.0f},
        { 0.5f,  0.0f,  0.0f}
    }; // 贝塞尔曲线控制点
    std::vector<float> pipeVertices;
    std::vector<unsigned int> pipeIndices;
    std::vector<float> keySectionVertices[3]; // 关键截面顶点
    const int sampleNum = 256; // 采样点数
    const int segmentNum = 256; // 管道段数
    std::vector<glm::vec3> keySections[3];  // 关键
    std::vector<std::vector<glm::vec3>> sections(segmentNum);

    std::vector<glm::vec3> &rectangleSection = keySections[0]; // 正方形截面
    for (int i = 0; i < sampleNum; i++) {
        if (i < sampleNum / 8) {
            float y = i / float(sampleNum / 4);
            rectangleSection.push_back(glm::vec3(-0.5f, y, 0.5f));
        } else if (i < 3 * sampleNum / 8) {
            float z = 0.5f - (i - sampleNum / 8) / float(sampleNum / 4);
            rectangleSection.push_back(glm::vec3(-0.5f, 0.5f, z));
        } else if (i < 5 * sampleNum / 8) {
            float y = 0.5f - (i - 3 * sampleNum / 8) / float(sampleNum / 4);
            rectangleSection.push_back(glm::vec3(-0.5f, y, -0.5f));
        } else if (i < 7 * sampleNum / 8) {
            float z = (i - 5 * sampleNum / 8) / float(sampleNum / 4) - 0.5f;
            rectangleSection.push_back(glm::vec3(-0.5f, -0.5f, z));
        } else {
            float y = (i - 7 * sampleNum / 8) / float(sampleNum / 4) - 0.5f;
            rectangleSection.push_back(glm::vec3(-0.5f, y, 0.5f));
        }
    }

    std::vector<glm::vec3> &ellipseSection = keySections[1];  // 椭圆截面
    for (int i = 0; i < sampleNum; i++) {
        float angle = glm::radians(360.0f / sampleNum * i);
        ellipseSection.push_back(glm::vec3(0.0f, glm::sin(angle) * 0.4f, glm::cos(angle) * 0.7f));
    }

    std::vector<glm::vec3> &circleSection = keySections[2];  // 圆形截面
    for (int i = 0; i < sampleNum; i++) {
        float angle = glm::radians(360.0f / sampleNum * i);
        circleSection.push_back(glm::vec3(0.5f, glm::sin(angle) * 0.6f, glm::cos(angle) * 0.6f));
    }

    // 管道插值（二次贝塞尔插值）
    for (int i = 0; i < sampleNum; i++) {
        glm::vec3 controlPoints1[] = {
            keySections[0][i],
            glm::vec3((keySections[0][i].x + keySections[1][i].x) / 2.0f,
                      keySections[1][i].y, keySections[1][i].z),
            keySections[1][i]
        };
        glm::vec3 controlPoints2[] = {
            keySections[1][i],
            glm::vec3((keySections[1][i].x + keySections[2][i].x) / 2.0f,
                      keySections[1][i].y, keySections[1][i].z),
            keySections[2][i]
        };
        int j = 0;
        for (; j < segmentNum / 2; j++) {
            float t = j / float(segmentNum / 2);
            sections[j].push_back(bezier2(controlPoints1[0], controlPoints1[1], controlPoints1[2], t));
        }
        for (; j < segmentNum; j++) {
            float t = (j - segmentNum / 2) / float(segmentNum / 2);
            sections[j].push_back(bezier2(controlPoints2[0], controlPoints2[1], controlPoints2[2], t));
        }
    }

    // 塞尔曲线偏移管道
    for (int i = 0; i < sections.size(); i++) {
        float t = i / float(sections.size());
        glm::vec3 offset = bezier3(controlPoints[0], controlPoints[1], controlPoints[2], controlPoints[3], t);
        for (int j = 0; j < sampleNum; j++) {
            sections[i][j] += offset;
        }       
    }

    // 生成管道顶点
    for (int i = 0; i < sections.size() - 1; i++) {
        for (int j = 0; j < sampleNum; j++) {
            pipeVertices.push_back(sections[i][j].x);
            pipeVertices.push_back(sections[i][j].y);
            pipeVertices.push_back(sections[i][j].z);

            glm::vec3 normal = glm::cross(sections[i + 1][j] - sections[i][j], sections[i][(j + 1) % sampleNum] - sections[i][j]);
            pipeVertices.push_back(normal.x);
            pipeVertices.push_back(normal.y);
            pipeVertices.push_back(normal.z);

            pipeVertices.push_back(sections[i + 1][j].x);
            pipeVertices.push_back(sections[i + 1][j].y);
            pipeVertices.push_back(sections[i + 1][j].z);

            pipeVertices.push_back(normal.x);
            pipeVertices.push_back(normal.y);
            pipeVertices.push_back(normal.z);
        }
    }

    for (int i = 0; i < sections.size() - 1; i++) {
        for (int j = 0; j < sampleNum; j++) {
            int base1 = i * sampleNum * 2 + j * 2;
            int base2 = i * sampleNum * 2 + (j + 1) * 2 % (sampleNum * 2);
            pipeIndices.push_back(base1);
            pipeIndices.push_back(base1 + 1);
            pipeIndices.push_back(base2);
            pipeIndices.push_back(base1 + 1);
            pipeIndices.push_back(base2);
            pipeIndices.push_back(base2 + 1);
        }
    }

    // 生成关键截面顶点
    for (int j = 0; j < sampleNum; j++) {
        keySectionVertices[0].push_back(sections[0][j].x);
        keySectionVertices[0].push_back(sections[0][j].y);
        keySectionVertices[0].push_back(sections[0][j].z);
    }
    for (int j = 0; j < sampleNum; j++) {
        keySectionVertices[1].push_back(sections[sections.size() / 2][j].x);
        keySectionVertices[1].push_back(sections[sections.size() / 2][j].y);
        keySectionVertices[1].push_back(sections[sections.size() / 2][j].z);
    }
    for (int j = 0; j < sampleNum; j++) {
        keySectionVertices[2].push_back(sections[sections.size() - 1][j].x);
        keySectionVertices[2].push_back(sections[sections.size() - 1][j].y);
        keySectionVertices[2].push_back(sections[sections.size() - 1][j].z);
    }

    // 载入管道顶点信息
    unsigned int VBO15, pipeVAO, pipeEBO;
    {
        glGenVertexArrays(1, &pipeVAO);
        glGenBuffers(1, &VBO15);
        glGenBuffers(1, &pipeEBO);
        glBindVertexArray(pipeVAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO15);
        glBufferData(GL_ARRAY_BUFFER, pipeVertices.size() * sizeof(float), pipeVertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pipeEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, pipeIndices.size() * sizeof(unsigned int), pipeIndices.data(), GL_STATIC_DRAW);
    
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(0 * sizeof(float)));
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    // 载入关键截面顶点信息
    unsigned int VBO16, keySection1VAO;
    {
        glGenVertexArrays(1, &keySection1VAO);
        glGenBuffers(1, &VBO16);

        glBindBuffer(GL_ARRAY_BUFFER, VBO16);
        glBufferData(GL_ARRAY_BUFFER, keySectionVertices[0].size() * sizeof(float), keySectionVertices[0].data(), GL_STATIC_DRAW);

        glBindVertexArray(keySection1VAO);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)(0 * sizeof(float)));
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    unsigned int VBO17, keySection2VAO;
    {
        glGenVertexArrays(1, &keySection2VAO);
        glGenBuffers(1, &VBO17);

        glBindBuffer(GL_ARRAY_BUFFER, VBO17);
        glBufferData(GL_ARRAY_BUFFER, keySectionVertices[1].size() * sizeof(float), keySectionVertices[1].data(), GL_STATIC_DRAW);

        glBindVertexArray(keySection2VAO);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)(0 * sizeof(float)));
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    unsigned int VBO18, keySection3VAO;
    {
        glGenVertexArrays(1, &keySection3VAO);
        glGenBuffers(1, &VBO18);

        glBindBuffer(GL_ARRAY_BUFFER, VBO18);
        glBufferData(GL_ARRAY_BUFFER, keySectionVertices[2].size() * sizeof(float), keySectionVertices[2].data(), GL_STATIC_DRAW);

        glBindVertexArray(keySection3VAO);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)(0 * sizeof(float)));
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }


    // 初始化随机数种子
    srand(static_cast<unsigned int>(glfwGetTime() * 1000));

    // 渲染循环
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // 时间逻辑
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // 输入
        // -----
        processInput(window);

        // 处理风车旋转
        if (windmillRotate) {
            windmillAngle += windmillSpeed * deltaTime;
            windmillAngle = windmillAngle - static_cast<int>(windmillAngle) + static_cast<int>(windmillAngle) % 360;
        }

        // 更新雪花粒子
        if (snowAppear) {
            for (auto& particle : snowParticles) {
                particle.position += particle.velocity * deltaTime;

                float x = particle.position.x;
                float y = particle.position.y;
                float z = particle.position.z;

                auto tmp = (boxUpCenter - y) / (boxUpCenter - boxDownCenter) * boxRadius;

                // 圣诞树包围盒碰撞检测
                if (y < 0.0f || ((y <= boxUpCenter || y >= boxDownCenter) && x * x + z * z < tmp * tmp)) {
                    initSnowParticle(particle);
                }
            }
            glBindBuffer(GL_ARRAY_BUFFER, VBO12);
            glBufferSubData(GL_ARRAY_BUFFER, 0, snowParticles.size() * sizeof(SnowParticle), snowParticles.data());
        }

        // 更新光粒子
        if (isLightOn) {
            for (auto& particle : lightParticles) {
                particle.position += particle.velocity * deltaTime;

                float x = particle.position.x;
                float y = particle.position.y;
                float z = particle.position.z;               

                if (x * x + z * z > 0.48f * 0.48f) {
                    particle.velocity.x *= -1;
                    particle.velocity.z *= -1;
                }

                if (y < 0.05f || y > 1.12f) {
                    particle.velocity.y *= -1;
                }

            }
            glBindBuffer(GL_ARRAY_BUFFER, VBO14);
            glBufferSubData(GL_ARRAY_BUFFER, 0, lightParticles.size() * sizeof(LightParticle), lightParticles.data());
        }

        // 开始渲染
        // ------
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 确保在设置 Uniforms/Drawing 对象时激活 Shader
        //---------------------------------------------------------------------
        lightingShader.use();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 model = glm::mat4(1.0f);

        areaLightingShader.use();
        //绘制天花板
        {
            //设置光照参数
            areaLightingShader.setVec3("viewPos", camera.Position);
            areaLightingShader.setVec3Array("lightPos", areaLightPosArray);
            areaLightingShader.setInt("lightNum", areaLightPosArray.size());
            areaLightingShader.setVec3("lightColor", areaLightColor);
            areaLightingShader.setVec4("albedo", 0.6f, 0.6f, 0.6f, 1.0f);
            areaLightingShader.setFloat("metallic", 0.8f);
            areaLightingShader.setFloat("roughness", 0.2f);
            areaLightingShader.setFloat("specular", 1.0f);

            // view/projection 变换
            areaLightingShader.setMat4("projection", projection);
            areaLightingShader.setMat4("view", view);

            // 世界坐标变换
            model = glm::translate(model, cubePos);
            model = glm::scale(model, glm::vec3(1.0f));
            areaLightingShader.setMat4("model", model);

            // 渲染
            glBindVertexArray(CeilingVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // 绘制地板
        {
            //设置光照参数
            areaLightingShader.setVec3("viewPos", camera.Position);
            areaLightingShader.setVec3Array("lightPos", areaLightPosArray);
            areaLightingShader.setInt("lightNum", areaLightPosArray.size());
            areaLightingShader.setVec3("lightColor", areaLightColor);
            areaLightingShader.setVec4("albedo", 0.1f, 0.1f, 0.1f, 1.0f);
            areaLightingShader.setFloat("metallic", 0.8f);
            areaLightingShader.setFloat("roughness", 0.1f);
            areaLightingShader.setFloat("specular", 1.0f);

            // view/projection 变换
            areaLightingShader.setMat4("projection", projection);
            areaLightingShader.setMat4("view", view);

            // 世界坐标变换
            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos);
            model = glm::scale(model, glm::vec3(1.0f));
            areaLightingShader.setMat4("model", model);

            // 渲染
            glBindVertexArray(FloorVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // 绘制左墙
        {
            //设置光照参数
            areaLightingShader.setVec3("viewPos", camera.Position);
            areaLightingShader.setVec3Array("lightPos", areaLightPosArray);
            areaLightingShader.setInt("lightNum", areaLightPosArray.size());
            areaLightingShader.setVec3("lightColor", areaLightColor);
            areaLightingShader.setVec4("albedo", 0.45f, 0.45f, 0.45f, 1.0f);
            areaLightingShader.setFloat("metallic", 0.8f);
            areaLightingShader.setFloat("roughness", 0.2f);
            areaLightingShader.setFloat("specular", 1.0f);

            // view/projection 变换
            areaLightingShader.setMat4("projection", projection);
            areaLightingShader.setMat4("view", view);

            // 世界坐标变换
            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos);
            model = glm::scale(model, glm::vec3(1.0f));
            areaLightingShader.setMat4("model", model);

            // 渲染
            glBindVertexArray(LWallVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // 绘制右墙
        {
            //设置光照参数
            areaLightingShader.setVec3("viewPos", camera.Position);
            areaLightingShader.setVec3Array("lightPos", areaLightPosArray);
            areaLightingShader.setInt("lightNum", areaLightPosArray.size());
            areaLightingShader.setVec3("lightColor", areaLightColor);
            areaLightingShader.setVec4("albedo", 0.45f, 0.45f, 0.45f, 1.0f);
            areaLightingShader.setFloat("metallic", 0.8f);
            areaLightingShader.setFloat("roughness", 0.2f);
            areaLightingShader.setFloat("specular", 1.0f);

            // view/projection 变换
            areaLightingShader.setMat4("projection", projection);
            areaLightingShader.setMat4("view", view);

            // 世界坐标变换
            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos);
            model = glm::scale(model, glm::vec3(1.0f));
            areaLightingShader.setMat4("model", model);

            // 渲染
            glBindVertexArray(RWallVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // 绘制前墙
        {
            //设置光照参数
            areaLightingShader.setVec3("viewPos", camera.Position);
            areaLightingShader.setVec3Array("lightPos", areaLightPosArray);
            areaLightingShader.setInt("lightNum", areaLightPosArray.size());
            areaLightingShader.setVec3("lightColor", areaLightColor);
            areaLightingShader.setVec4("albedo", 0.35f, 0.35f, 0.35f, 1.0f);
            areaLightingShader.setFloat("metallic", 0.8f);
            areaLightingShader.setFloat("roughness", 0.2f);
            areaLightingShader.setFloat("specular", 1.0f);

            // view/projection 变换
            areaLightingShader.setMat4("projection", projection);
            areaLightingShader.setMat4("view", view);

            // 世界坐标变换
            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos);
            model = glm::scale(model, glm::vec3(1.0f));
            areaLightingShader.setMat4("model", model);

            // 渲染
            glBindVertexArray(FWallVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        
        // 绘制黑板
        if (blackboardDisplay) {
        // 绘制黑板正面边框部分
        {
            lightingShader.setVec3("objectColor", 0.75f, 0.5f, 0.3f);
            lightingShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setVec3("lightPos", lightPos);
            lightingShader.setVec3("viewPos", camera.Position);

            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);

            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos + glm::vec3(0.0f, 0.08f, -0.49f));
            model = glm::scale(model, glm::vec3(0.6f, 0.4f, 0.02f));
            lightingShader.setMat4("model", model);

            glBindVertexArray(BWallVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // 绘制黑板正面黑板部分
        {
            lightingShader.setVec3("objectColor", 0.04f, 0.04f, 0.04f);
            lightingShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setVec3("lightPos", lightPos);
            lightingShader.setVec3("viewPos", camera.Position);

            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);

            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos + glm::vec3(0.0f, 0.08f, -0.4899f));
            model = glm::scale(model, glm::vec3(0.55f, 0.35f, 0.02f));
            lightingShader.setMat4("model", model);

            glBindVertexArray(BWallVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // 绘制黑板背面
        {
            lightingShader.setVec3("objectColor", 0.75f, 0.5f, 0.3f);
            lightingShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setVec3("lightPos", lightPos);
            lightingShader.setVec3("viewPos", camera.Position);

            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);

            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos + glm::vec3(0.0f, 0.08f, -0.4899f));
            model = glm::scale(model, glm::vec3(0.6f, 0.4f, 0.02f));
            lightingShader.setMat4("model", model);

            glBindVertexArray(FWallVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // 绘制黑板上侧面
        {
            lightingShader.setVec3("objectColor", 0.75f, 0.5f, 0.3f);
            lightingShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setVec3("lightPos", lightPos);
            lightingShader.setVec3("viewPos", camera.Position);

            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);

            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos + glm::vec3(0.0f, 0.08f, -0.49f));
            model = glm::scale(model, glm::vec3(0.6f, 0.4f, 0.02f));
            lightingShader.setMat4("model", model);

            glBindVertexArray(CeilingVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // 绘制黑板下侧面
        {
            lightingShader.setVec3("objectColor", 0.75f, 0.5f, 0.3f);
            lightingShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setVec3("lightPos", lightPos);
            lightingShader.setVec3("viewPos", camera.Position);

            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);

            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos + glm::vec3(0.0f, 0.08f, -0.49f));
            model = glm::scale(model, glm::vec3(0.6f, 0.4f, 0.02f));
            lightingShader.setMat4("model", model);

            glBindVertexArray(FloorVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // 绘制黑板左侧面
        {
            lightingShader.setVec3("objectColor", 0.75f, 0.5f, 0.3f);
            lightingShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setVec3("lightPos", lightPos);
            lightingShader.setVec3("viewPos", camera.Position);

            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);

            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos + glm::vec3(0.0f, 0.08f, -0.49f));
            model = glm::scale(model, glm::vec3(0.6f, 0.4f, 0.02f));
            lightingShader.setMat4("model", model);

            glBindVertexArray(LWallVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // 绘制黑板右侧面
        {
            lightingShader.setVec3("objectColor", 0.75f, 0.5f, 0.3f);
            lightingShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setVec3("lightPos", lightPos);
            lightingShader.setVec3("viewPos", camera.Position);

            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);

            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos + glm::vec3(0.0f, 0.08f, -0.49f));
            model = glm::scale(model, glm::vec3(0.6f, 0.4f, 0.02f));
            lightingShader.setMat4("model", model);

            glBindVertexArray(RWallVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // 绘制风车线1
        if (windmillAppear) {
            lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setVec3("lightPos", lightPos);
            lightingShader.setVec3("viewPos", camera.Position);

            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);

            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos + glm::vec3(0.0f, 0.08f, -0.4897f));
            model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.02f));
            model = glm::rotate(model, glm::radians(windmillAngle), glm::vec3(0.0f, 0.0f, 1.0f));
            lightingShader.setMat4("model", model);

            glBindVertexArray(Windmill1VAO);
            glLineWidth(1.0f);
            glDrawArrays(GL_LINE_LOOP, 0, 3);
        }

        // 绘制风车线2
        if (windmillAppear) {
            lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setVec3("lightPos", lightPos);
            lightingShader.setVec3("viewPos", camera.Position);

            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);

            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos + glm::vec3(0.0f, 0.08f, -0.4897f));
            model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.02f));
            model = glm::rotate(model, glm::radians(windmillAngle), glm::vec3(0.0f, 0.0f, 1.0f));
            lightingShader.setMat4("model", model);

            glBindVertexArray(Windmill2VAO);
            glLineWidth(1.0f);
            glDrawArrays(GL_LINE_LOOP, 0, 3);
        }

        // 绘制风车线3
        if (windmillAppear) {
            lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setVec3("lightPos", lightPos);
            lightingShader.setVec3("viewPos", camera.Position);

            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);

            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos + glm::vec3(0.0f, 0.08f, -0.4897f));
            model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.02f));
            model = glm::rotate(model, glm::radians(windmillAngle + 90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            lightingShader.setMat4("model", model);

            glBindVertexArray(Windmill1VAO);
            glLineWidth(1.0f);
            glDrawArrays(GL_LINE_LOOP, 0, 3);
        }

        // 绘制风车线4
        if (windmillAppear) {
            lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setVec3("lightPos", lightPos);
            lightingShader.setVec3("viewPos", camera.Position);

            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);

            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos + glm::vec3(0.0f, 0.08f, -0.4897f));
            model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.02f));
            model = glm::rotate(model, glm::radians(windmillAngle + 90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            lightingShader.setMat4("model", model);

            glBindVertexArray(Windmill2VAO);
            glLineWidth(1.0f);
            glDrawArrays(GL_LINE_LOOP, 0, 3);
        }

        // 绘制风车线5
        if (windmillAppear) {
            lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setVec3("lightPos", lightPos);
            lightingShader.setVec3("viewPos", camera.Position);

            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);

            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos + glm::vec3(0.0f, 0.08f, -0.4897f));
            model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.02f));
            model = glm::rotate(model, glm::radians(windmillAngle + 180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            lightingShader.setMat4("model", model);

            glBindVertexArray(Windmill1VAO);
            glLineWidth(1.0f);
            glDrawArrays(GL_LINE_LOOP, 0, 3);
        }

        // 绘制风车线6
        if (windmillAppear) {
            lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setVec3("lightPos", lightPos);
            lightingShader.setVec3("viewPos", camera.Position);

            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);

            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos + glm::vec3(0.0f, 0.08f, -0.4897f));
            model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.02f));
            model = glm::rotate(model, glm::radians(windmillAngle + 180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            lightingShader.setMat4("model", model);

            glBindVertexArray(Windmill2VAO);
            glLineWidth(1.0f);
            glDrawArrays(GL_LINE_LOOP, 0, 3);
        }

        // 绘制风车线7
        if (windmillAppear) {
            lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setVec3("lightPos", lightPos);
            lightingShader.setVec3("viewPos", camera.Position);

            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);

            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos + glm::vec3(0.0f, 0.08f, -0.4897f));
            model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.02f));
            model = glm::rotate(model, glm::radians(windmillAngle + 270.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            lightingShader.setMat4("model", model);

            glBindVertexArray(Windmill1VAO);
            glLineWidth(1.0f);
            glDrawArrays(GL_LINE_LOOP, 0, 3);
        }

        // 绘制风车线8
        if (windmillAppear) {
            lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setVec3("lightPos", lightPos);
            lightingShader.setVec3("viewPos", camera.Position);

            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);

            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos + glm::vec3(0.0f, 0.08f, -0.4897f));
            model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.02f));
            model = glm::rotate(model, glm::radians(windmillAngle + 270.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            lightingShader.setMat4("model", model);

            glBindVertexArray(Windmill2VAO);
            glLineWidth(1.0f);
            glDrawArrays(GL_LINE_LOOP, 0, 3);
        }

        // 绘制风车面1
        if (windmillColorful) {
            lightingShader.setVec3("objectColor", windmillColor[0], windmillColor[1], windmillColor[2]);
            lightingShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setVec3("lightPos", lightPos);
            lightingShader.setVec3("viewPos", camera.Position);

            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);

            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos + glm::vec3(0.0f, 0.08f, -0.4898f));
            model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.02f));
            model = glm::rotate(model, glm::radians(windmillAngle), glm::vec3(0.0f, 0.0f, 1.0f));
            lightingShader.setMat4("model", model);

            glBindVertexArray(Windmill1VAO);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        // 绘制风车面2
        if (windmillColorful) {
            lightingShader.setVec3("objectColor", windmillColor[3], windmillColor[4], windmillColor[5]);
            lightingShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setVec3("lightPos", lightPos);
            lightingShader.setVec3("viewPos", camera.Position);

            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);

            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos + glm::vec3(0.0f, 0.08f, -0.4898f));
            model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.02f));
            model = glm::rotate(model, glm::radians(windmillAngle), glm::vec3(0.0f, 0.0f, 1.0f));
            lightingShader.setMat4("model", model);

            glBindVertexArray(Windmill2VAO);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        // 绘制风车面3
        if (windmillColorful) {
            lightingShader.setVec3("objectColor", windmillColor[6], windmillColor[7], windmillColor[8]);
            lightingShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setVec3("lightPos", lightPos);
            lightingShader.setVec3("viewPos", camera.Position);

            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);

            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos + glm::vec3(0.0f, 0.08f, -0.4898f));
            model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.02f));
            model = glm::rotate(model, glm::radians(windmillAngle + 90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            lightingShader.setMat4("model", model);

            glBindVertexArray(Windmill1VAO);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        // 绘制风车面4
        if (windmillColorful) {
            lightingShader.setVec3("objectColor", windmillColor[9], windmillColor[10], windmillColor[11]);
            lightingShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setVec3("lightPos", lightPos);
            lightingShader.setVec3("viewPos", camera.Position);

            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);

            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos + glm::vec3(0.0f, 0.08f, -0.4898f));
            model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.02f));
            model = glm::rotate(model, glm::radians(windmillAngle + 90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            lightingShader.setMat4("model", model);

            glBindVertexArray(Windmill2VAO);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        // 绘制风车面5
        if (windmillColorful) {
            lightingShader.setVec3("objectColor", windmillColor[12], windmillColor[13], windmillColor[14]);
            lightingShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setVec3("lightPos", lightPos);
            lightingShader.setVec3("viewPos", camera.Position);

            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);

            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos + glm::vec3(0.0f, 0.08f, -0.4898f));
            model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.02f));
            model = glm::rotate(model, glm::radians(windmillAngle + 180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            lightingShader.setMat4("model", model);

            glBindVertexArray(Windmill1VAO);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        // 绘制风车面6
        if (windmillColorful) {
            lightingShader.setVec3("objectColor", windmillColor[15], windmillColor[16], windmillColor[17]);
            lightingShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setVec3("lightPos", lightPos);
            lightingShader.setVec3("viewPos", camera.Position);

            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);

            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos + glm::vec3(0.0f, 0.08f, -0.4898f));
            model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.02f));
            model = glm::rotate(model, glm::radians(windmillAngle + 180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            lightingShader.setMat4("model", model);

            glBindVertexArray(Windmill2VAO);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        // 绘制风车面7
        if (windmillColorful) {
            lightingShader.setVec3("objectColor", windmillColor[18], windmillColor[19], windmillColor[20]);
            lightingShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setVec3("lightPos", lightPos);
            lightingShader.setVec3("viewPos", camera.Position);

            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);

            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos + glm::vec3(0.0f, 0.08f, -0.4898f));
            model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.02f));
            model = glm::rotate(model, glm::radians(windmillAngle + 270.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            lightingShader.setMat4("model", model);

            glBindVertexArray(Windmill1VAO);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        // 绘制风车面8
        if (windmillColorful) {
            lightingShader.setVec3("objectColor", windmillColor[21], windmillColor[22], windmillColor[23]);
            lightingShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setVec3("lightPos", lightPos);
            lightingShader.setVec3("viewPos", camera.Position);

            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);

            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos + glm::vec3(0.0f, 0.08f, -0.4898f));
            model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.02f));
            model = glm::rotate(model, glm::radians(windmillAngle + 270.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            lightingShader.setMat4("model", model);

            glBindVertexArray(Windmill2VAO);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        // 绘制灯方块
        {
            lightCubeShader.use();
            lightCubeShader.setMat4("projection", projection);
            lightCubeShader.setMat4("view", view);
            model = glm::mat4(1.0f);
            model = glm::translate(model, lightPos + glm::vec3(0.0f, -0.0001f, 0.0f));
            model = glm::scale(model, glm::vec3(0.1f)); // a smaller cube
            lightCubeShader.setMat4("model", model);

            glBindVertexArray(lightCubeVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        }

        // 绘制桌子和圣诞树
        if (tableDisplay) {
        // 绘制桌子
        christmasTreeShader.use();
        {
            christmasTreeShader.setVec3("lightAmbient", 0.5f * glm::vec3(1.0f, 1.0f, 1.0f));
            christmasTreeShader.setVec3("lightDiffuse", 0.2f * glm::vec3(1.0f, 1.0f, 1.0f));
            christmasTreeShader.setVec3("lightSpecular", glm::vec3(1.0f, 1.0f, 1.0f));
            christmasTreeShader.setVec3("lightPos", lightPos);
            christmasTreeShader.setVec3("viewPos", camera.Position);

            christmasTreeShader.setMat4("projection", projection);
            christmasTreeShader.setMat4("view", view);

            //// render the loaded model
            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos + glm::vec3(-0.25f, -0.4999f, -0.125f));
            model = glm::scale(model, glm::vec3(0.01f, 0.01f, 0.01f));
            model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            christmasTreeShader.setMat4("model", model);
            tableModel.Draw(christmasTreeShader);
        }        
        
        // 绘制圣诞树
        {
            christmasTreeShader.setVec3("lightAmbient", 0.5f * glm::vec3(1.0f, 1.0f, 1.0f));
            christmasTreeShader.setVec3("lightDiffuse", 0.2f * glm::vec3(1.0f, 1.0f, 1.0f));
            christmasTreeShader.setVec3("lightSpecular", glm::vec3(1.0f, 1.0f, 1.0f));
            christmasTreeShader.setVec3("lightPos", lightPos);
            christmasTreeShader.setVec3("viewPos", camera.Position);
            christmasTreeShader.setBool("isLightOn", isLightOn);

            christmasTreeShader.setMat4("projection", projection);
            christmasTreeShader.setMat4("view", view);

            //// render the loaded model
            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos + glm::vec3(-0.0f, -0.180f, -0.25f));
            model = glm::scale(model, glm::vec3(0.10f, 0.10f, 0.10f));
            christmasTreeShader.setMat4("model", model);
            christmasTreeModel.Draw(christmasTreeShader);
        }

        // 绘制地形
        terrainShader.use();
        {
            terrainShader.setInt("inner", inner);
            terrainShader.setInt("outer", outer);
            
            terrainShader.setMat4("projection", projection);
            terrainShader.setMat4("view", view);

            terrainShader.setVec3("color", 0.0f, 1.0f, 0.0f);
            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos + glm::vec3(0.0f, -0.1900f, -0.25f));
            model = glm::scale(model, glm::vec3(0.20f, 0.020f, 0.20f));
            terrainShader.setMat4("model", model);

            glBindVertexArray(terrainVAO);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glLineWidth(0.2f);
            glDrawElements(GL_PATCHES, terrainIndices.size(), GL_UNSIGNED_INT, terrainIndices.data());
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

            terrainShader.setVec3("color", 0.0f, 0.0f, 0.0f);
            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos + glm::vec3(0.0f, -0.1901f, -0.25f));
            model = glm::scale(model, glm::vec3(0.20f, 0.020f, 0.20f));
            terrainShader.setMat4("model", model);

            glBindVertexArray(terrainVAO);
            glDrawElements(GL_PATCHES, terrainIndices.size(), GL_UNSIGNED_INT, terrainIndices.data());
        }

        // 绘制平台
        lightingShader.use();
        {
            lightingShader.setVec3("objectColor", 0.4f, 0.3f, 0.2f);
            lightingShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setVec3("lightPos", lightPos);
            lightingShader.setVec3("viewPos", camera.Position);

            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);

            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos + glm::vec3(0.0f, -0.1899f, -0.25f));
            model = glm::scale(model, glm::vec3(0.20f, 0.020f, 0.20f));
            lightingShader.setMat4("model", model);

            glBindVertexArray(platformVAO);
            glDrawElements(GL_TRIANGLES, platformIndices.size(), GL_UNSIGNED_INT, platformIndices.data());
        }

        // 渲染雪花粒子
        snowShader.use();
        if (snowAppear) {
            snowShader.setMat4("projection", projection);
            snowShader.setMat4("view", view);

            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos + glm::vec3(0.0f, -0.19f, -0.25f));
            model = glm::scale(model, glm::vec3(0.19f, 0.20f, 0.19f));
            snowShader.setMat4("model", model);

            glBindVertexArray(snowVAO);
            glDrawArrays(GL_POINTS, 0, snowParticleCount);

#if 0
            // 显示圣诞树简易包围盒
            glBindVertexArray(christmaxTreeBoxVAO);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glLineWidth(2.0f);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
        }

        // 渲染光粒子
        lightPointShader.use();
        if (isLightOn) {
            lightPointShader.setMat4("projection", projection);
            lightPointShader.setMat4("view", view);
            lightPointShader.setFloat("time", glfwGetTime());

            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos + glm::vec3(0.0f, -0.19f, -0.25f));
            model = glm::scale(model, glm::vec3(0.19f, 0.20f, 0.19f));
            lightPointShader.setMat4("model", model);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, glowTexture);
            glBindVertexArray(lightPointVAO);
            glDrawArrays(GL_POINTS, 0, lightParticleCount);
        }
        }

        // 绘制面光源
        areaLightCubeShader.use();
        {
            areaLightCubeShader.setMat4("projection", projection);
            areaLightCubeShader.setMat4("view", view);
            areaLightCubeShader.setVec3("lightColor", areaLightColor);
            areaLightCubeShader.setInt("lightNum", lightNum);
            model = glm::mat4(1.0f);
            model = glm::translate(model, areaLightPos + glm::vec3(0.0f, -0.0002f, 0.0f));
            model = glm::scale(model, glm::vec3(0.32f, 0.02f, 0.32f)); // a smaller cube
            lightCubeShader.setMat4("model", model);

            glBindVertexArray(FloorVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6); 
        }

        // 面光源厚度面
        areaLightingShader.use();
        {
            //设置光照参数
            areaLightingShader.setVec3("viewPos", camera.Position);
            areaLightingShader.setVec3Array("lightPos", areaLightPosArray);
            areaLightingShader.setInt("lightNum", areaLightPosArray.size());
            areaLightingShader.setVec3("lightColor", areaLightColor);
            areaLightingShader.setVec4("albedo", 1.0f, 1.0f, 1.0f, 1.0f);
            areaLightingShader.setFloat("metallic", 0.0f);
            areaLightingShader.setFloat("roughness", 1.0f);
            areaLightingShader.setFloat("specular", 1.0f);

            // view/projection 变换
            areaLightingShader.setMat4("projection", projection);
            areaLightingShader.setMat4("view", view);

            // 世界坐标变换
            model = glm::mat4(1.0f);
            model = glm::translate(model, areaLightPos + glm::vec3(0.0f, -0.00001f, 0.0f));
            model = glm::scale(model, glm::vec3(0.32f, 0.02f, 0.32f)); // a smaller cube
            areaLightingShader.setMat4("model", model);

            // 渲染
            glBindVertexArray(lightCubeVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // 绘制管道
        areaLightingShader.use();
        {
            areaLightingShader.setVec3("viewPos", camera.Position);
            areaLightingShader.setVec3Array("lightPos", areaLightPosArray);
            areaLightingShader.setInt("lightNum", areaLightPosArray.size());
            areaLightingShader.setVec3("lightColor", areaLightColor);
            areaLightingShader.setVec4("albedo", pipeColorR, pipeColorG, pipeColorB, pipeColorA);
            areaLightingShader.setFloat("metallic", pipeMetallic);
            areaLightingShader.setFloat("roughness", pipeRoughness);
            areaLightingShader.setFloat("specular", pipeSpecular);

            areaLightingShader.setMat4("projection", projection);
            areaLightingShader.setMat4("view", view);

            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos);
            model = glm::scale(model, glm::vec3(0.25f, 0.12f, 0.12f));
            areaLightingShader.setMat4("model", model);

            glBindVertexArray(pipeVAO);
            glDrawElements(GL_TRIANGLES, pipeIndices.size(), GL_UNSIGNED_INT, 0);
        }

        // 绘制管道关键截面
        areaLightCubeShader.use();
        {
            areaLightCubeShader.setVec3("lightColor", 0.0f, 1.0f, 0.04f);
            areaLightCubeShader.setInt("lightNum", 1);
            areaLightCubeShader.setMat4("projection", projection);
            areaLightCubeShader.setMat4("view", view);

            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos);
            model = glm::scale(model, glm::vec3(0.25f, 0.12f, 0.12f));
            areaLightCubeShader.setMat4("model", model);

            glLineWidth(2.0f);
            
            glBindVertexArray(keySection1VAO);
            glDrawArrays(GL_LINE_LOOP, 0, sampleNum);

            glBindVertexArray(keySection2VAO);
            glDrawArrays(GL_LINE_LOOP, 0, sampleNum);

            glBindVertexArray(keySection3VAO);
            glDrawArrays(GL_LINE_LOOP, 0, sampleNum);
        }

        // glfw：交换缓冲区和轮询 IO 事件（按下/释放键、移动鼠标等）
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // （可选）一旦资源超出其用途，就取消分配所有资源：
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &CeilingVAO);
    glDeleteVertexArrays(1, &FloorVAO);
    glDeleteVertexArrays(1, &RWallVAO);
    glDeleteVertexArrays(1, &LWallVAO);
    glDeleteVertexArrays(1, &FWallVAO);
    glDeleteVertexArrays(1, &BWallVAO);
    glDeleteVertexArrays(1, &lightCubeVAO);
    glDeleteVertexArrays(1, &Windmill1VAO);
    glDeleteVertexArrays(1, &Windmill2VAO);
    glDeleteBuffers(1, &VBO1);
    glDeleteBuffers(1, &VBO2);
    glDeleteBuffers(1, &VBO3);
    glDeleteBuffers(1, &VBO4);
    glDeleteBuffers(1, &VBO5);
    glDeleteBuffers(1, &VBO6);
    glDeleteBuffers(1, &VBO7);
    glDeleteBuffers(1, &VBO8);
    glDeleteBuffers(1, &VBO9);


    // glfw：终止，清除所有以前分配的 GLFW 资源。
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

//查询 GLFW 是否按下/释放了该帧的相关键并做出相应的反应
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        windmillSpeed += 180.0f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        windmillSpeed -= 180.0f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        windmillAngle += 2.0f * windmillSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        windmillAngle -= 2.0f * windmillSpeed * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET)) {
        if (pipeMaterialSelect == 1) {
            pipeMetallic -= 0.2f * deltaTime;
            if (pipeMetallic < 0.0f)
                pipeMetallic = 0.0f;
        }
        if (pipeMaterialSelect == 2) {
            pipeRoughness -= 0.2f * deltaTime;
            if (pipeRoughness < 0.0f)
                pipeRoughness = 0.0f;
        }
        if (pipeMaterialSelect == 3) {
            pipeSpecular -= 0.2f * deltaTime;
            if (pipeSpecular < 0.0f)
                pipeSpecular = 0.0f;
        }
        if (pipeMaterialSelect == 4) {
            pipeColorR -= 0.4f * deltaTime;
            if (pipeColorR < 0.0f)
                pipeColorR = 0.0f;
        }
        if (pipeMaterialSelect == 5) {
            pipeColorG -= 0.4f * deltaTime;
            if (pipeColorG < 0.0f)
                pipeColorG = 0.0f;
        }
        if (pipeMaterialSelect == 6) {
            pipeColorB -= 0.4f * deltaTime;
            if (pipeColorB < 0.0f)
                pipeColorB = 0.0f;
        }
        if (pipeMaterialSelect == 7) {
            pipeColorA -= 0.4f * deltaTime;
            if (pipeColorA < 0.0f)
                pipeColorA = 0.0f;
        }
        if (pipeMaterialSelect == 8) {
            areaLightColor.r -= 0.4f * deltaTime;
            if (areaLightColor.r < 0.0f)
                areaLightColor.r = 0.0f;
        }
        if (pipeMaterialSelect == 9) {
            areaLightColor.g -= 0.4f * deltaTime;
            if (areaLightColor.g < 0.0f)
                areaLightColor.g = 0.0f;
        }
        if (pipeMaterialSelect == 0) {
            areaLightColor.b -= 0.4f * deltaTime;
            if (areaLightColor.b < 0.0f)
                areaLightColor.b = 0.0f;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET)) {
        if (pipeMaterialSelect == 1) {
            pipeMetallic += 0.2f * deltaTime;
            if (pipeMetallic > 1)
                pipeMetallic = 1;
        }
        if (pipeMaterialSelect == 2) {
            pipeRoughness += 0.2f * deltaTime;
            if (pipeRoughness > 1)
                pipeRoughness = 1;
        }
        if (pipeMaterialSelect == 3) {
            pipeSpecular += 0.2f * deltaTime;
            if (pipeSpecular > 1.0f)
                pipeSpecular = 1.0f;
        }
        if (pipeMaterialSelect == 4) {
            pipeColorR += 0.4f * deltaTime;
            if (pipeColorR > 1.0f)
                pipeColorR = 1.0f;
        }
        if (pipeMaterialSelect == 5) {
            pipeColorG += 0.4f * deltaTime;
            if (pipeColorG > 1.0f)
                pipeColorG = 1.0f;
        }
        if (pipeMaterialSelect == 6) {
            pipeColorB += 0.4f * deltaTime;
            if (pipeColorB > 1.0f)
                pipeColorB = 1.0f;
        }
        if (pipeMaterialSelect == 7) {
            pipeColorA += 0.4f * deltaTime;
            if (pipeColorA > 1.0f)
                pipeColorA = 1.0f;
        }
        if (pipeMaterialSelect == 8) {
            areaLightColor.r += 0.4f * deltaTime;
            if (areaLightColor.r > 1.0f)
                areaLightColor.r = 1.0f;
        }
        if (pipeMaterialSelect == 9) {
            areaLightColor.g += 0.4f * deltaTime;
            if (areaLightColor.g > 1.0f)
                areaLightColor.g = 1.0f;
        }
        if (pipeMaterialSelect == 0) {
            areaLightColor.b += 0.4f * deltaTime;
            if (areaLightColor.b > 1.0f)
                areaLightColor.b = 1.0f;
        }
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_F && action == GLFW_PRESS) {
        if (!windmillAppear) {
            windmillSpeed = 90.0f;
            windmillAngle = 0.0f;
            windmillRotate = false;
            windmillColorful = false;
            windmillAppear = true;
        } else {
            windmillRotate = false;
            windmillColorful = false;
            windmillAppear = false;
        }
    }

    if (key == GLFW_KEY_C && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        if (!windmillAppear)
            return;
        for (int i = 0; i < 24; ++i)
            windmillColor[i] = static_cast<float>(rand()) / RAND_MAX;
            windmillColorful = true;
    }

    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        if (!windmillAppear)
            return;
        windmillRotate = !windmillRotate;
    }

    if (key == GLFW_KEY_E && action == GLFW_PRESS) {
        snowAppear = !snowAppear;
    }

    if (key == GLFW_KEY_L && action == GLFW_PRESS) {
        isLightOn = !isLightOn;
    }

    if (GLFW_KEY_0 <= key && key <= GLFW_KEY_9 && action == GLFW_PRESS) {
        pipeMaterialSelect = key - GLFW_KEY_0;
    }
}

// glfw：每当窗口大小发生变化（通过操作系统或用户调整大小）时，此回调函数都会执行
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // 确保视区与新的窗口尺寸匹配;请注意，width和height将明显大于 Retina 显示屏上指定的高度
    glViewport(0, 0, width, height);
    glfwSwapBuffers(window);
}


// glfw: 每当鼠标移动时，该回调都会被调用
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // 反转，因为 y 坐标从下到上

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw:每当鼠标滚轮滚动时，该回调都会被调用
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

// 初始化雪花粒子
void initSnowParticle(SnowParticle& particle) {
    float x_r = (rand() % 100) / 100.0f * 2 - 1.0f;
    float z_r = (rand() % 100) / 100.0f * 2 - 1.0f;

    float x_c = x_r * glm::sqrt(1 - z_r * z_r / 2.0f) * 0.5f;
    float z_c = z_r * glm::sqrt(1 - x_r * x_r / 2.0f) * 0.5f;

    particle.position = glm::vec3(x_c, (rand() % 100) / 100.0f * 1.5f, z_c);
    particle.velocity = glm::vec3(0.0f, (rand() % 100) / 100.0f * 0.2f - 0.3f, 0.0f);
    float colorTmp = (rand() % 100) / 100.0f * 0.4f;
    particle.color = glm::vec3(1.0f - colorTmp, 1.0f - colorTmp, 1.0f);
}

// 初始化光粒子
void initLightParticle(LightParticle &particle) {
    float x_r = (rand() % 100) / 100.0f * 2 - 1.0f;
    float z_r = (rand() % 100) / 100.0f * 2 - 1.0f;

    float x_c = x_r * glm::sqrt(1 - z_r * z_r / 2.0f) * 0.48f;
    float z_c = z_r * glm::sqrt(1 - x_r * x_r / 2.0f) * 0.48f;

    particle.position = glm::vec3(x_c, (rand() % 100) / 100.0f * 1.15f + 0.05f, z_c);
    particle.velocity = glm::vec3((rand() % 100) / 100.0f * 0.06f - 0.03f, (rand() % 100) / 100.0f * 0.08f - 0.04f, (rand() % 100) / 100.0f * 0.06f - 0.03f);
    float colorTmp = (rand() % 100) / 100.0f * 0.6f;
    particle.color = glm::vec3(1.0f, 1.0f, 1.0f - colorTmp);

    particle.flashDelTime = (rand() % 100) / 100.0f;
}

glm::vec3 bezier2(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, float t) {
    return (1 - t) * (1 - t) * p0 + 2 * t * (1 - t) * p1 + t * t * p2;
}

glm::vec3 bezier3(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, float t) {
    return (1 - t) * (1 - t) * (1 - t) * p0 + 3 * t * (1 - t) * (1 - t) * p1 + 3 * t * t * (1 - t) * p2 + t * t * t * p3;
}