// main_animation_only.cpp
// Modified for third-person shooter camera control
// Mouse rotates character, camera follows behind

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/animator.h>
#include <learnopengl/model_animation.h>

#include <iostream>

struct Bullet {
    glm::vec3 position;
    glm::vec3 direction;
    float speed;
    float life;
};

std::vector<Bullet> bullets;
const float BULLET_SPEED = 15.0f;
const float BULLET_LIFETIME = 3.0f;

struct Target {
    glm::vec3 position;
    float speed;
};

std::vector<Target> targets;
const float TARGET_SPEED = 1.2f;
const float SPAWN_INTERVAL = 3.0f;
float timeSinceLastSpawn = 0.0f;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void updateCamera();

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 1.2f, 4.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// player (character)
glm::vec3 characterPosition = glm::vec3(0.0f, 0.09f, 0.0f);
float characterYaw = 0.0f; // rotation of the player model
float cameraYaw = 0.0f;    // horizontal orbit angle around the player
float cameraPitch = 0.0f;
glm::vec3 characterScale = glm::vec3(0.5f);
const float CHARACTER_SPEED = 2.5f; // units/sec
const float MOUSE_SENSITIVITY = 0.1f;
const float CAMERA_DISTANCE = 3.0f; // distance behind character
const float CAMERA_HEIGHT = 1.5f;   // height above character
const float HIT_DISTANCE = 0.3f; // adjust for bullet + target size

// animation state
Animation* idleAnimPtr = nullptr;
Animation* runForwardPtr = nullptr;
Animation* runBackPtr = nullptr;
Animation* runLeftPtr = nullptr;
Animation* runRightPtr = nullptr;
Animation* runForwardLeftPtr = nullptr;
Animation* runForwardRightPtr = nullptr;
Animation* runBackLeftPtr = nullptr;
Animation* runBackRightPtr = nullptr;
Animation* currentAnimPtr = nullptr;
Animator* animatorPtr = nullptr;

unsigned int cubeVAO = 0, cubeVBO = 0;

float cubeVertices[] = {
    // A simple 1x1x1 cube (36 vertices)
    -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f, 0.5f,-0.5f,
     0.5f, 0.5f,-0.5f, -0.5f, 0.5f,-0.5f, -0.5f,-0.5f,-0.5f,
    -0.5f,-0.5f, 0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f,
     0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f,-0.5f, 0.5f,
    -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f, -0.5f,-0.5f,-0.5f,
    -0.5f,-0.5f,-0.5f, -0.5f,-0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
     0.5f, 0.5f, 0.5f,  0.5f, 0.5f,-0.5f,  0.5f,-0.5f,-0.5f,
     0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f,
    -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f,
     0.5f,-0.5f, 0.5f, -0.5f,-0.5f, 0.5f, -0.5f,-0.5f,-0.5f,
    -0.5f, 0.5f,-0.5f,  0.5f, 0.5f,-0.5f,  0.5f, 0.5f, 0.5f,
     0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f
};

void initCube() {
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);

    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);
}

