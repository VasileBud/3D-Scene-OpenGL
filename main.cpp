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

// ship transform and derived lamp positions
glm::mat4 shipModelMatrix;
glm::vec3 lampWorld1;
glm::vec3 lampWorld2;
float shipYaw = 0.0f;

glm::vec3 fogColor(0.56f, 0.59f, 0.64f);
float fogDensity = 0.00028f;
float fogStart = 1200.0f;
float fogMinFactor = 0.45f;
const float oceanSurfaceY = -5.0f;
const float introMinOceanClearance = 60.0f;

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
GLint oceanAmplitudeLoc;
GLint oceanFrequencyLoc;
GLint oceanSpeedLoc;
GLint oceanLightDirLoc;
GLint oceanLightColorLoc;
GLint oceanFogColorLoc;
GLint oceanFogDensityLoc;
GLint oceanFogStartLoc;
GLint oceanFogMinFactorLoc;
GLint moonModelLoc;
GLint moonNormalMatrixLoc;
GLint shipModelLoc;
GLint shipViewLoc;
GLint shipProjectionLoc;
GLint shipNormalMatrixLoc;
GLint shipLightDirLoc;
GLint shipLightColorLoc;
GLint fogColorLoc;
GLint fogDensityLoc;
GLint fogStartLoc;
GLint fogMinFactorLoc;
GLint lightSpaceTrMatrixLoc;
GLint shadowMapLoc;
GLint oceanLightSpaceTrMatrixLoc;
GLint oceanShadowMapLoc;

glm::vec3 lampColor1(12.0f, 9.6f, 7.2f);
glm::vec3 lampColor2(12.0f, 9.6f, 7.2f);

glm::vec3 lampPos1(2.6f, 9.9f, -30.1f);
glm::vec3 lampPos2(-2.6f, 9.9f, -30.1f);

GLint lampPos1Loc, lampColor1Loc;
GLint lampPos2Loc, lampColor2Loc;

const glm::vec3 shipWorldTranslation(10000.0f, 100.0f, -10000.0f);
const float shipWorldScale = 90.0f;
const glm::vec3 shipSpawnLocal = 0.5f * (lampPos1 + lampPos2);
const glm::vec3 shipLookLocal = shipSpawnLocal + glm::vec3(-20.0f, 0.0f, -10.0f);
const glm::vec3 shipSpawnWorld = shipWorldTranslation + shipWorldScale * shipSpawnLocal;
const glm::vec3 shipLookWorld = shipWorldTranslation + shipWorldScale * shipLookLocal;
const glm::vec3 introStartPos(0.0f, 6500.0f, 9500.0f);
const glm::vec3 introStartTarget(0.0f, 0.0f, 0.0f);

// camera
gps::Camera myCamera(
    introStartPos,
    introStartTarget,
    glm::vec3(0.0f, 1.0f, 0.0f));

GLfloat cameraSpeed = 35.0f;

struct IntroKeyframe
{
    float t;
    glm::vec3 position;
    glm::vec3 target;
};

const float introDuration = 22.0f;
float introStartTime = 0.0f;
bool introActive = true;
bool resetMouseState = false;
std::array<IntroKeyframe, 9> introKeyframes;
glm::vec3 shipIntroSpawnWorld = shipSpawnWorld;

float oceanAmplitude = 0.2f;
float oceanFrequency = 0.7f;
float oceanSpeed = 2.6f;

GLboolean pressedKeys[1024];

enum RenderMode { RENDER_SOLID = 0, RENDER_WIREFRAME, RENDER_POLYGONAL, RENDER_SMOOTH };
RenderMode currentRenderMode = RENDER_SOLID;

const GLuint SHADOW_WIDTH = 2048*4;
const GLuint SHADOW_HEIGHT = 2048*4;
GLuint shadowMapFBO;
GLuint depthMapTexture;
glm::mat4 lightSpaceTrMatrix;

// models
gps::Model3D teapot;
gps::Model3D ocean;
gps::Model3D moon;
gps::Model3D ship;
gps::SkyBox mySkyBox;
gps::Model3D::AABB shipBoundsLocal;

const float shipWalkMargin = 1.5f;
float shipEyeHeightLocal = 1.7f;
float shipFloorDefaultLocal = 0.0f;

GLfloat angle;

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

