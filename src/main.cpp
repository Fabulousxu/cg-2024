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

// ѩ������
struct SnowParticle {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 color;  // ѩ����ɫ�����ڰ�ɫ��ǳ��ɫ֮��
};

// ������
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

// ��������
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// ���������
Camera camera(glm::vec3(0.0f, 0.3f, 3.3f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// ʱ������
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// ��������
glm::vec3 lightPos(0.0f, 0.75f, 1.65f);
glm::vec3 cubePos(0.0f, 0.3f, 2.0f);

// �糵�����ɫ
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

bool windmillAppear = false; // �糵�Ƿ����
bool windmillColorful = false; // �糵�Ƿ������ɫ
bool windmillRotate = false; // �糵�Ƿ���ת
float windmillSpeed = 90.0f; // �糵��ת�ٶ�
float windmillAngle = 0.0f; // �糵��ǰ�Ƕ�
bool snowAppear = false;     // �Ƿ���ѩ��Ч��
unsigned int snowParticleCount = 400; // ѩ����������
bool isLightOn = false; // ʥ�����Ƿ�����
unsigned int lightParticleCount = 100; // ����������

int main()
{
    // ��ʼ��������glfw
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw��������
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

    // ���� GLFW �������ǵ����
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad���������� OpenGL ����ָ��
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // ����ȫ�� OpenGL ״̬
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_POINT_SPRITE);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // ����shader����
    // ------------------------------------
    Shader lightingShader("shaders/lighting.vs.glsl", "shaders/lighting.fs.glsl");
    Shader lightCubeShader("shaders/lightcube.vs.glsl", "shaders/lightcube.fs.glsl");
    Shader christmasTreeShader("shaders/christmas_tree.vs.glsl", "shaders/christmas_tree.fs.glsl");
    Shader terrainShader("shaders/terrain.vert.glsl", "shaders/terrain.frag.glsl", "shaders/terrain.tesc.glsl", "shaders/terrain.tese.glsl", "shaders/terrain.gs.glsl");
    Shader snowShader("shaders/snow.vs.glsl", "shaders/snow.fs.glsl");
    Shader lightPointShader("shaders/lightpoint.vs.glsl", "shaders/lightpoint.fs.glsl");

    Model christmasTreeModel("models/obj/christmas_tree/christmas_tree.obj");
    Model tableModel("models/obj/table/table.obj");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    int inner = 1;
    int outer = 1;

    // ͳһ�����õ���������Ϣ(ÿһ��ǰ��������Ϊ������꣬������Ϊ������)
    // ------------------------------------------------------------------
    float vertices[] = {
        // �����嶥������
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,

        // �糵��������
          0.0f,   0.0f,   0.5f,   0.0f,   0.0f,   1.0f,
          0.5f,   0.0f,   0.5f,   0.0f,   0.0f,   1.0f,
          0.5f,   0.5f,   0.5f,   0.0f,   0.0f,   1.0f,

          0.0f,   0.0f,   0.5f,   0.0f,   0.0f,   1.0f,
         0.25f,  0.25f,   0.5f,   0.0f,   0.0f,   1.0f,
          0.0f,   0.5f,   0.5f,   0.0f,   0.0f,   1.0f,
    };

    //��ȡ����ƽ�������
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


    // �����컨��Ķ�����Ϣ
    // ------------------------------------------------------------------
    unsigned int VBO1, CeilingVAO;
    {
        glGenVertexArrays(1, &CeilingVAO);
        glGenBuffers(1, &VBO1);

        glBindBuffer(GL_ARRAY_BUFFER, VBO1);
        glBufferData(GL_ARRAY_BUFFER, sizeof(CeilingVertices), CeilingVertices, GL_STATIC_DRAW);

        glBindVertexArray(CeilingVAO);

        // ����λ��
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(0 * sizeof(float)));
        glEnableVertexAttribArray(0);
        // ���뷨����
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    //����ذ�Ķ�����Ϣ
    // ------------------------------------------------------------------
    unsigned int VBO2, FloorVAO;
    {
        glGenVertexArrays(1, &FloorVAO);
        glGenBuffers(1, &VBO2);

        glBindBuffer(GL_ARRAY_BUFFER, VBO2);
        glBufferData(GL_ARRAY_BUFFER, sizeof(FloorVertices), FloorVertices, GL_STATIC_DRAW);

        glBindVertexArray(FloorVAO);

        // ����λ��
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(0 * sizeof(float)));
        glEnableVertexAttribArray(0);
        // ���뷨����
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    //������ǽ�Ķ�����Ϣ
    // ------------------------------------------------------------------
    unsigned int VBO3, LWallVAO;
    {
        glGenVertexArrays(1, &LWallVAO);
        glGenBuffers(1, &VBO3);

        glBindBuffer(GL_ARRAY_BUFFER, VBO3);
        glBufferData(GL_ARRAY_BUFFER, sizeof(LWallVertices), LWallVertices, GL_STATIC_DRAW);

        glBindVertexArray(LWallVAO);

        // ����λ��
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(0 * sizeof(float)));
        glEnableVertexAttribArray(0);
        // ���뷨����
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    //������ǽ�Ķ�����Ϣ
    // ------------------------------------------------------------------
    unsigned int VBO4, RWallVAO;
    {
        glGenVertexArrays(1, &RWallVAO);
        glGenBuffers(1, &VBO4);

        glBindBuffer(GL_ARRAY_BUFFER, VBO4);
        glBufferData(GL_ARRAY_BUFFER, sizeof(RWallVertices), RWallVertices, GL_STATIC_DRAW);

        glBindVertexArray(RWallVAO);

        // ����λ��
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(0 * sizeof(float)));
        glEnableVertexAttribArray(0);
        // ���뷨����
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    //����ǰǽ�Ķ�����Ϣ
    // ------------------------------------------------------------------
    unsigned int VBO5, FWallVAO;
    {
        glGenVertexArrays(1, &FWallVAO);
        glGenBuffers(1, &VBO5);

        glBindBuffer(GL_ARRAY_BUFFER, VBO5);
        glBufferData(GL_ARRAY_BUFFER, sizeof(FWallVertices), FWallVertices, GL_STATIC_DRAW);

        glBindVertexArray(FWallVAO);

        // ����λ��
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(0 * sizeof(float)));
        glEnableVertexAttribArray(0);
        // ���뷨����
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    // ���뷽��ƵĶ�����Ϣ
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

    //�����ǽ�Ķ�����Ϣ
    //  ------------------------------------------------------------------
    unsigned int VBO7, BWallVAO;
    {
        glGenVertexArrays(1, &BWallVAO);
        glGenBuffers(1, &VBO7);

        glBindBuffer(GL_ARRAY_BUFFER, VBO7);
        glBufferData(GL_ARRAY_BUFFER, sizeof(BWallVertices), BWallVertices, GL_STATIC_DRAW);

        glBindVertexArray(BWallVAO);

        // ����λ��
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(0 * sizeof(float)));
        glEnableVertexAttribArray(0);
        // ���뷨����
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    // ����糵��1�Ķ�����Ϣ
    unsigned int VBO8, Windmill1VAO;
    {
        glGenVertexArrays(1, &Windmill1VAO);
        glGenBuffers(1, &VBO8);

        glBindBuffer(GL_ARRAY_BUFFER, VBO8);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Windmill1Vertices), Windmill1Vertices, GL_DYNAMIC_DRAW);

        glBindVertexArray(Windmill1VAO);

        // ����λ��
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(0 * sizeof(float)));
        glEnableVertexAttribArray(0);
        // ���뷨����
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    // ����糵��2�Ķ�����Ϣ
    unsigned int VBO9, Windmill2VAO;
    {
        glGenVertexArrays(1, &Windmill2VAO);
        glGenBuffers(1, &VBO9);

        glBindBuffer(GL_ARRAY_BUFFER, VBO9);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Windmill2Vertices), Windmill2Vertices, GL_DYNAMIC_DRAW);

        glBindVertexArray(Windmill2VAO);

        // ����λ��
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(0 * sizeof(float)));
        glEnableVertexAttribArray(0);
        // ���뷨����
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    std::vector<float> terrainVertices;
    std::vector<unsigned int> terrainIndices;
    const unsigned int terrainWidth = 64;

    // ����ԭʼ���ζ�����Ϣ����ӳ���Բ
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
    
    // ���ɵ�������
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

    // ƽ���˲�����ͼ
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

    // ������ζ�����Ϣ
    unsigned int VBO10, terrainVAO, terrainEBO;
    {
        glGenVertexArrays(1, &terrainVAO);
        glGenBuffers(1, &VBO10);
        glGenBuffers(1, &terrainEBO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO10);
        glBufferData(GL_ARRAY_BUFFER, terrainVertices.size() * sizeof(float), terrainVertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrainEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, terrainIndices.size() * sizeof(unsigned int), terrainIndices.data(), GL_STATIC_DRAW);

        glBindVertexArray(terrainVAO);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)(0 * sizeof(float)));
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    
    std::vector<float> platformVertices;
    std::vector<unsigned int> platformIndices;

    // ����ƽ̨���涥��
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

    // ����ƽ̨����
    for (int i = 0; i < platformVertices.size() / 12; ++i) {
        platformIndices.push_back(i * 2);
        platformIndices.push_back((i * 2 + 1) % (platformVertices.size() / 6));
        platformIndices.push_back((i * 2 + 3) % (platformVertices.size() / 6));

        platformIndices.push_back(i * 2);
        platformIndices.push_back((i * 2 + 3) % (platformVertices.size() / 6));
        platformIndices.push_back((i * 2 + 2) % (platformVertices.size() / 6));
    }
    
    // ����ƽ̨������Ϣ
    unsigned int VBO11, platformVAO, platformEBO;
    {
        glGenVertexArrays(1, &platformVAO);
        glGenBuffers(1, &VBO11);
        glGenBuffers(1, &platformEBO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO11);
        glBufferData(GL_ARRAY_BUFFER, platformVertices.size() * sizeof(float), platformVertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, platformEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, platformIndices.size() * sizeof(unsigned int), platformIndices.data(), GL_STATIC_DRAW);

        glBindVertexArray(platformVAO);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(0 * sizeof(float)));
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }


    // ѩ������
    std::vector<SnowParticle> snowParticles;

    // ��ʼ��ѩ������
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

    // ��ʾʥ������Χ��
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


    // ������
    std::vector<LightParticle> lightParticles;

    // ��ʼ��������
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

    // ���ع����Ӳ���
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


    // ��ʼ�����������
    srand(static_cast<unsigned int>(glfwGetTime() * 1000));

    // ��Ⱦѭ��
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // ʱ���߼�
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // ����
        // -----
        processInput(window);

        // ����糵��ת
        if (windmillRotate) {
            windmillAngle += windmillSpeed * deltaTime;
            windmillAngle = windmillAngle - static_cast<int>(windmillAngle) + static_cast<int>(windmillAngle) % 360;
        }

        // ����ѩ������
        if (snowAppear) {
            for (auto& particle : snowParticles) {
                particle.position += particle.velocity * deltaTime;

                float x = particle.position.x;
                float y = particle.position.y;
                float z = particle.position.z;

                auto tmp = (boxUpCenter - y) / (boxUpCenter - boxDownCenter) * boxRadius;

                // ʥ������Χ����ײ���
                if (y < 0.0f || ((y <= boxUpCenter || y >= boxDownCenter) && x * x + z * z < tmp * tmp)) {
                    initSnowParticle(particle);
                }
            }
            glBindBuffer(GL_ARRAY_BUFFER, VBO12);
            glBufferSubData(GL_ARRAY_BUFFER, 0, snowParticles.size() * sizeof(SnowParticle), snowParticles.data());
        }

        // ���¹�����
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

        // ��ʼ��Ⱦ
        // ------
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ȷ�������� Uniforms/Drawing ����ʱ���� Shader
        //---------------------------------------------------------------------
        lightingShader.use();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 model = glm::mat4(1.0f);

        //�����컨��
        {
            //���ù��ղ���
            lightingShader.setVec3("objectColor", 0.5, 0.5f, 0.5f);
            lightingShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setVec3("lightPos", lightPos);
            lightingShader.setVec3("viewPos", camera.Position);

            // view/projection �任
            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);

            // ��������任
            model = glm::translate(model, cubePos);
            model = glm::scale(model, glm::vec3(1.0f));
            lightingShader.setMat4("model", model);

            // ��Ⱦ
            glBindVertexArray(CeilingVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // ���Ƶذ�
        {
            //���ù��ղ���
            lightingShader.setVec3("objectColor", 0.5f, 0.5f, 0.5f);
            lightingShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setVec3("lightPos", lightPos);
            lightingShader.setVec3("viewPos", camera.Position);

            // view/projection �任
            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);

            // ��������任
            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos);
            model = glm::scale(model, glm::vec3(1.0f));
            lightingShader.setMat4("model", model);

            // ��Ⱦ
            glBindVertexArray(FloorVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // ������ǽ
        {
            //���ù��ղ���
            lightingShader.setVec3("objectColor", 1.0f, 0.0f, 0.31f);
            lightingShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setVec3("lightPos", lightPos);
            lightingShader.setVec3("viewPos", camera.Position);

            // view/projection �任
            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);

            // ��������任
            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos);
            model = glm::scale(model, glm::vec3(1.0f));
            lightingShader.setMat4("model", model);

            // ��Ⱦ
            glBindVertexArray(LWallVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // ������ǽ
        {
            //���ù��ղ���
            lightingShader.setVec3("objectColor", 1.0f, 0.5f, 0.31f);
            lightingShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setVec3("lightPos", lightPos);
            lightingShader.setVec3("viewPos", camera.Position);

            // view/projection �任
            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);

            // ��������任
            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos);
            model = glm::scale(model, glm::vec3(1.0f));
            lightingShader.setMat4("model", model);

            // ��Ⱦ
            glBindVertexArray(RWallVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // ����ǰǽ
        {
            //���ù��ղ���
            lightingShader.setVec3("objectColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
            lightingShader.setVec3("lightPos", lightPos);
            lightingShader.setVec3("viewPos", camera.Position);

            // view/projection �任
            lightingShader.setMat4("projection", projection);
            lightingShader.setMat4("view", view);

            // ��������任
            model = glm::mat4(1.0f);
            model = glm::translate(model, cubePos);
            model = glm::scale(model, glm::vec3(1.0f));
            lightingShader.setMat4("model", model);

            // ��Ⱦ
            glBindVertexArray(FWallVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // ���ƺڰ�����߿򲿷�
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

        // ���ƺڰ�����ڰ岿��
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

        // ���ƺڰ屳��
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

        // ���ƺڰ��ϲ���
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

        // ���ƺڰ��²���
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

        // ���ƺڰ������
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

        // ���ƺڰ��Ҳ���
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

        // ���Ʒ糵��1
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

        // ���Ʒ糵��2
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

        // ���Ʒ糵��3
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

        // ���Ʒ糵��4
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

        // ���Ʒ糵��5
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

        // ���Ʒ糵��6
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

        // ���Ʒ糵��7
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

        // ���Ʒ糵��8
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

        // ���Ʒ糵��1
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

        // ���Ʒ糵��2
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

        // ���Ʒ糵��3
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

        // ���Ʒ糵��4
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

        // ���Ʒ糵��5
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

        // ���Ʒ糵��6
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

        // ���Ʒ糵��7
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

        // ���Ʒ糵��8
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

        // ���ƵƷ���
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

        christmasTreeShader.use();
        // ��������
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
        
        // ����ʥ����
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

        terrainShader.use();
        // ���Ƶ���
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

        // ����ƽ̨
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

        // ��Ⱦѩ������
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
            // ��ʾʥ�������װ�Χ��
            glBindVertexArray(christmaxTreeBoxVAO);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glLineWidth(2.0f);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
        }

        // ��Ⱦ������
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


        // glfw����������������ѯ IO �¼�������/�ͷż����ƶ����ȣ�
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ����ѡ��һ����Դ��������;����ȡ������������Դ��
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


    // glfw����ֹ�����������ǰ����� GLFW ��Դ��
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

//��ѯ GLFW �Ƿ���/�ͷ��˸�֡����ؼ���������Ӧ�ķ�Ӧ
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
}

// glfw��ÿ�����ڴ�С�����仯��ͨ������ϵͳ���û�������С��ʱ���˻ص���������ִ��
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // ȷ���������µĴ��ڳߴ�ƥ��;��ע�⣬width��height�����Դ��� Retina ��ʾ����ָ���ĸ߶�
    glViewport(0, 0, width, height);
    glfwSwapBuffers(window);
}


// glfw: ÿ������ƶ�ʱ���ûص����ᱻ����
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
    float yoffset = lastY - ypos; // ��ת����Ϊ y ������µ���

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw:ÿ�������ֹ���ʱ���ûص����ᱻ����
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

// ��ʼ��ѩ������
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

// ��ʼ��������
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