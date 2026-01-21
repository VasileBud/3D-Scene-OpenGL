#if defined (__APPLE__)
#define GLFW_INCLUDE_GLCOREARB
#define GL_SILENCE_DEPRECATION
#else
#define GLEW_STATIC
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#include <glm/glm.hpp> //core glm functionality
#include <glm/gtc/matrix_transform.hpp> //glm extension for generating common transformation matrices
#include <glm/gtc/matrix_inverse.hpp> //glm extension for computing inverse matrices
#include <glm/gtc/type_ptr.hpp> //glm extension for accessing the internal data structure of glm types

#include "Window.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"
#include "SkyBox.hpp"

#include <iostream>
#include <array>
#include <cmath>

// window
gps::Window myWindow;

// matrices
glm::mat4 model;
glm::mat4 view;
glm::mat4 projection;
glm::mat3 normalMatrix;
glm::mat4 oceanModel;

// light parameters
glm::vec3 lightDir;
glm::vec3 lightColor;

// shader uniform locations
GLint modelLoc;
GLint viewLoc;
GLint projectionLoc;
GLint normalMatrixLoc;
GLint lightDirLoc;
GLint lightColorLoc;
GLint oceanTimeLoc;
GLint oceanViewLoc;
GLint oceanProjectionLoc;
GLint oceanModelLoc;
GLint oceanNormalMatrixLoc;
GLint oceanLightDirLoc;
GLint oceanLightColorLoc;
GLint moonModelLoc;
GLint moonNormalMatrixLoc;
GLint shipModelLoc;
GLint shipViewLoc;
GLint shipProjectionLoc;
GLint shipNormalMatrixLoc;
GLint shipLightDirLoc;
GLint shipLightColorLoc;
GLint lightSpaceTrMatrixLoc;
GLint shadowMapLoc;
GLint oceanLightSpaceTrMatrixLoc;
GLint oceanShadowMapLoc;
GLint shipWorldMatrixLoc;

// ship transform
glm::mat4 shipModelMatrix;
float shipYaw = 0.0f;

// camera
gps::Camera myCamera(
    glm::vec3(10000.0f, 991.0f, -12709.0f),
    glm::vec3(8200.0f, 991.0f, -13609.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));

GLfloat cameraSpeed = 35.0f;

struct IntroKeyframe
{
    float t;
    glm::vec3 position;
    glm::vec3 target;
};

const float introDuration = 13.0f;
float introStartTime = 0.0f;
bool introActive = true;
bool resetMouseState = false;
std::array<IntroKeyframe, 9> introKeyframes;
glm::vec3 shipIntroSpawnWorld(0.0f);
bool collisionsEnabled = true;

GLboolean pressedKeys[1024];

// render mode
enum RenderMode { RENDER_SOLID = 0, RENDER_WIREFRAME, RENDER_POLYGONAL, RENDER_SMOOTH };
RenderMode currentRenderMode = RENDER_SOLID;

// shadow mapping
const GLuint SHADOW_WIDTH = 2048 * 4;
const GLuint SHADOW_HEIGHT = 2048 * 4;
GLuint shadowMapFBO;
GLuint depthMapTexture;
glm::mat4 lightSpaceTrMatrix;

// models
gps::Model3D teapot;
gps::Model3D nanosuit;
gps::Model3D chest;
gps::Model3D ocean;
gps::Model3D moon;
gps::Model3D ship;
gps::SkyBox mySkyBox;
gps::Model3D::AABB shipBoundsLocal;

const float shipWalkMargin = 1.5f;
float shipEyeHeightLocal = 1.7f;
float shipFloorDefaultLocal = 0.0f;

GLfloat angle;

// pickup state
enum HeldItem { HELD_NONE = 0, HELD_TEAPOT, HELD_NANOSUIT };
HeldItem heldItem = HELD_NONE;
glm::vec3 teapotWorldPos;
glm::vec3 nanosuitWorldPos;
glm::vec3 chestWorldPos;
// shaders
gps::Shader myBasicShader;
gps::Shader oceanShader;
gps::Shader skyboxShader;
gps::Shader moonShader;
gps::Shader depthShader;

