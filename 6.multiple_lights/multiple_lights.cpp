// multiple_lights.cpp
// Corrected instanced-cubes art example. Fixes: identifiers, setVec3 overloads, removed stray typos.

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>

#include <iostream>
#include <vector>
#include <cmath>
#include <string>

// callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
unsigned int loadTexture(const char* path);

// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// camera
Camera camera(glm::vec3(0.0f, 6.0f, 18.0f));
float lastX = (float)SCR_WIDTH / 2.0f;
float lastY = (float)SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int main()
{
    // GLFW init
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Instanced Art - Thousands of Cubes (fixed)", NULL, NULL);
    if (!window) { std::cout << "Failed to create GLFW window\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { std::cout << "Failed to initialize GLAD\n"; return -1; }

    glEnable(GL_DEPTH_TEST);

    // shaders
    Shader lightingShader("6.multiple_lights.vs", "6.multiple_lights.fs");
    Shader lightCubeShader("6.light_cube.vs", "6.light_cube.fs");

    // cube geometry (36 vertices)
    float vertices[] = {
        // positions          // normals           // texcoords
        -0.5f,-0.5f,-0.5f,    0.0f,0.0f,-1.0f,     0.0f,0.0f,
         0.5f,-0.5f,-0.5f,    0.0f,0.0f,-1.0f,     1.0f,0.0f,
         0.5f, 0.5f,-0.5f,    0.0f,0.0f,-1.0f,     1.0f,1.0f,
         0.5f, 0.5f,-0.5f,    0.0f,0.0f,-1.0f,     1.0f,1.0f,
        -0.5f, 0.5f,-0.5f,    0.0f,0.0f,-1.0f,     0.0f,1.0f,
        -0.5f,-0.5f,-0.5f,    0.0f,0.0f,-1.0f,     0.0f,0.0f,

        -0.5f,-0.5f, 0.5f,    0.0f,0.0f, 1.0f,     0.0f,0.0f,
         0.5f,-0.5f, 0.5f,    0.0f,0.0f, 1.0f,     1.0f,0.0f,
         0.5f, 0.5f, 0.5f,    0.0f,0.0f, 1.0f,     1.0f,1.0f,
         0.5f, 0.5f, 0.5f,    0.0f,0.0f, 1.0f,     1.0f,1.0f,
        -0.5f, 0.5f, 0.5f,    0.0f,0.0f, 1.0f,     0.0f,1.0f,
        -0.5f,-0.5f, 0.5f,    0.0f,0.0f, 1.0f,     0.0f,0.0f,

        -0.5f, 0.5f, 0.5f,   -1.0f,0.0f,0.0f,      1.0f,0.0f,
        -0.5f, 0.5f,-0.5f,   -1.0f,0.0f,0.0f,      1.0f,1.0f,
        -0.5f,-0.5f,-0.5f,   -1.0f,0.0f,0.0f,      0.0f,1.0f,
        -0.5f,-0.5f,-0.5f,   -1.0f,0.0f,0.0f,      0.0f,1.0f,
        -0.5f,-0.5f, 0.5f,   -1.0f,0.0f,0.0f,      0.0f,0.0f,
        -0.5f, 0.5f, 0.5f,   -1.0f,0.0f,0.0f,      1.0f,0.0f,

         0.5f, 0.5f, 0.5f,    1.0f,0.0f,0.0f,      1.0f,0.0f,
         0.5f, 0.5f,-0.5f,    1.0f,0.0f,0.0f,      1.0f,1.0f,
         0.5f,-0.5f,-0.5f,    1.0f,0.0f,0.0f,      0.0f,1.0f,
         0.5f,-0.5f,-0.5f,    1.0f,0.0f,0.0f,      0.0f,1.0f,
         0.5f,-0.5f, 0.5f,    1.0f,0.0f,0.0f,      0.0f,0.0f,
         0.5f, 0.5f, 0.5f,    1.0f,0.0f,0.0f,      1.0f,0.0f,

        -0.5f,-0.5f,-0.5f,    0.0f,-1.0f,0.0f,     0.0f,1.0f,
         0.5f,-0.5f,-0.5f,    0.0f,-1.0f,0.0f,     1.0f,1.0f,
         0.5f,-0.5f, 0.5f,    0.0f,-1.0f,0.0f,     1.0f,0.0f,
         0.5f,-0.5f, 0.5f,    0.0f,-1.0f,0.0f,     1.0f,0.0f,
        -0.5f,-0.5f, 0.5f,    0.0f,-1.0f,0.0f,     0.0f,0.0f,
        -0.5f,-0.5f,-0.5f,    0.0f,-1.0f,0.0f,     0.0f,1.0f,

        -0.5f, 0.5f,-0.5f,    0.0f,1.0f,0.0f,      0.0f,1.0f,
         0.5f, 0.5f,-0.5f,    0.0f,1.0f,0.0f,      1.0f,1.0f,
         0.5f, 0.5f, 0.5f,    0.0f,1.0f,0.0f,      1.0f,0.0f,
         0.5f, 0.5f, 0.5f,    0.0f,1.0f,0.0f,      1.0f,0.0f,
        -0.5f, 0.5f, 0.5f,    0.0f,1.0f,0.0f,      0.0f,0.0f,
        -0.5f, 0.5f,-0.5f,    0.0f,1.0f,0.0f,      0.0f,1.0f
    };

    // cube VAO/VBO
    unsigned int cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0); // pos
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); // normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float))); // texcoords
    glEnableVertexAttribArray(2);

    // Instancing parameters
    const unsigned int GRID = 120;      // GRID x GRID instances (120 -> 14,400 cubes)
    const float SPACING = 0.13f;        // distance between cubes
    const float SCALE = 0.85f;          // base cube scale

    // prepare instance data (packed vec4: x, z, phase, dist)
    std::vector<float> instanceData;
    instanceData.reserve(GRID * GRID * 4);
    for (unsigned int x = 0; x < GRID; ++x)
    {
        for (unsigned int z = 0; z < GRID; ++z)
        {
            float fx = ((float)x - (float)GRID / 2.0f) * SPACING;
            float fz = ((float)z - (float)GRID / 2.0f) * SPACING;
            float phase = (x * 0.7f + z * 1.3f) * 0.6f;
            float dist = std::sqrt(fx * fx + fz * fz);
            instanceData.push_back(fx);
            instanceData.push_back(fz);
            instanceData.push_back(phase);
            instanceData.push_back(dist);
        }
    }

    unsigned int instanceVBO;
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, instanceData.size() * sizeof(float), instanceData.data(), GL_STATIC_DRAW);

    // set instanced attribute: layout(location = 3) vec4 aInst
    glBindVertexArray(cubeVAO);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glVertexAttribDivisor(3, 1);

    // light cube VAO (reuse cubeVBO but only position)
    unsigned int lightCubeVAO;
    glGenVertexArrays(1, &lightCubeVAO);
    glBindVertexArray(lightCubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // shader initial uniforms
    lightingShader.use();
    lightingShader.setFloat("baseScale", SCALE);

    // colors for moving lights
    glm::vec3 baseColors[3] = {
        glm::vec3(1.0f, 0.55f, 0.12f),
        glm::vec3(0.12f, 0.55f, 1.0f),
        glm::vec3(0.9f, 0.2f, 0.9f)
    };

    // animation params (tweakable)
    const float GLOBAL_AMPLITUDE = 1.6f;
    const float GLOBAL_SPEED = 0.9f;
    const float PRIMARY_FREQ = 1.8f;
    const float SECONDARY_FREQ = 0.9f;
    const float RIPPLE_FREQ = 0.95f;
    const float HEIGHT_EXPONENT = 0.95f;

    // render loop
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // animate 3 point lights
        glm::vec3 lightPos[3];
        glm::vec3 lightCol[3];
        for (int i = 0; i < 3; ++i)
        {
            float t = currentFrame * (0.3f + 0.08f * i);
            float r = 6.5f + 1.2f * i;
            lightPos[i] = glm::vec3(r * cos(t * (0.6f + 0.1f * i)), 1.8f + 0.8f * sin(t * (0.7f + 0.05f * i)), r * sin(t * (0.6f + 0.1f * i)));
            lightCol[i] = baseColors[i] * (0.75f + 0.25f * (0.5f + 0.5f * sin(t * (0.9f + 0.06f * i))));
        }

        // clear
        glClearColor(0.02f, 0.02f, 0.03f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // use lighting shader
        lightingShader.use();
        lightingShader.setVec3("viewPos", camera.Position);

        // directional light (use glm::vec3 for setVec3)
        lightingShader.setVec3("dirLight.direction", glm::vec3(-0.2f, -1.0f, -0.25f));
        lightingShader.setVec3("dirLight.ambient", glm::vec3(0.02f, 0.02f, 0.03f));
        lightingShader.setVec3("dirLight.diffuse", glm::vec3(0.32f, 0.32f, 0.36f));
        lightingShader.setVec3("dirLight.specular", glm::vec3(0.5f, 0.5f, 0.5f));

        // set point lights (first 3); shader expects pointLights[0..3]
        for (int i = 0; i < 3; ++i)
        {
            std::string base = "pointLights[" + std::to_string(i) + "]";
            lightingShader.setVec3(base + ".position", lightPos[i]);
            lightingShader.setVec3(base + ".ambient", lightCol[i] * 0.02f);
            lightingShader.setVec3(base + ".diffuse", lightCol[i] * 0.95f);
            lightingShader.setVec3(base + ".specular", glm::vec3(1.0f));
            lightingShader.setFloat(base + ".constant", 1.0f);
            lightingShader.setFloat(base + ".linear", 0.07f);
            lightingShader.setFloat(base + ".quadratic", 0.017f);
        }
        // disable the 4th point light
        {
            std::string base = "pointLights[3]";
            lightingShader.setVec3(base + ".position", glm::vec3(0.0f, -50.0f, 0.0f));
            lightingShader.setVec3(base + ".ambient", glm::vec3(0.0f));
            lightingShader.setVec3(base + ".diffuse", glm::vec3(0.0f));
            lightingShader.setVec3(base + ".specular", glm::vec3(0.0f));
            lightingShader.setFloat(base + ".constant", 1.0f);
            lightingShader.setFloat(base + ".linear", 0.09f);
            lightingShader.setFloat(base + ".quadratic", 0.032f);
        }

        // spotlight from camera
        lightingShader.setVec3("spotLight.position", camera.Position);
        lightingShader.setVec3("spotLight.direction", camera.Front);
        lightingShader.setVec3("spotLight.ambient", glm::vec3(0.0f));
        lightingShader.setVec3("spotLight.diffuse", glm::vec3(1.0f));
        lightingShader.setVec3("spotLight.specular", glm::vec3(1.0f));
        lightingShader.setFloat("spotLight.constant", 1.0f);
        lightingShader.setFloat("spotLight.linear", 0.09f);
        lightingShader.setFloat("spotLight.quadratic", 0.032f);
        lightingShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
        lightingShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));

        // projection / view
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 120.0f);
        glm::mat4 view = camera.GetViewMatrix();
        lightingShader.setMat4("projection", projection);
        lightingShader.setMat4("view", view);

        // pass animation uniforms
        lightingShader.setFloat("time", currentFrame * GLOBAL_SPEED);
        lightingShader.setFloat("amplitude", GLOBAL_AMPLITUDE);
        lightingShader.setFloat("freq", PRIMARY_FREQ);
        lightingShader.setFloat("freq2", SECONDARY_FREQ);
        lightingShader.setFloat("rippleFreq", RIPPLE_FREQ);
        lightingShader.setFloat("heightPow", HEIGHT_EXPONENT);
        lightingShader.setFloat("baseScale", SCALE);

        // draw instanced cubes
        glBindVertexArray(cubeVAO);
        unsigned int amount = GRID * GRID;
        glDrawArraysInstanced(GL_TRIANGLES, 0, 36, amount);

        // swap
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // cleanup
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteVertexArrays(1, &lightCubeVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteBuffers(1, &instanceVBO);

    glfwTerminate();
    return 0;
}

// input & callbacks
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.ProcessKeyboard(RIGHT, deltaTime);
}

void framebuffer_size_callback(GLFWwindow* /*window*/, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* /*window*/, double xposIn, double yposIn)
{
    float xpos = (float)xposIn, ypos = (float)yposIn;
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos; lastY = ypos;
    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* /*window*/, double /*xoffset*/, double yoffset)
{
    camera.ProcessMouseScroll((float)yoffset);
}

// trivial texture loader (not used in current shaders, but kept)
unsigned int loadTexture(char const* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format = (nrComponents == 1) ? GL_RED : (nrComponents == 3) ? GL_RGB : GL_RGBA;
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, (GLint)format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }
    return textureID;
}
