// main_driving.cpp
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>
#include <vector>
#include <cmath>
#include <cfloat> // FLT_MAX
#include <algorithm>

// compute normalization transform (center -> scale -> translate)
glm::mat4 getNormalizationTransform(const Model& m, float targetSize, const glm::vec3& worldPos = glm::vec3(0.0f))
{
    glm::vec3 minP(FLT_MAX, FLT_MAX, FLT_MAX);
    glm::vec3 maxP(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    // Access LearnOpenGL Model's meshes and vertices (works with standard LearnOpenGL Model class)
    for (const auto& mesh : m.meshes)
    {
        for (const auto& v : mesh.vertices)
        {
            glm::vec3 p = v.Position;
            minP.x = std::min(minP.x, p.x);
            minP.y = std::min(minP.y, p.y);
            minP.z = std::min(minP.z, p.z);
            maxP.x = std::max(maxP.x, p.x);
            maxP.y = std::max(maxP.y, p.y);
            maxP.z = std::max(maxP.z, p.z);
        }
    }

    glm::vec3 size = maxP - minP;
    float maxDim = std::max(size.x, std::max(size.y, size.z));
    float scale = 1.0f;
    if (maxDim > 0.0f) scale = targetSize / maxDim;

    glm::vec3 center = (minP + maxP) * 0.5f;

    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::translate(transform, -center);
    transform = glm::scale(transform, glm::vec3(scale));
    transform = glm::translate(transform, worldPos);
    return transform;
}


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
unsigned int loadTexture(const char* path);
unsigned int loadCubemap(const std::vector<std::string>& faces);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera (we will override camera position each frame to follow the car)
Camera camera(glm::vec3(0.0f, 3.0f, 6.0f));
float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// --- Car state (global so processInput can modify) ---
glm::vec3 carPosition(0.0f, 0.0f, 0.0f); // Y should match your model's ground
float carRotation = 0.0f; // degrees around Y axis, 0 means +Z forward
float carSpeed = 0.0f;

const float MAX_SPEED = 15.0f;         // units per second
const float ACCELERATION = 10.0f;      // units per second^2
const float BRAKE = 12.0f;            // braking deceleration
const float TURN_SPEED = 90.0f;       // degrees per second (steering rate)
const float FRICTION = 4.0f;          // natural slow down

int main()
{
    // glfw init
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Driving Demo", NULL, NULL);
    if (!window)
    {
        std::cout << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    // capture mouse (optional)
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD\n";
        return -1;
    }

    stbi_set_flip_vertically_on_load(true);

    glEnable(GL_DEPTH_TEST);

    // shaders (use your existing shader files or model loading shaders)
    Shader shader("6.1.cubemaps.vs", "6.1.cubemaps.fs");       // use a shader that supports textures and basic lighting
    Shader skyboxShader("6.1.skybox.vs", "6.1.skybox.fs");

    // --- (keep your cube and skybox vertex data) ---
    float cubeVertices[] = {
        // positions          // texture Coords
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    };
    float skyboxVertices[] = {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    // Cube VAO (not used heavily but kept)
    unsigned int cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    // Replace sizeof(cubeVertices) with the actual size in bytes when you paste full array
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    // skybox VAO
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    // load textures
    unsigned int cubeTexture = loadTexture(FileSystem::getPath("resources/textures/container.jpg").c_str());

    std::vector<std::string> faces
    {
        FileSystem::getPath("resources/textures/skybox/right.jpg"),
        FileSystem::getPath("resources/textures/skybox/left.jpg"),
        FileSystem::getPath("resources/textures/skybox/top.jpg"),
        FileSystem::getPath("resources/textures/skybox/bottom.jpg"),
        FileSystem::getPath("resources/textures/skybox/front.jpg"),
        FileSystem::getPath("resources/textures/skybox/back.jpg")
    };
    stbi_set_flip_vertically_on_load(false);
    unsigned int cubemapTexture = loadCubemap(faces);
    stbi_set_flip_vertically_on_load(true);

    shader.use();
    shader.setInt("texture1", 0);

    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    // --- Load models (car & city) ---
    // Ensure these paths match your resources layout.
    Model city(FileSystem::getPath("resources/objects/city/city.obj"));
    Model car(FileSystem::getPath("resources/objects/car/car.obj"));
    glm::mat4 cityBase = getNormalizationTransform(city, 200.0f, glm::vec3(0.0f));    // city scaled to ~200 units
    glm::mat4 carBase = getNormalizationTransform(car, 10.0f, glm::vec3(0.0f));

    // You might need to tune initial car position/scale depending on model origin/size:
    carPosition = glm::vec3(112.0f, 26.5f, -120.0f);
    carRotation = 180.0f; // face towards -Z or +Z depending on your model

    // render loop
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        processInput(window);

        // simple friction
        if (fabs(carSpeed) > 0.01f)
        {
            float sign = (carSpeed > 0.0f) ? 1.0f : -1.0f;
            carSpeed -= sign * FRICTION * deltaTime;
            if (sign > 0.0f && carSpeed < 0.0f) carSpeed = 0.0f;
            if (sign < 0.0f && carSpeed > 0.0f) carSpeed = 0.0f;
        }
        else carSpeed = 0.0f;

        // update camera to follow the car (third-person)
        const float camDistance = 22.0f;
        const float camHeight = 8.0f;

        // compute direction vector from camera yaw/pitch (Camera class exposes Yaw and Pitch)
        float yawRad = glm::radians(camera.Yaw);
        float pitchRad = glm::radians(camera.Pitch);

        // direction vector pointing from camera towards forward direction defined by yaw/pitch
        glm::vec3 camDir;
        camDir.x = cos(pitchRad) * sin(yawRad);
        camDir.y = sin(pitchRad);
        camDir.z = cos(pitchRad) * cos(yawRad);

        // place the camera behind the car along the direction (offset origin to car position)
        camera.Position = carPosition - camDir * camDistance + glm::vec3(0.0f, camHeight, 0.0f);

        // Ensure the camera looks at the car — this sets the view but does not override yaw/pitch values
        camera.Front = glm::normalize(carPosition - camera.Position);
        // render
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // set common matrices
        shader.use();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
        glm::mat4 view = camera.GetViewMatrix();
        shader.setMat4("view", view);
        shader.setMat4("projection", projection);

        // --- Draw city ---
        glm::mat4 model = cityBase; // already includes scale & recenter
        shader.setMat4("model", model);
        city.Draw(shader);

        // --- Draw car (nanosuit) at carPosition with rotation and the carBase normalization ---
        model = glm::mat4(1.0f);
        model = glm::translate(model, carPosition);
        model = glm::rotate(model, glm::radians(carRotation), glm::vec3(0.0f, 1.0f, 0.0f));
        model = model * carBase; // apply normalization after translation/rotation so it's aligned correctly
        shader.setMat4("model", model);
        car.Draw(shader);

        // --- Draw skybox last ---
        glDepthFunc(GL_LEQUAL);
        skyboxShader.use();
        // remove translation from view for skybox
        glm::mat4 skyboxView = glm::mat4(glm::mat3(camera.GetViewMatrix()));
        skyboxShader.setMat4("view", skyboxView);
        skyboxShader.setMat4("projection", projection);

        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // cleanup
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteBuffers(1, &skyboxVBO);

    glfwTerminate();
    return 0;
}

// process input -> car control (W/S to accelerate/brake, A/D to steer)
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // acceleration/brake
    bool forward = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
    bool backward = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
    bool left = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
    bool right = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;

    // accelerate / brake
    if (forward)
    {
        carSpeed += ACCELERATION * deltaTime;
    }
    else if (backward)
    {
        // if moving forward, stronger braking
        if (carSpeed > 0.0f) carSpeed -= BRAKE * deltaTime;
        else carSpeed -= ACCELERATION * deltaTime;
    }

    // clamp speed
    if (carSpeed > MAX_SPEED) carSpeed = MAX_SPEED;
    if (carSpeed < -MAX_SPEED / 2.0f) carSpeed = -MAX_SPEED / 2.0f; // reverse slower

    // steering - steering effectiveness depends on speed sign
    float steerDirection = 0.0f;
    if (left) steerDirection = 1.0f;
    if (right) steerDirection = -1.0f;

    if (fabs(carSpeed) > 0.01f)
    {
        // steering scaled by speed magnitude (faster -> more effective steering)
        float steer = TURN_SPEED * (carSpeed / MAX_SPEED) * steerDirection;
        carRotation += steer * deltaTime;
    }
    else
    {
        // when stopped: allow slow in-place turning so player can orient car while idle
        const float INPLACE_FACTOR = 0.6f; // tweak (0.0 .. 1.0)
        carRotation += TURN_SPEED * INPLACE_FACTOR * steerDirection * deltaTime;
    }


    // move car forward in its facing direction
    float rad = glm::radians(carRotation);
    glm::vec3 forwardVec = glm::vec3(sin(rad), 0.0f, cos(rad));
    carPosition += forwardVec * carSpeed * deltaTime;

    // simple boundary clamp to keep car inside some area (tweak as needed)
    float BOUND = 500.0f;
    carPosition.x = glm::clamp(carPosition.x, -BOUND, BOUND);
    carPosition.z = glm::clamp(carPosition.z, -BOUND, BOUND);
}

// callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    static float lastX = 400.0f;
    static bool firstMouse = true;

    float xpos = static_cast<float>(xposIn);

    if (firstMouse)
    {
        lastX = xpos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    lastX = xpos;

    // only allow horizontal rotation (yaw), no pitch
    const float sensitivity = 0.1f;
    xoffset *= sensitivity;

    camera.Yaw += xoffset;

    // update camera Front vector manually since we're not using ProcessMouseMovement
    glm::vec3 front;
    front.x = cos(glm::radians(camera.Pitch)) * cos(glm::radians(camera.Yaw));
    front.y = sin(glm::radians(camera.Pitch));   // pitch stays fixed
    front.z = cos(glm::radians(camera.Pitch)) * sin(glm::radians(camera.Yaw));
    camera.Front = glm::normalize(front);
}


void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

// texture loader
unsigned int loadTexture(const char* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format = GL_RGB;
        if (nrComponents == 1) format = GL_RED;
        else if (nrComponents == 3) format = GL_RGB;
        else if (nrComponents == 4) format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
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

// cubemap loader (robust to 3 or 4 channels)
unsigned int loadCubemap(const std::vector<std::string>& faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            GLenum format = GL_RGB;
            if (nrChannels == 1) format = GL_RED;
            else if (nrChannels == 3) format = GL_RGB;
            else if (nrChannels == 4) format = GL_RGBA;

            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}