GLenum glCheckError_(const char* file, int line)
{
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR)
    {
        std::string error;
        switch (errorCode)
        {
        case GL_INVALID_ENUM:
            error = "INVALID_ENUM";
            break;
        case GL_INVALID_VALUE:
            error = "INVALID_VALUE";
            break;
        case GL_INVALID_OPERATION:
            error = "INVALID_OPERATION";
            break;
        case GL_OUT_OF_MEMORY:
            error = "OUT_OF_MEMORY";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            error = "INVALID_FRAMEBUFFER_OPERATION";
            break;
        }
        std::cout << error << " | " << file << " (" << line << ")" << std::endl;
    }
    return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void applyRenderMode(){
    switch (currentRenderMode){
        case RENDER_SOLID:
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glDisable(GL_LINE_SMOOTH);
            glDisable(GL_POINT_SMOOTH);
            glDisable(GL_POLYGON_SMOOTH);
            glDisable(GL_BLEND);
            break;
        case RENDER_WIREFRAME:
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDisable(GL_POINT_SMOOTH);
            glDisable(GL_POLYGON_SMOOTH);
            glEnable(GL_LINE_SMOOTH);
            glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
            glLineWidth(1.5f);
            glDisable(GL_BLEND);
            break;
        case RENDER_POLYGONAL:
            glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
            glDisable(GL_LINE_SMOOTH);
            glDisable(GL_POLYGON_SMOOTH);
            glEnable(GL_POINT_SMOOTH);
            glPointSize(3.0f);
            glDisable(GL_BLEND);
            break;
        case RENDER_SMOOTH:
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glDisable(GL_LINE_SMOOTH);
            glDisable(GL_POINT_SMOOTH);
            glEnable(GL_POLYGON_SMOOTH);
            glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            break;
    }
}

void updateViewUniforms()
{
    view = myCamera.getViewMatrix();
    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    oceanShader.useShaderProgram();
    glUniformMatrix4fv(oceanViewLoc, 1, GL_FALSE, glm::value_ptr(view));
}

void updateShipTransform()
{
    shipModelMatrix = glm::mat4(1.0f);
    const glm::vec3 shipWorldTranslation(10000.0f, 100.0f, -10000.0f);
    const float shipWorldScale = 90.0f;
    shipModelMatrix = glm::translate(shipModelMatrix, shipWorldTranslation);
    shipModelMatrix = glm::rotate(shipModelMatrix, glm::radians(shipYaw), glm::vec3(0.0f, 1.0f, 0.0f));
    shipModelMatrix = glm::scale(shipModelMatrix, glm::vec3(shipWorldScale));
}


glm::vec3 getHoldWorldPosition()
{
    glm::vec3 front = glm::normalize(myCamera.getCameraFrontDirection());
    return myCamera.getPosition() + front * 250.0f + glm::vec3(0.0f, 0.0f, 0.0f);
}

glm::mat4 buildWorldMatrix(const glm::vec3& worldPos, float localScale, const glm::vec3& rotationDegrees)
{
    glm::mat4 model = glm::translate(glm::mat4(1.0f), worldPos);
    if (rotationDegrees.x != 0.0f)
    {
        model = model * glm::rotate(glm::mat4(1.0f),
                                    glm::radians(rotationDegrees.x),
                                    glm::vec3(1.0f, 0.0f, 0.0f));
    }
    if (rotationDegrees.y != 0.0f)
    {
        model = model * glm::rotate(glm::mat4(1.0f),
                                    glm::radians(rotationDegrees.y),
                                    glm::vec3(0.0f, 1.0f, 0.0f));
    }
    if (rotationDegrees.z != 0.0f)
    {
        model = model * glm::rotate(glm::mat4(1.0f),
                                    glm::radians(rotationDegrees.z),
                                    glm::vec3(0.0f, 0.0f, 1.0f));
    }
    const float shipWorldScale = 90.0f;
    model = model * glm::scale(glm::mat4(1.0f), glm::vec3(localScale * shipWorldScale));
    return model;
}

glm::mat4 buildHeldMatrix(float localScale, const glm::vec3& rotationDegrees)
{
    glm::mat4 model = glm::translate(glm::mat4(1.0f), getHoldWorldPosition());
    if (rotationDegrees.x != 0.0f)
    {
        model = model * glm::rotate(glm::mat4(1.0f),
                                    glm::radians(rotationDegrees.x),
                                    glm::vec3(1.0f, 0.0f, 0.0f));
    }
    if (rotationDegrees.y != 0.0f)
    {
        model = model * glm::rotate(glm::mat4(1.0f),
                                    glm::radians(rotationDegrees.y),
                                    glm::vec3(0.0f, 1.0f, 0.0f));
    }
    if (rotationDegrees.z != 0.0f)
    {
        model = model * glm::rotate(glm::mat4(1.0f),
                                    glm::radians(rotationDegrees.z),
                                    glm::vec3(0.0f, 0.0f, 1.0f));
    }
    const float shipWorldScale = 90.0f;
    model = model * glm::scale(glm::mat4(1.0f), glm::vec3(localScale * shipWorldScale));
    return model;
}