int main()
{
    // glfw init + callbacks
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Third-Person Character Control", NULL, NULL);
    if (!window) { std::cout << "Failed to create window\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { std::cout << "Failed GLAD\n"; return -1; }
    stbi_set_flip_vertically_on_load(true);
    glEnable(GL_DEPTH_TEST);

    // shaders
    Shader skinnedShader("anim_model.vs", "anim_model.fs");
    Shader platformShader("single_color.vs", "single_color.fs");

    // load model + animations
    Model ourModel(FileSystem::getPath("resources/objects/gun/rifle.dae"));
    Animation idleAnim(FileSystem::getPath("resources/objects/gun/rifle_idle.dae"), &ourModel);
    Animation runForwardAnim(FileSystem::getPath("resources/objects/gun/run_forward.dae"), &ourModel);
    Animation runBackAnim(FileSystem::getPath("resources/objects/gun/run_back.dae"), &ourModel);
    Animation runLeftAnim(FileSystem::getPath("resources/objects/gun/run_left.dae"), &ourModel);
    Animation runRightAnim(FileSystem::getPath("resources/objects/gun/run_right.dae"), &ourModel);
    Animation runForwardLeftAnim(FileSystem::getPath("resources/objects/gun/run_forward_left.dae"), &ourModel);
    Animation runForwardRightAnim(FileSystem::getPath("resources/objects/gun/run_forward_right.dae"), &ourModel);
    Animation runBackLeftAnim(FileSystem::getPath("resources/objects/gun/run_back_left.dae"), &ourModel);
    Animation runBackRightAnim(FileSystem::getPath("resources/objects/gun/run_back_right.dae"), &ourModel);

    idleAnimPtr = &idleAnim;
    runForwardPtr = &runForwardAnim;
    runBackPtr = &runBackAnim;
    runLeftPtr = &runLeftAnim;
    runRightPtr = &runRightAnim;
    runForwardLeftPtr = &runForwardLeftAnim;
    runForwardRightPtr = &runForwardRightAnim;
    runBackLeftPtr = &runBackLeftAnim;
    runBackRightPtr = &runBackRightAnim;

    Animator animator(&idleAnim);
    animatorPtr = &animator;
    animator.PlayAnimation(idleAnimPtr);
    currentAnimPtr = idleAnimPtr;

    initCube();

    // render loop
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);
        updateCamera();
        animator.UpdateAnimation(deltaTime);

        // --- Target spawn logic ---
        timeSinceLastSpawn += deltaTime;
        if (timeSinceLastSpawn >= SPAWN_INTERVAL)
        {
            timeSinceLastSpawn = 0.0f;

            // spawn target far from player
            float range = 4.0f; // outer spawn range
            glm::vec3 pos;
            do {
                pos = glm::vec3(
                    (rand() % 100 / 100.0f - 0.5f) * 2.0f * range,
                    0.1f,
                    (rand() % 100 / 100.0f - 0.5f) * 2.0f * range
                );
            } while (glm::length(pos - characterPosition) < 2.5f); // not too close

            Target t;
            t.position = pos;
            t.speed = TARGET_SPEED;
            targets.push_back(t);
        }

        // Update bullets
        for (int i = 0; i < bullets.size(); )
        {
            bullets[i].position += bullets[i].direction * bullets[i].speed * deltaTime;
            bullets[i].life -= deltaTime;

            if (bullets[i].life <= 0.0f)
                bullets.erase(bullets.begin() + i);
            else
                ++i;
        }

        // --- Update targets (move toward player) ---
        for (auto& t : targets)
        {
            glm::vec3 dir = glm::normalize(characterPosition - t.position);
            t.position += dir * t.speed * deltaTime;
        }

        // --- Bullet-target collision detection ---
        for (int i = 0; i < bullets.size(); )
        {
            bool bulletRemoved = false;

            for (int j = 0; j < targets.size(); )
            {
                glm::vec3 targetCenter = targets[j].position + glm::vec3(0.0f, 0.75f, 0.0f); // half of Y-scale
                float dist = glm::length(bullets[i].position - targetCenter);

                if (dist < HIT_DISTANCE)
                {
                    bullets.erase(bullets.begin() + i);
                    targets.erase(targets.begin() + j);
                    bulletRemoved = true;
                    break;
                }
                else
                {
                    ++j;
                }
            }

            if (!bulletRemoved)
                ++i;
        }


        // render
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();

        skinnedShader.use();
        skinnedShader.setMat4("projection", projection);
        skinnedShader.setMat4("view", view);

        auto transforms = animator.GetFinalBoneMatrices();
        for (int i = 0; i < (int)transforms.size(); ++i)
            skinnedShader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", transforms[i]);

        // model transform
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, characterPosition);
        model = glm::rotate(model, glm::radians(characterYaw + 180.0f), glm::vec3(0, 1, 0));
        model = glm::scale(model, characterScale);
        skinnedShader.setMat4("model", model);

        ourModel.Draw(skinnedShader);

        platformShader.use();
        platformShader.setMat4("projection", projection);
        platformShader.setMat4("view", view);

        // platform
        platformShader.setVec3("color", glm::vec3(0.4f, 0.4f, 0.4f));
        glm::mat4 m = glm::mat4(1.0f);
        m = glm::scale(m, glm::vec3(10.0f, 0.2f, 10.0f));
        platformShader.setMat4("model", m);
        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // walls
        platformShader.setVec3("color", glm::vec3(0.2f, 0.2f, 0.2f));

        // back wall
        m = glm::mat4(1.0f);
        m = glm::translate(m, glm::vec3(0.0f, 1.0f, -5.0f));
        m = glm::scale(m, glm::vec3(10.0f, 2.0f, 0.2f));
        platformShader.setMat4("model", m);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // front
        m = glm::mat4(1.0f);
        m = glm::translate(m, glm::vec3(0.0f, 1.0f, 5.0f));
        m = glm::scale(m, glm::vec3(10.0f, 2.0f, 0.2f));
        platformShader.setMat4("model", m);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // left
        m = glm::mat4(1.0f);
        m = glm::translate(m, glm::vec3(-5.0f, 1.0f, 0.0f));
        m = glm::scale(m, glm::vec3(0.2f, 2.0f, 10.0f));
        platformShader.setMat4("model", m);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // right
        m = glm::mat4(1.0f);
        m = glm::translate(m, glm::vec3(5.0f, 1.0f, 0.0f));
        m = glm::scale(m, glm::vec3(0.2f, 2.0f, 10.0f));
        platformShader.setMat4("model", m);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        platformShader.use();
        platformShader.setMat4("projection", projection);
        platformShader.setMat4("view", view);
        platformShader.setVec3("color", glm::vec3(1.0f, 0.8f, 0.2f)); // yellowish

        for (auto& bullet : bullets)
        {
            glm::mat4 m = glm::mat4(1.0f);
            m = glm::translate(m, bullet.position);
            m = glm::scale(m, glm::vec3(0.06f)); // small bullet
            platformShader.setMat4("model", m);
            glBindVertexArray(cubeVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // --- Draw targets ---
        platformShader.use();
        platformShader.setVec3("color", glm::vec3(0.9f, 0.1f, 0.1f)); // red enemies

        for (auto& t : targets)
        {
            glm::mat4 m = glm::mat4(1.0f);
            m = glm::translate(m, t.position);
            m = glm::scale(m, glm::vec3(0.3f, 1.5f, 0.3f)); // target size
            platformShader.setMat4("model", m);
            glBindVertexArray(cubeVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

// Update camera position to follow character
void updateCamera()
{
    characterYaw = cameraYaw;

    // Calculate camera position behind character
    float yawRad = glm::radians(cameraYaw);
    float pitchRad = glm::radians(cameraPitch);

    // Offset from character (behind and up)
    glm::vec3 offset;
    offset.x = CAMERA_DISTANCE * sin(yawRad) * cos(pitchRad);
    offset.y = CAMERA_HEIGHT + CAMERA_DISTANCE * sin(pitchRad);
    offset.z = CAMERA_DISTANCE * cos(yawRad) * cos(pitchRad);

    camera.Position = characterPosition + offset;

    // Make camera look at character (slightly above center)
    glm::vec3 lookAtPoint = characterPosition + glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 direction = glm::normalize(lookAtPoint - camera.Position);

    // Update camera's front vector
    camera.Front = direction;
    camera.Right = glm::normalize(glm::cross(camera.Front, glm::vec3(0.0f, 1.0f, 0.0f)));
    camera.Up = glm::normalize(glm::cross(camera.Right, camera.Front));
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float yawRad = glm::radians(cameraYaw);
    glm::vec3 camForward = glm::normalize(glm::vec3(-sin(yawRad), 0.0f, -cos(yawRad)));
    glm::vec3 camRight = glm::normalize(glm::vec3(cos(yawRad), 0.0f, -sin(yawRad)));

    glm::vec3 moveDir(0.0f);

    bool w = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
    bool s = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
    bool a = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
    bool d = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;

    if (w) moveDir += camForward;
    if (s) moveDir -= camForward;
    if (a) moveDir -= camRight;
    if (d) moveDir += camRight;

    bool moving = glm::length(moveDir) > 0.01f;
    if (moving)
    {
        moveDir = glm::normalize(moveDir);
        characterPosition += moveDir * CHARACTER_SPEED * deltaTime;
    }

    // Keep within area
    float limit = 4.5f;
    characterPosition.x = glm::clamp(characterPosition.x, -limit, limit);
    characterPosition.z = glm::clamp(characterPosition.z, -limit, limit);

    // Pick the right animation
    Animation* newAnim = idleAnimPtr;
    if (moving)
    {
        if (w && a && !s && !d)
            newAnim = runForwardLeftPtr;
        else if (w && d && !s && !a)
            newAnim = runForwardRightPtr;
        else if (s && a && !w && !d)
            newAnim = runBackLeftPtr;
        else if (s && d && !w && !a)
            newAnim = runBackRightPtr;
        else if (w && !a && !s && !d)
            newAnim = runForwardPtr;
        else if (s && !a && !w && !d)
            newAnim = runBackPtr;
        else if (a && !w && !s && !d)
            newAnim = runLeftPtr;
        else if (d && !w && !s && !a)
            newAnim = runRightPtr;
        else
            newAnim = runForwardPtr; // fallback
    }

    // Switch animation only if changed
    if (newAnim != currentAnimPtr)
    {
        currentAnimPtr = newAnim;
        animatorPtr->PlayAnimation(newAnim);
    }

    // Shooting (press J or Left Mouse)
    static bool shootPressedLastFrame = false;
    bool jPressed = glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS;
    bool mousePressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    bool shootPressed = jPressed || mousePressed;

    if (shootPressed && !shootPressedLastFrame)
    {
        // Aim where the camera looks
        glm::vec3 forward = glm::normalize(glm::vec3(camera.Front.x, camera.Front.y, camera.Front.z));

        Bullet bullet;
        bullet.position = characterPosition + glm::vec3(-0.1f, 0.8f, 0.0f);
        bullet.direction = forward;
        bullet.speed = BULLET_SPEED;
        bullet.life = BULLET_LIFETIME;
        bullets.push_back(bullet);
    }

    shootPressedLastFrame = shootPressed;

}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = ypos - lastY;
    lastX = xpos;
    lastY = ypos;

    xoffset *= MOUSE_SENSITIVITY;
    yoffset *= MOUSE_SENSITIVITY;

    // Rotate camera (not character)
    characterYaw -= xoffset;
    cameraYaw -= xoffset;
    cameraPitch += yoffset;

    if (cameraPitch > 45.0f)
        cameraPitch = 45.0f;
    if (cameraPitch < -45.0f)
        cameraPitch = -45.0f;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}