void applyRenderMode()
{
    switch (currentRenderMode)
    {
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
    shipModelMatrix = glm::translate(shipModelMatrix, shipWorldTranslation);
    shipModelMatrix = glm::rotate(shipModelMatrix, glm::radians(shipYaw), glm::vec3(0.0f, 1.0f, 0.0f));
    shipModelMatrix = glm::scale(shipModelMatrix, glm::vec3(shipWorldScale));

    lampWorld1 = glm::vec3(shipModelMatrix * glm::vec4(lampPos1, 1.0f));
    lampWorld2 = glm::vec3(shipModelMatrix * glm::vec4(lampPos2, 1.0f));
}

glm::vec3 catmullRom(const glm::vec3& p0,
                     const glm::vec3& p1,
                     const glm::vec3& p2,
                     const glm::vec3& p3,
                     float t)
{
    float t2 = t * t;
    float t3 = t2 * t;
    return 0.5f * ((2.0f * p1) +
                   (-p0 + p2) * t +
                   (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
                   (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
}

void setCameraLookAt(const glm::vec3& position, const glm::vec3& target)
{
    glm::vec3 dir = glm::normalize(target - position);
    float yaw = glm::degrees(atan2(dir.z, dir.x));
    float pitch = glm::degrees(asin(glm::clamp(dir.y, -1.0f, 1.0f)));
    myCamera.setPosition(position);
    myCamera.rotate(yaw, pitch);
}

void initIntroAnimation()
{
    const glm::vec3 wideTarget(0.0f, 0.0f, 0.0f);
    const glm::vec3 oceanTarget(0.0f, 0.0f, -15.0f);
    const glm::vec3 oceanHoverPos(0.0f, 140.0f, 500.0f);
    introKeyframes[0] = { 0.0f, introStartPos, wideTarget };
    introKeyframes[1] = { 2.5f, glm::vec3(0.0f, 2200.0f, 3500.0f), oceanTarget };
    introKeyframes[2] = { 4.5f, oceanHoverPos, oceanTarget };
    introKeyframes[3] = { 5.5f, oceanHoverPos, oceanTarget };

    glm::vec3 shipCenter = shipWorldTranslation + glm::vec3(0.0f, 300.0f, 0.0f);
    const float orbitRadius = 3500.0f;
    const float orbitHeight = 1500.0f;

    introKeyframes[4] = { 10.5f, shipWorldTranslation + glm::vec3(-6000.0f, 2200.0f, 2000.0f), shipCenter };
    introKeyframes[5] = { 14.0f, shipCenter + glm::vec3(orbitRadius, orbitHeight, 0.0f), shipCenter };
    introKeyframes[6] = { 17.0f, shipCenter + glm::vec3(0.0f, orbitHeight + 200.0f, orbitRadius), shipCenter };
    introKeyframes[7] = { 20.0f, shipCenter + glm::vec3(-orbitRadius, orbitHeight, 0.0f), shipCenter };

    introKeyframes[8] = { introDuration, shipIntroSpawnWorld, shipLookWorld };
}

void updateIntroCamera()
{
    if (!introActive)
    {
        shipYaw = 0.0f;
        return;
    }

    float elapsed = static_cast<float>(glfwGetTime()) - introStartTime;
    if (elapsed >= introDuration)
    {
        introActive = false;
        shipYaw = 0.0f;
        setCameraLookAt(shipIntroSpawnWorld, shipLookWorld);
        updateViewUniforms();
        resetMouseState = true;
        return;
    }

    float normalized = glm::clamp(elapsed / introDuration, 0.0f, 1.0f);
    shipYaw = 360.0f * normalized;

    float oceanHoldStart = introKeyframes[2].t;
    float oceanHoldEnd = introKeyframes[3].t;
    if (elapsed >= oceanHoldStart && elapsed <= oceanHoldEnd)
    {
        glm::vec3 pos = introKeyframes[2].position;
        glm::vec3 target = introKeyframes[2].target;
        float minY = oceanSurfaceY + introMinOceanClearance;
        if (pos.y < minY)
        {
            pos.y = minY;
        }
        if (target.y < oceanSurfaceY + 2.0f)
        {
            target.y = oceanSurfaceY + 2.0f;
        }
        setCameraLookAt(pos, target);
        updateViewUniforms();
        return;
    }

    size_t idx = 0;
    while (idx + 1 < introKeyframes.size() && elapsed > introKeyframes[idx + 1].t)
    {
        ++idx;
    }

    const IntroKeyframe& a = introKeyframes[idx];
    const IntroKeyframe& b = introKeyframes[idx + 1];
    float t = (elapsed - a.t) / (b.t - a.t);
    t = glm::clamp(t, 0.0f, 1.0f);
    float s = t * t * (3.0f - 2.0f * t);

    size_t i0 = (idx == 0) ? idx : idx - 1;
    size_t i1 = idx;
    size_t i2 = idx + 1;
    size_t i3 = (idx + 2 < introKeyframes.size()) ? idx + 2 : idx + 1;

    glm::vec3 pos = catmullRom(introKeyframes[i0].position,
                               introKeyframes[i1].position,
                               introKeyframes[i2].position,
                               introKeyframes[i3].position,
                               s);
    glm::vec3 target = catmullRom(introKeyframes[i0].target,
                                  introKeyframes[i1].target,
                                  introKeyframes[i2].target,
                                  introKeyframes[i3].target,
                                  s);
    float minY = oceanSurfaceY + introMinOceanClearance;
    if (pos.y < minY)
    {
        pos.y = minY;
    }
    if (target.y < oceanSurfaceY + 2.0f)
    {
        target.y = oceanSurfaceY + 2.0f;
    }
    setCameraLookAt(pos, target);
    updateViewUniforms();
}

glm::vec3 findFreeSpawnLocal()
{
    glm::vec3 minB = shipBoundsLocal.min + glm::vec3(shipWalkMargin, 0.0f, shipWalkMargin);
    glm::vec3 maxB = shipBoundsLocal.max - glm::vec3(shipWalkMargin, 0.0f, shipWalkMargin);

    auto tryPosition = [&](float x, float z, glm::vec3& outLocal) -> bool
    {
        if (x < minB.x || x > maxB.x || z < minB.z || z > maxB.z)
        {
            return false;
        }

        float floorHeight = 0.0f;
        if (!ship.getHeightAt(x, z, shipSpawnLocal.y, floorHeight))
        {
            return false;
        }

        outLocal = glm::vec3(x, floorHeight + shipEyeHeightLocal, z);
        return true;
    };

    glm::vec3 result;
    const float baseX = 0.5f * (shipBoundsLocal.min.x + shipBoundsLocal.max.x);
    const float baseZ = 0.5f * (shipBoundsLocal.min.z + shipBoundsLocal.max.z);

    if (tryPosition(baseX, baseZ, result))
    {
        return result;
    }

    const float step = 1.0f;
    const int maxRing = 25;
    for (int r = 1; r <= maxRing; ++r)
    {
        for (int dx = -r; dx <= r; ++dx)
        {
            if (tryPosition(baseX + dx * step, baseZ + r * step, result) ||
                tryPosition(baseX + dx * step, baseZ - r * step, result))
            {
                return result;
            }
        }

        for (int dz = -r + 1; dz <= r - 1; ++dz)
        {
            if (tryPosition(baseX + r * step, baseZ + dz * step, result) ||
                tryPosition(baseX - r * step, baseZ + dz * step, result))
            {
                return result;
            }
        }
    }

    return glm::vec3(shipSpawnLocal.x, shipSpawnLocal.y, shipSpawnLocal.z);
}

void placeCameraOnShip()
{
    glm::vec3 localPos = findFreeSpawnLocal();
    glm::vec3 worldPos = glm::vec3(shipModelMatrix * glm::vec4(localPos, 1.0f));
    setCameraLookAt(worldPos, shipLookWorld);
    updateViewUniforms();
    resetMouseState = true;
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
    const GLfloat ortho_size = 8000.0f;
    glm::mat4 lightProjection = glm::ortho(-ortho_size, ortho_size, -ortho_size, ortho_size,
                                           near_plane, far_plane);
    return lightProjection * lightView;
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
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
        clampCameraToShip(prevPos);
        updateViewUniforms();
    }

    if (pressedKeys[GLFW_KEY_Q])
    {
        angle -= 1.0f;
        // update model matrix for teapot
        model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
        // update normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }

    if (pressedKeys[GLFW_KEY_E])
    {
        angle += 1.0f;
        // update model matrix for teapot
        model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
        // update normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
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

void initOpenGLState()
{
    glClearColor(0.5, 0.5, 0.5, 1.0);
    glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_DEPTH_TEST); // enable depth-testing
    glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
    glEnable(GL_CULL_FACE); // cull face
    glCullFace(GL_BACK); // cull back face
    glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
    applyRenderMode();
}

void initModels()
{
    teapot.LoadModel("models/teapot/teapot20segUT.obj");
    ocean.LoadModel("models/ocean/ocean.obj");
    moon.LoadModel("models/moon/moon.obj");
    ship.LoadModel("models/ship/ship_v1_03.obj");
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

    glm::vec3 introLocal = findFreeSpawnLocal();
    shipIntroSpawnWorld = shipWorldTranslation + shipWorldScale * introLocal;
}

void initShaders()
{
    myBasicShader.loadShader("shaders/basic.vert","shaders/basic.frag");
    oceanShader.loadShader("shaders/ocean.vert", "shaders/ocean.frag");
    moonShader.loadShader("shaders/moon.vert", "shaders/moon.frag");
    skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
    depthShader.loadShader("shaders/depthShader.vert", "shaders/depthShader.frag");
    skyboxShader.useShaderProgram();
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

    //set light color
    lightColor = glm::vec3(0.12f, 0.14f, 0.20f);
    lightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor");
    // send light color to shader
    glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

    fogColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "fogColor");
    fogDensityLoc = glGetUniformLocation(myBasicShader.shaderProgram, "fogDensity");
    fogMinFactorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "fogMinFactor");
    glUniform3fv(fogColorLoc, 1, glm::value_ptr(fogColor));
    glUniform1f(fogDensityLoc, fogDensity);
    fogStartLoc = glGetUniformLocation(myBasicShader.shaderProgram, "fogStart");
    glUniform1f(fogMinFactorLoc, fogMinFactor);
    glUniform1f(fogStartLoc, fogStart);

    oceanShader.useShaderProgram();

    // create model matrix for ocean
    oceanModel = glm::mat4(1.0f);
    oceanModel = glm::translate(oceanModel, glm::vec3(0.0f, -5.0f, -15.0f));
    oceanModel = glm::scale(oceanModel, glm::vec3(60.0f)); 

    // get uniform locations for ocean shader
    oceanModelLoc = glGetUniformLocation(oceanShader.shaderProgram, "model");
    oceanNormalMatrixLoc = glGetUniformLocation(oceanShader.shaderProgram, "normalMatrix");
    oceanTimeLoc = glGetUniformLocation(oceanShader.shaderProgram, "time");
    oceanAmplitudeLoc = glGetUniformLocation(oceanShader.shaderProgram, "amplitude");
    oceanFrequencyLoc = glGetUniformLocation(oceanShader.shaderProgram, "frequency");
    oceanSpeedLoc = glGetUniformLocation(oceanShader.shaderProgram, "speed");
    oceanViewLoc = glGetUniformLocation(oceanShader.shaderProgram, "view");
    oceanProjectionLoc = glGetUniformLocation(oceanShader.shaderProgram, "projection");
    oceanLightDirLoc = glGetUniformLocation(oceanShader.shaderProgram, "lightDir");
    oceanLightColorLoc = glGetUniformLocation(oceanShader.shaderProgram, "lightColor");
    oceanFogColorLoc = glGetUniformLocation(oceanShader.shaderProgram, "fogColor");
    oceanFogDensityLoc = glGetUniformLocation(oceanShader.shaderProgram, "fogDensity");
    oceanFogStartLoc = glGetUniformLocation(oceanShader.shaderProgram, "fogStart");
    oceanFogMinFactorLoc = glGetUniformLocation(oceanShader.shaderProgram, "fogMinFactor");

    // send view and projection matrices to ocean shader
    glUniformMatrix4fv(oceanViewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(oceanProjectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // send light parameters to ocean shader
    glUniform3fv(oceanLightDirLoc, 1, glm::value_ptr(lightDir));
    glUniform3fv(oceanLightColorLoc, 1, glm::value_ptr(lightColor));
    glUniform3fv(oceanFogColorLoc, 1, glm::value_ptr(fogColor));
    glUniform1f(oceanFogDensityLoc, fogDensity);
    glUniform1f(oceanFogStartLoc, fogStart);
    glUniform1f(oceanFogMinFactorLoc, fogMinFactor);

    // send wave parameters to ocean shader
    glUniform1f(oceanAmplitudeLoc, oceanAmplitude);
    glUniform1f(oceanFrequencyLoc, oceanFrequency);
    glUniform1f(oceanSpeedLoc, oceanSpeed);

    skyboxShader.useShaderProgram();

    GLint hazeColorLoc = glGetUniformLocation(skyboxShader.shaderProgram, "hazeColor");
    GLint hazeStrLoc = glGetUniformLocation(skyboxShader.shaderProgram, "hazeStrength");
    GLint hazeHLoc = glGetUniformLocation(skyboxShader.shaderProgram, "hazeHeight");
    GLint skyTintLoc = glGetUniformLocation(skyboxShader.shaderProgram, "skyTint");
    GLint skyTintStrLoc = glGetUniformLocation(skyboxShader.shaderProgram, "skyTintStrength");
    GLint skyLightDirLoc = glGetUniformLocation(skyboxShader.shaderProgram, "lightDir");

    if (hazeColorLoc != -1)
        glUniform3f(hazeColorLoc,  0.55f, 0.58f, 0.63f); // mai aproape de fog, mai multa ceata pe orizont
    if (hazeStrLoc != -1)
        glUniform1f(hazeStrLoc, 0.55f);
    if (hazeHLoc != -1)
        glUniform1f(hazeHLoc, 0.65f);
    if (skyTintLoc != -1)
        glUniform3f(skyTintLoc,   0.003f, 0.006f, 0.015f);
    if (skyTintStrLoc != -1)
        glUniform1f(skyTintStrLoc, 0.75f);
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

    lampPos1Loc   = glGetUniformLocation(myBasicShader.shaderProgram, "lampPos1");
    lampColor1Loc = glGetUniformLocation(myBasicShader.shaderProgram, "lampColor1");

    lampPos2Loc   = glGetUniformLocation(myBasicShader.shaderProgram, "lampPos2");
    lampColor2Loc = glGetUniformLocation(myBasicShader.shaderProgram, "lampColor2");

    // initialize ship transform and lamp positions/colors
    updateShipTransform();

    glUniform3fv(lampPos1Loc, 1, glm::value_ptr(lampWorld1));
    glUniform3fv(lampColor1Loc, 1, glm::value_ptr(lampColor1));
    glUniform3fv(lampPos2Loc, 1, glm::value_ptr(lampWorld2));
    glUniform3fv(lampColor2Loc, 1, glm::value_ptr(lampColor2));

    lightSpaceTrMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightSpaceTrMatrix");
    shadowMapLoc = glGetUniformLocation(myBasicShader.shaderProgram, "shadowMap");
    glUniform1i(shadowMapLoc, 5);

    oceanShader.useShaderProgram();
    oceanLightSpaceTrMatrixLoc = glGetUniformLocation(oceanShader.shaderProgram, "lightSpaceTrMatrix");
    oceanShadowMapLoc = glGetUniformLocation(oceanShader.shaderProgram, "shadowMap");
    glUniform1i(oceanShadowMapLoc, 5);
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

void renderTeapot(gps::Shader shader)
{
    // select active shader program
    shader.useShaderProgram();

    //send teapot model matrix data to shader
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    //send teapot normal matrix data to shader
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    // draw teapot
    teapot.Draw(shader);
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
}

void renderShip(gps::Shader shader)
{
    shader.useShaderProgram();

    glUniform3fv(lampPos1Loc, 1, glm::value_ptr(lampWorld1));
    glUniform3fv(lampColor1Loc, 1, glm::value_ptr(lampColor1));
    glUniform3fv(lampPos2Loc, 1, glm::value_ptr(lampWorld2));
    glUniform3fv(lampColor2Loc, 1, glm::value_ptr(lampColor2));

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

void renderScene()
{
    lightSpaceTrMatrix = computeLightSpaceTrMatrix();
    updateShipTransform();

    // render to depth map
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

    // render the scene
    glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);

    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(lightSpaceTrMatrixLoc, 1, GL_FALSE, glm::value_ptr(lightSpaceTrMatrix));

    oceanShader.useShaderProgram();
    glUniformMatrix4fv(oceanLightSpaceTrMatrixLoc, 1, GL_FALSE, glm::value_ptr(lightSpaceTrMatrix));

    renderOcean(oceanShader);

    // render the ship
    renderShip(myBasicShader);

    // render the moon
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
    initIntroAnimation();
    initShaders();
    initSkybox();
    initUniforms();
    setWindowCallbacks();
    introStartTime = static_cast<float>(glfwGetTime());

    // application loop
    while (!glfwWindowShouldClose(myWindow.getWindow()))
    {
        updateIntroCamera();
        processMovement();
        renderScene();

        glfwPollEvents();
        glfwSwapBuffers(myWindow.getWindow());

    }

    cleanup();

    return EXIT_SUCCESS;
}