void setCameraLookAt(const glm::vec3& position, const glm::vec3& target)
{
    glm::vec3 dir = glm::normalize(target - position);
    float yaw = glm::degrees(atan2(dir.z, dir.x));
    float pitch = glm::degrees(asin(glm::clamp(dir.y, -1.0f, 1.0f)));
    myCamera.setPosition(position);
    myCamera.rotate(yaw, pitch);
}

void intro()
{
    if (!introActive)
    {
        shipYaw = 0.0f;
        return;
    }

    const glm::vec3 shipWorldTranslation(10000.0f, 100.0f, -10000.0f);
    const float shipWorldScale = 90.0f;
    const glm::vec3 shipSpawnLocal(0.0f, 9.9f, -30.1f);
    const glm::vec3 shipLookLocal = shipSpawnLocal + glm::vec3(-20.0f, 0.0f, -10.0f);
    const glm::vec3 shipLocalCenter = 0.5f * (shipBoundsLocal.min + shipBoundsLocal.max);
    const glm::vec3 shipCenter = shipWorldTranslation + shipWorldScale * shipLocalCenter;
    const glm::vec3 shipLookWorld = shipWorldTranslation + shipWorldScale * shipLookLocal;

    float elapsed = static_cast<float>(glfwGetTime()) - introStartTime;
    if (elapsed >= introDuration)
    {
        introActive = false;
        shipYaw = 0.0f;
        updateViewUniforms();
        resetMouseState = true;
        return;
    }

    shipYaw = 0.0f;
    const float oceanHold = 2.0f;
    const float riseTime = 2.0f;
    const float landTime = 2.0f;
    const float orbitTime = introDuration - (oceanHold + riseTime + landTime);

    auto smoothstep = [](float x) {
        return x * x * (3.0f - 2.0f * x);
    };

    glm::vec3 shipSize = shipBoundsLocal.max - shipBoundsLocal.min;
    float baseRadius = glm::max(shipSize.x, shipSize.z) * shipWorldScale * 0.6f;
    baseRadius = glm::max(baseRadius, 1800.0f);

    float radiusStart = baseRadius * 1.1f;
    float radiusEnd = baseRadius * 0.75f;
    float heightStart = shipCenter.y + 260.0f;
    float heightEnd = shipCenter.y + 90.0f;
    const float oceanHeight = -5.0f + 110.0f;

    const float startAngle = 0.0f;
    glm::vec3 seaStart(shipCenter.x + std::cos(startAngle) * radiusStart * 0.55f,
                       oceanHeight,
                       shipCenter.z + std::sin(startAngle) * radiusStart * 0.55f);
    glm::vec3 orbitStart(shipCenter.x + std::cos(startAngle) * radiusStart,
                         heightStart,
                         shipCenter.z + std::sin(startAngle) * radiusStart);

    if (elapsed < oceanHold)
    {
        glm::vec3 oceanTarget = seaStart + glm::vec3(0.0f, 0.0f, -3000.0f);
        setCameraLookAt(seaStart, oceanTarget);
        updateViewUniforms();
        return;
    }

    if (elapsed < oceanHold + riseTime)
    {
        float t = glm::clamp((elapsed - oceanHold) / riseTime, 0.0f, 1.0f);
        float ease = smoothstep(t);
        glm::vec3 pos = glm::mix(seaStart, orbitStart, ease);
        glm::vec3 oceanTarget = seaStart + glm::vec3(0.0f, 0.0f, -3000.0f);
        glm::vec3 target = glm::mix(oceanTarget, shipCenter, ease);
        setCameraLookAt(pos, target);
        updateViewUniforms();
        return;
    }

    float elapsedOrbit = elapsed - oceanHold - riseTime;
    if (elapsedOrbit < orbitTime)
    {
        float t = glm::clamp(elapsedOrbit / orbitTime, 0.0f, 1.0f);
        float ease = smoothstep(t);
        float angle = 6.283185f * ease;
        float radius = glm::mix(radiusStart, radiusEnd, ease);
        float height = glm::mix(heightStart, heightEnd, ease);

        glm::vec3 pos(shipCenter.x + std::cos(angle) * radius,
                      height,
                      shipCenter.z + std::sin(angle) * radius);
        glm::vec3 lookTarget = shipCenter + glm::vec3(0.0f, -160.0f, 0.0f);
        setCameraLookAt(pos, lookTarget);
        updateViewUniforms();
        return;
    }

    float t = glm::clamp((elapsedOrbit - orbitTime) / landTime, 0.0f, 1.0f);
    float ease = smoothstep(t);

    float angle = 6.283185f;
    glm::vec3 orbitEnd(shipCenter.x + std::cos(angle) * radiusEnd,
                       heightEnd,
                       shipCenter.z + std::sin(angle) * radiusEnd);
    glm::vec3 deckPos = chestWorldPos + glm::vec3(160.0f, 50.0f, 140.0f);
    glm::vec3 pos = glm::mix(orbitEnd, deckPos, ease);
    glm::vec3 target = glm::mix(shipCenter, shipLookWorld, ease);
    setCameraLookAt(pos, target);
    updateViewUniforms();
}

