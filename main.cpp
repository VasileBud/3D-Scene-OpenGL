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

glm::vec3 fogColor(0.56f, 0.59f, 0.64f);
float fogDensity = 0.00028f;
float fogStart = 1200.0f;
float fogMinFactor = 0.45f;

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

glm::vec3 lampColor1(12.0f, 9.6f, 7.2f);
glm::vec3 lampColor2(12.0f, 9.6f, 7.2f);

glm::vec3 lampPos1(2.6f, 9.9f, -30.1f);
glm::vec3 lampPos2(-2.6f, 9.9f, -30.1f);

GLint lampPos1Loc, lampColor1Loc;
GLint lampPos2Loc, lampColor2Loc;

const glm::vec3 shipWorldTranslation(10000.0f, 100.0f, -10000.0f);
const float shipWorldScale = 70.0f;
const glm::vec3 shipSpawnLocal = 0.5f * (lampPos1 + lampPos2);
const glm::vec3 shipLookLocal = shipSpawnLocal + glm::vec3(0.0f, 0.0f, -10.0f);
const glm::vec3 shipSpawnWorld = shipWorldTranslation + shipWorldScale * shipSpawnLocal;
const glm::vec3 shipLookWorld = shipWorldTranslation + shipWorldScale * shipLookLocal;

// camera
gps::Camera myCamera(
    // pornim camera direct pe corabie
    shipSpawnWorld,
    shipLookWorld,
    glm::vec3(0.0f, 1.0f, 0.0f));

GLfloat cameraSpeed = 50.0f;

float oceanAmplitude = 0.2f;
float oceanFrequency = 0.7f;
float oceanSpeed = 2.6f;

GLboolean pressedKeys[1024];

enum RenderMode { RENDER_SOLID = 0, RENDER_WIREFRAME, RENDER_POLYGONAL, RENDER_SMOOTH };
RenderMode currentRenderMode = RENDER_SOLID;

// models
gps::Model3D teapot;
gps::Model3D ocean;
gps::Model3D moon;
gps::Model3D ship;
gps::SkyBox mySkyBox;

GLfloat angle;

// shaders
gps::Shader myBasicShader;
gps::Shader oceanShader;
gps::Shader skyboxShader;
gps::Shader moonShader;

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

    view = myCamera.getViewMatrix();
    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    // update view matrix for ocean shader
    oceanShader.useShaderProgram();
    glUniformMatrix4fv(oceanViewLoc, 1, GL_FALSE, glm::value_ptr(view));
}

void processMovement()
{
    if (pressedKeys[GLFW_KEY_W])
    {
        myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        // update view matrix for ocean shader
        oceanShader.useShaderProgram();
        glUniformMatrix4fv(oceanViewLoc, 1, GL_FALSE, glm::value_ptr(view));
    }

    if (pressedKeys[GLFW_KEY_S])
    {
        myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        // update view matrix for ocean shader
        oceanShader.useShaderProgram();
        glUniformMatrix4fv(oceanViewLoc, 1, GL_FALSE, glm::value_ptr(view));
    }

    if (pressedKeys[GLFW_KEY_A])
    {
        myCamera.move(gps::MOVE_LEFT, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        // update view matrix for ocean shader
        oceanShader.useShaderProgram();
        glUniformMatrix4fv(oceanViewLoc, 1, GL_FALSE, glm::value_ptr(view));
    }

    if (pressedKeys[GLFW_KEY_D])
    {
        myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        // update view matrix for ocean shader
        oceanShader.useShaderProgram();
        glUniformMatrix4fv(oceanViewLoc, 1, GL_FALSE, glm::value_ptr(view));
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
}

void initShaders()
{
    myBasicShader.loadShader("shaders/basic.vert","shaders/basic.frag");
    oceanShader.loadShader("shaders/ocean.vert", "shaders/ocean.frag");
    moonShader.loadShader("shaders/moon.vert", "shaders/moon.frag");
    skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
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

    // ship transform is static; precompute and send lamp positions/colors once
    shipModelMatrix = glm::mat4(1.0f);
    shipModelMatrix = glm::translate(shipModelMatrix, shipWorldTranslation);
    shipModelMatrix = glm::scale(shipModelMatrix, glm::vec3(shipWorldScale));

    lampWorld1 = glm::vec3(shipModelMatrix * glm::vec4(lampPos1, 1.0f));
    lampWorld2 = glm::vec3(shipModelMatrix * glm::vec4(lampPos2, 1.0f));

    glUniform3fv(lampPos1Loc, 1, glm::value_ptr(lampWorld1));
    glUniform3fv(lampColor1Loc, 1, glm::value_ptr(lampColor1));
    glUniform3fv(lampPos2Loc, 1, glm::value_ptr(lampWorld2));
    glUniform3fv(lampColor2Loc, 1, glm::value_ptr(lampColor2));
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

void renderShip(gps::Shader shader)
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

void renderScene()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //render the scene
    renderOcean(oceanShader);

    // render the teapot
    //renderTeapot(myBasicShader);

    // render the ship
    renderShip(myBasicShader);

    // render the moon
    renderMoon(moonShader);

    mySkyBox.Draw(skyboxShader, view, projection);

}

void cleanup()
{
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
    initModels();
    initShaders();
    initSkybox();
    initUniforms();
    setWindowCallbacks();

    // application loop
    while (!glfwWindowShouldClose(myWindow.getWindow()))
    {
        processMovement();
        renderScene();

        glfwPollEvents();
        glfwSwapBuffers(myWindow.getWindow());

    }

    cleanup();

    return EXIT_SUCCESS;
}
//spawnat in corabie sus pe ea
//culoare corabie putin mai intunecata