void togglePickup()
{
    if (introActive)
    {
        return;
    }

    if (heldItem != HELD_NONE)
    {
        glm::vec3 holdPos = getHoldWorldPosition();
        glm::mat4 invShip = glm::inverse(shipModelMatrix);
        glm::vec3 local = glm::vec3(invShip * glm::vec4(holdPos, 1.0f));

        glm::vec3 minB = shipBoundsLocal.min + glm::vec3(shipWalkMargin, 0.0f, shipWalkMargin);
        glm::vec3 maxB = shipBoundsLocal.max - glm::vec3(shipWalkMargin, 0.0f, shipWalkMargin);
        local.x = glm::clamp(local.x, minB.x, maxB.x);
        local.z = glm::clamp(local.z, minB.z, maxB.z);

        float floorHeight = shipFloorDefaultLocal;
        if (ship.getHeightAt(local.x, local.z, local.y, floorHeight))
        {
            float dropLift = 0.15f;
            if (heldItem == HELD_TEAPOT)
            {
                local.y = floorHeight + dropLift;
                teapotWorldPos = glm::vec3(shipModelMatrix * glm::vec4(local, 1.0f));
            }
            else if (heldItem == HELD_NANOSUIT)
            {
                local.y = floorHeight + dropLift;
                nanosuitWorldPos = glm::vec3(shipModelMatrix * glm::vec4(local, 1.0f));
            }
            heldItem = HELD_NONE;
        }

        return;
    }

    glm::vec3 camPos = myCamera.getPosition();
    float distTeapot = glm::length(teapotWorldPos - camPos);
    float distNanosuit = glm::length(nanosuitWorldPos - camPos);

    if (distTeapot <= 350.0f && distTeapot <= distNanosuit)
    {
        heldItem = HELD_TEAPOT;
    }
    else if (distNanosuit <= 350.0f)
    {
        heldItem = HELD_NANOSUIT;
    }
}

void clampCameraToShip(const glm::vec3& prevWorldPos)
{
    glm::vec3 pos = myCamera.getPosition();
    glm::mat4 invShip = glm::inverse(shipModelMatrix);
    glm::vec3 localPos = glm::vec3(invShip * glm::vec4(pos, 1.0f));

    glm::vec3 minB = shipBoundsLocal.min + glm::vec3(shipWalkMargin, 0.0f, shipWalkMargin);
    glm::vec3 maxB = shipBoundsLocal.max - glm::vec3(shipWalkMargin, 0.0f, shipWalkMargin);

    localPos.x = glm::clamp(localPos.x, minB.x, maxB.x);
    localPos.z = glm::clamp(localPos.z, minB.z, maxB.z);
    float floorHeight = 0.0f;
    if (!ship.getHeightAt(localPos.x, localPos.z, localPos.y, floorHeight))
    {
        myCamera.setPosition(prevWorldPos);
        return;
    }

    localPos.y = floorHeight + shipEyeHeightLocal;

    glm::vec3 clampedWorld = glm::vec3(shipModelMatrix * glm::vec4(localPos, 1.0f));
    myCamera.setPosition(clampedWorld);
}

glm::mat4 computeLightSpaceTrMatrix()
{
    const glm::vec3 sceneCenter = myCamera.getPosition();
    const GLfloat lightDistance = 10000.0f;
    glm::vec3 lightPos = sceneCenter + lightDir * lightDistance;
    glm::mat4 lightView = glm::lookAt(lightPos, sceneCenter, glm::vec3(0.0f, 1.0f, 0.0f));
    const GLfloat near_plane = 1.0f, far_plane = 20000.0f;
    const GLfloat ortho_size = 3000.0f;
    glm::mat4 lightProjection = glm::ortho(-ortho_size, ortho_size, -ortho_size, ortho_size,
                                           near_plane, far_plane);
    return lightProjection * lightView;
}

void windowResizeCallback(GLFWwindow* window, int width, int height)
{
    fprintf(stdout, "Window resized! New width: %d , and height: %d\n", width, height);
    myWindow.setWindowDimensions({width, height});
    glViewport(0, 0, width, height);
    projection = glm::perspective(glm::radians(45.0f),
                                  (float)width / (float)height,
                                  0.1f, 2000.0f);

    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
    // update projection matrix for ocean shader
    oceanShader.useShaderProgram();
    glUniformMatrix4fv(oceanProjectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

    if (key >= 0 && key < 1024)
    {
        if (action == GLFW_PRESS)
        {
            pressedKeys[key] = true;
        }
        else if (action == GLFW_RELEASE)
        {
            pressedKeys[key] = false;
        }
    }

    if (key == GLFW_KEY_M && action == GLFW_PRESS)
    {
        currentRenderMode = static_cast<RenderMode>((currentRenderMode + 1) % 4);
        applyRenderMode();
    }

    if (key == GLFW_KEY_F && action == GLFW_PRESS)
    {
        togglePickup();
    }

    if (key == GLFW_KEY_C && action == GLFW_PRESS)
    {
        collisionsEnabled = !collisionsEnabled;
    }
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    static bool initializedAngles = false;
    static bool firstMouse = true;
    static float lastX = 0.0f;
    static float lastY = 0.0f;
    static float cameraYaw = 0.0f;
    static float cameraPitch = 0.0f;

    if (resetMouseState)
    {
        initializedAngles = false;
        firstMouse = true;
        resetMouseState = false;
    }

    if (introActive)
    {
        return;
    }

    if (!initializedAngles)
    {
        glm::vec3 initialFrontDirection = myCamera.getCameraFrontDirection();
        cameraYaw = glm::degrees(atan2(initialFrontDirection.z, initialFrontDirection.x));
        cameraPitch = glm::degrees(asin(initialFrontDirection.y));
        initializedAngles = true;
    }

    if (firstMouse)
    {
        lastX = static_cast<float>(xpos);
        lastY = static_cast<float>(ypos);
        firstMouse = false;
    }

    float xoffset = static_cast<float>(xpos) - lastX;
    float yoffset = lastY - static_cast<float>(ypos);
    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);

    const float sensitivity = 0.05f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    cameraYaw += xoffset;
    cameraPitch += yoffset;

    if (cameraPitch > 89.0f) cameraPitch = 89.0f;
    if (cameraPitch < -89.0f) cameraPitch = -89.0f;
    myCamera.rotate(cameraYaw, cameraPitch);
    updateViewUniforms();
}

void processMovement()
{
    if (introActive)
    {
        return;
    }

    bool moved = false;
    glm::vec3 prevPos = myCamera.getPosition();
    if (pressedKeys[GLFW_KEY_W])
    {
        myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
        moved = true;
    }

    if (pressedKeys[GLFW_KEY_S])
    {
        myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
        moved = true;
    }

    if (pressedKeys[GLFW_KEY_A])
    {
        myCamera.move(gps::MOVE_LEFT, cameraSpeed);
        moved = true;
    }

    if (pressedKeys[GLFW_KEY_D])
    {
        myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
        moved = true;
    }

    if (moved)
    {
        if (collisionsEnabled)
        {
            clampCameraToShip(prevPos);
        }
        updateViewUniforms();
    }
}

void initOpenGLWindow()
{
    myWindow.Create(1024, 768, "OpenGL Project Core");
}

void setWindowCallbacks()
{
    glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
    glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
    glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);
    glfwSetInputMode(myWindow.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void initOpenGLState(){
    glClearColor(0.56f, 0.59f, 0.64f, 1.0f);
    glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_DEPTH_TEST); // enable depth-testing
    glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
    glEnable(GL_CULL_FACE); // cull face
    glCullFace(GL_BACK); // cull back face
    glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
    applyRenderMode();
}

void initShadowMap()
{
    glGenFramebuffers(1, &shadowMapFBO);
    glGenTextures(1, &depthMapTexture);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                 SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void initModels()
{
    teapot.LoadModel("models/teapot/teapot20segUT.obj");
    nanosuit.LoadModel("models/nanosuit/nanosuit.obj");
    chest.LoadModel("models/chest/treasure_chest.obj");
    ocean.LoadModel("models/ocean/ocean.obj");
    moon.LoadModel("models/moon/moon.obj");
    ship.LoadModel("models/ship/ship_v1_03.obj");
}

void initObjectPositions()
{
    const glm::vec3 shipWorldTranslation(10000.0f, 100.0f, -10000.0f);
    const float shipWorldScale = 90.0f;
    const glm::vec3 shipSpawnLocal(0.0f, 9.9f, -30.1f);

    shipBoundsLocal = ship.getBounds();
    float spawnFloor = shipBoundsLocal.min.y;
    if (ship.getHeightAt(shipSpawnLocal.x, shipSpawnLocal.z, shipSpawnLocal.y, spawnFloor))
    {
        shipFloorDefaultLocal = spawnFloor;
    }
    else
    {
        shipFloorDefaultLocal = shipBoundsLocal.min.y;
    }
    shipEyeHeightLocal = shipSpawnLocal.y - shipFloorDefaultLocal;
    if (shipEyeHeightLocal < 0.5f)
    {
        shipEyeHeightLocal = 1.7f;
    }

    shipIntroSpawnWorld = shipWorldTranslation + shipWorldScale * shipSpawnLocal;

    const glm::vec3 teapotLocal(-2.2f, 6.0f, -10.4f);
    const glm::vec3 nanosuitLocal(3.2f, 5.5f, -6.8f);
    const glm::vec3 chestLocal(0.2f, 6.0f, -10.4f);

    teapotWorldPos = shipWorldTranslation + shipWorldScale * teapotLocal;
    nanosuitWorldPos = shipWorldTranslation + shipWorldScale * nanosuitLocal;
    chestWorldPos = shipWorldTranslation + shipWorldScale * chestLocal;
}

void initShaders()
{
    myBasicShader.loadShader("shaders/basic.vert", "shaders/basic.frag");
    oceanShader.loadShader("shaders/ocean.vert", "shaders/ocean.frag");
    moonShader.loadShader("shaders/moon.vert", "shaders/moon.frag");
    skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
    depthShader.loadShader("shaders/depthShader.vert", "shaders/depthShader.frag");
    skyboxShader.useShaderProgram();
}

void initSkybox()
{
    std::vector<const GLchar*> faces;
    faces.push_back("skybox/xpos.png"); // +X (right)
    faces.push_back("skybox/xneg.png"); // -X (left)
    faces.push_back("skybox/ypos.png"); // +Y (top)
    faces.push_back("skybox/yneg.png"); // -Y (bottom)
    faces.push_back("skybox/zpos.png"); // +Z (front)
    faces.push_back("skybox/zneg.png"); // -Z (back)
    mySkyBox.Load(faces);
}

void initUniforms()
{
    myBasicShader.useShaderProgram();

    // create model matrix for teapot
    model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
    modelLoc = glGetUniformLocation(myBasicShader.shaderProgram, "model");

    // get view matrix for current camera
    view = myCamera.getViewMatrix();
    viewLoc = glGetUniformLocation(myBasicShader.shaderProgram, "view");
    // send view matrix to shader
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // compute normal matrix for teapot
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    normalMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "normalMatrix");

    // create projection matrix
    projection = glm::perspective(glm::radians(45.0f),
                                  (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().
                                  height,
                                  0.1f, 10000.0f);

    projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
    // send projection matrix to shader
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    //set the light direction (direction towards the light)
    lightDir = glm::normalize(glm::vec3(-0.2f, 1.0f, -0.3f)); // "de sus" ca luna
    lightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir");
    // send light dir to shader
    glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));

    // set light color
    lightColor = glm::vec3(0.12f, 0.14f, 0.20f);
    lightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor");
    glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

    oceanShader.useShaderProgram();

    // create model matrix for ocean
    oceanModel = glm::mat4(1.0f);
    oceanModel = glm::translate(oceanModel, glm::vec3(0.0f, -5.0f, -15.0f));
    oceanModel = glm::scale(oceanModel, glm::vec3(60.0f));

    // get uniform locations for ocean shader
    oceanModelLoc = glGetUniformLocation(oceanShader.shaderProgram, "model");
    oceanNormalMatrixLoc = glGetUniformLocation(oceanShader.shaderProgram, "normalMatrix");
    oceanTimeLoc = glGetUniformLocation(oceanShader.shaderProgram, "time");
    oceanViewLoc = glGetUniformLocation(oceanShader.shaderProgram, "view");
    oceanProjectionLoc = glGetUniformLocation(oceanShader.shaderProgram, "projection");
    oceanLightDirLoc = glGetUniformLocation(oceanShader.shaderProgram, "lightDir");
    oceanLightColorLoc = glGetUniformLocation(oceanShader.shaderProgram, "lightColor");

    // send view and projection matrices to ocean shader
    glUniformMatrix4fv(oceanViewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(oceanProjectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // send light parameters to ocean shader
    glUniform3fv(oceanLightDirLoc, 1, glm::value_ptr(lightDir));
    glUniform3fv(oceanLightColorLoc, 1, glm::value_ptr(lightColor));

    skyboxShader.useShaderProgram();

    GLint skyLightDirLoc = glGetUniformLocation(skyboxShader.shaderProgram, "lightDir");

    if (skyLightDirLoc != -1)
        glUniform3fv(skyLightDirLoc, 1, glm::value_ptr(lightDir));

    moonShader.useShaderProgram();

    glm::vec3 moonWorldPos = glm::vec3(-7000.0f, 5000.0f, -2000.0f);
    float moonScale = 100.0f;

    glm::mat4 moonModel = glm::mat4(1.0f);
    moonModel = glm::translate(moonModel, moonWorldPos);
    moonModel = glm::scale(moonModel, glm::vec3(moonScale));

    GLint moonModelLoc = glGetUniformLocation(moonShader.shaderProgram, "model");
    glUniformMatrix4fv(moonModelLoc, 1, GL_FALSE, glm::value_ptr(moonModel));

    glm::mat3 moonNormalMatrix =
        glm::mat3(glm::inverseTranspose(view * moonModel));

    GLint moonNormalLoc = glGetUniformLocation(moonShader.shaderProgram, "normalMatrix");
    glUniformMatrix3fv(moonNormalLoc, 1, GL_FALSE, glm::value_ptr(moonNormalMatrix));

    myBasicShader.useShaderProgram();

    // Initialize ship uniforms
    shipModelLoc = glGetUniformLocation(myBasicShader.shaderProgram, "model");
    shipViewLoc = glGetUniformLocation(myBasicShader.shaderProgram, "view");
    shipProjectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
    shipNormalMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "normalMatrix");
    shipLightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir");
    shipLightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor");

    shipWorldMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "shipModelMatrix");

    // initialize ship transform and lamp positions/colors
    updateShipTransform();

    glUniformMatrix4fv(shipWorldMatrixLoc, 1, GL_FALSE, glm::value_ptr(shipModelMatrix));

    lightSpaceTrMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightSpaceTrMatrix");
    shadowMapLoc = glGetUniformLocation(myBasicShader.shaderProgram, "shadowMap");
    glUniform1i(shadowMapLoc, 5);

    oceanShader.useShaderProgram();
    oceanLightSpaceTrMatrixLoc = glGetUniformLocation(oceanShader.shaderProgram, "lightSpaceTrMatrix");
    oceanShadowMapLoc = glGetUniformLocation(oceanShader.shaderProgram, "shadowMap");
    glUniform1i(oceanShadowMapLoc, 5);
}

void renderOcean(gps::Shader shader)
{
    // select active shader program
    shader.useShaderProgram();

    // update time uniform for wave animation
    glUniform1f(oceanTimeLoc, (float)glfwGetTime());

    // single patch (foloseste oceanModel din initUniforms)
    glUniformMatrix4fv(oceanModelLoc, 1, GL_FALSE, glm::value_ptr(oceanModel));

    glm::mat3 oceanNormalMatrix = glm::mat3(glm::inverseTranspose(view * oceanModel));
    glUniformMatrix3fv(oceanNormalMatrixLoc, 1, GL_FALSE, glm::value_ptr(oceanNormalMatrix));

    ocean.Draw(shader);
}

void renderMoon(gps::Shader& shader)
{
    shader.useShaderProgram();

    GLint viewLoc = glGetUniformLocation(shader.shaderProgram, "view");
    GLint projLoc = glGetUniformLocation(shader.shaderProgram, "projection");

    // fix moon on cer
    glm::mat4 viewNoTranslate = glm::mat4(glm::mat3(view));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewNoTranslate));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    moon.Draw(shader);
}

void renderSceneDepth()
{
    depthShader.useShaderProgram();
    GLint modelLoc = glGetUniformLocation(depthShader.shaderProgram, "model");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(oceanModel));
    ocean.Draw(depthShader);

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(shipModelMatrix));
    ship.Draw(depthShader);

    glm::mat4 teapotMatrix = (heldItem == HELD_TEAPOT)
        ? buildHeldMatrix(0.18f, glm::vec3(0.0f))
        : buildWorldMatrix(teapotWorldPos, 0.18f, glm::vec3(0.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(teapotMatrix));
    teapot.Draw(depthShader);

    glm::mat4 nanosuitMatrix = (heldItem == HELD_NANOSUIT)
        ? buildHeldMatrix(0.22f, glm::vec3(90.0f, 0.0f, 0.0f))
        : buildWorldMatrix(nanosuitWorldPos, 0.22f, glm::vec3(90.0f, 180.0f, 0.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(nanosuitMatrix));
    nanosuit.Draw(depthShader);

    glm::mat4 chestMatrix = buildWorldMatrix(chestWorldPos, 0.2f, glm::vec3(0.0f, 270.0f, 0.0f));
    glDisable(GL_CULL_FACE);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(chestMatrix));
    chest.Draw(depthShader);
    glEnable(GL_CULL_FACE);
}

void renderShip(gps::Shader& shader)
{
    shader.useShaderProgram();

    // Send ship model matrix
    glUniformMatrix4fv(shipModelLoc, 1, GL_FALSE, glm::value_ptr(shipModelMatrix));

    // Send view and projection matrices
    glUniformMatrix4fv(shipViewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(shipProjectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Compute and send normal matrix
    glm::mat3 shipNormalMatrix = glm::mat3(glm::inverseTranspose(view * shipModelMatrix));
    glUniformMatrix3fv(shipNormalMatrixLoc, 1, GL_FALSE, glm::value_ptr(shipNormalMatrix));

    // Send light parameters
    glUniform3fv(shipLightDirLoc, 1, glm::value_ptr(lightDir));
    glUniform3fv(shipLightColorLoc, 1, glm::value_ptr(lightColor));

    ship.Draw(shader);
}

void renderTeapot(gps::Shader& shader)
{
    glm::mat4 teapotMatrix = (heldItem == HELD_TEAPOT)
                                 ? buildHeldMatrix(0.18f, glm::vec3(0.0f))
                                 : buildWorldMatrix(teapotWorldPos, 0.18f, glm::vec3(0.0f));
    shader.useShaderProgram();

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(teapotMatrix));
    glm::mat3 objectNormalMatrix = glm::mat3(glm::inverseTranspose(view * teapotMatrix));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(objectNormalMatrix));
    teapot.Draw(shader);
}

void renderNanosuit(gps::Shader& shader)
{
    glm::mat4 nanosuitMatrix = (heldItem == HELD_NANOSUIT)
                                   ? buildHeldMatrix(0.22f, glm::vec3(90.0f, 0.0f, 0.0f))
                                   : buildWorldMatrix(nanosuitWorldPos, 0.22f, glm::vec3(90.0f, 180.0f, 0.0f));
    shader.useShaderProgram();
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(nanosuitMatrix));
    glm::mat3 objectNormalMatrix = glm::mat3(glm::inverseTranspose(view * nanosuitMatrix));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(objectNormalMatrix));
    nanosuit.Draw(shader);
}

void renderChest(gps::Shader& shader)
{
    glm::mat4 chestMatrix = buildWorldMatrix(chestWorldPos, 0.2f, glm::vec3(0.0f, 270.0f, 0.0f));

    shader.useShaderProgram();
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(chestMatrix));
    glm::mat3 objectNormalMatrix = glm::mat3(glm::inverseTranspose(view * chestMatrix));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(objectNormalMatrix));

    glDisable(GL_CULL_FACE);
    chest.Draw(shader);
    glEnable(GL_CULL_FACE);
}

void renderDepthMapPass()
{
    lightSpaceTrMatrix = computeLightSpaceTrMatrix();
    updateShipTransform();
    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(shipWorldMatrixLoc, 1, GL_FALSE, glm::value_ptr(shipModelMatrix));

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    depthShader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(depthShader.shaderProgram, "lightSpaceTrMatrix"),
                       1, GL_FALSE, glm::value_ptr(lightSpaceTrMatrix));
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    renderSceneDepth();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    applyRenderMode();
}

void renderScenePass()
{
    glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);

    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(lightSpaceTrMatrixLoc, 1, GL_FALSE, glm::value_ptr(lightSpaceTrMatrix));

    oceanShader.useShaderProgram();
    glUniformMatrix4fv(oceanLightSpaceTrMatrixLoc, 1, GL_FALSE, glm::value_ptr(lightSpaceTrMatrix));
}

void renderScene()
{
    //render the scene
    renderDepthMapPass();
    renderScenePass();

    renderOcean(oceanShader);
    renderShip(myBasicShader);
    renderTeapot(myBasicShader);
    renderNanosuit(myBasicShader);
    renderChest(myBasicShader);
    renderMoon(moonShader);

    mySkyBox.Draw(skyboxShader, view, projection);
}

void cleanup()
{
    glDeleteFramebuffers(1, &shadowMapFBO);
    glDeleteTextures(1, &depthMapTexture);
    myWindow.Delete();
    //cleanup code for your own data
}

int main(int argc, const char* argv[])
{
    try
    {
        initOpenGLWindow();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    initOpenGLState();
    initShadowMap();
    initModels();
    initObjectPositions();
    initShaders();
    initSkybox();
    initUniforms();
    setWindowCallbacks();
    introActive = true;
    shipYaw = 0.0f;
    introStartTime = static_cast<float>(glfwGetTime());
    resetMouseState = true;

    // application loop
    while (!glfwWindowShouldClose(myWindow.getWindow()))
    {
        intro();
        processMovement();
        renderScene();

        glfwPollEvents();
        glfwSwapBuffers(myWindow.getWindow());
    }

    cleanup();

    return EXIT_SUCCESS;
}
