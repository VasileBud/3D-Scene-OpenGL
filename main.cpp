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
GLint moonModelLoc;
GLint moonNormalMatrixLoc;
GLint shipModelLoc;
GLint shipViewLoc;
GLint shipProjectionLoc;
GLint shipNormalMatrixLoc;
GLint shipLightDirLoc;
GLint shipLightColorLoc;

// camera
gps::Camera myCamera(
    glm::vec3(0.0f, 0.0f, 3.0f),
    glm::vec3(0.0f, 0.0f, -10.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));

GLfloat cameraSpeed = 5.0f;

float oceanAmplitude = 0.2f;
float oceanFrequency = 0.7f;
float oceanSpeed = 2.6f;

GLboolean pressedKeys[1024];

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
    glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
    glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_DEPTH_TEST); // enable depth-testing
    glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
    glEnable(GL_CULL_FACE); // cull face
    glCullFace(GL_BACK); // cull back face
    glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
}

void initModels()
{
    teapot.LoadModel("models/teapot/teapot20segUT.obj");
    ocean.LoadModel("models/ocean/ocean.obj");
    moon.LoadModel("models/moon/moon.obj");
    ship.LoadModel("models/ship/Golden_Galleon_0418173919_texture.obj");
}

void initShaders()
{
    myBasicShader.loadShader(
        "shaders/basic.vert",
        "shaders/basic.frag");
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

    oceanShader.useShaderProgram();

    // create model matrix for ocean
    oceanModel = glm::mat4(1.0f);
    oceanModel = glm::translate(oceanModel, glm::vec3(0.0f, -5.0f, -15.0f));
    oceanModel = glm::scale(oceanModel, glm::vec3(10.0f));

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

    // send view and projection matrices to ocean shader
    glUniformMatrix4fv(oceanViewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(oceanProjectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // send light parameters to ocean shader
    glUniform3fv(oceanLightDirLoc, 1, glm::value_ptr(lightDir));
    glUniform3fv(oceanLightColorLoc, 1, glm::value_ptr(lightColor));

    // send wave parameters to ocean shader
    glUniform1f(oceanAmplitudeLoc, oceanAmplitude);
    glUniform1f(oceanFrequencyLoc, oceanFrequency);
    glUniform1f(oceanSpeedLoc, oceanSpeed);

    skyboxShader.useShaderProgram();

    GLint hazeColorLoc = glGetUniformLocation(skyboxShader.shaderProgram, "hazeColor");
    GLint hazeStrLoc = glGetUniformLocation(skyboxShader.shaderProgram, "hazeStrength");
    GLint hazeHLoc = glGetUniformLocation(skyboxShader.shaderProgram, "hazeHeight");

    if (hazeColorLoc != -1)
        glUniform3f(hazeColorLoc,  0.003f, 0.006f, 0.015f);
    if (hazeStrLoc != -1)
        glUniform1f(hazeStrLoc, 0.75f);
    if (hazeHLoc != -1)
        glUniform1f(hazeHLoc, 1.5f);

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

    // send ocean model matrix data to shader
    glUniformMatrix4fv(oceanModelLoc, 1, GL_FALSE, glm::value_ptr(oceanModel));

    // compute and send ocean normal matrix data to shader
    glm::mat3 oceanNormalMatrix = glm::mat3(glm::inverseTranspose(view * oceanModel));
    glUniformMatrix3fv(oceanNormalMatrixLoc, 1, GL_FALSE, glm::value_ptr(oceanNormalMatrix));

    //update time uniform for wave animation
    glUniform1f(oceanTimeLoc, (float)glfwGetTime());

    ocean.Draw(shader);
}

void renderMoon(gps::Shader& shader)
{
    shader.useShaderProgram();

    GLint viewLoc = glGetUniformLocation(shader.shaderProgram, "view");
    GLint projLoc = glGetUniformLocation(shader.shaderProgram, "projection");

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    moon.Draw(shader);
}

void renderShip(gps::Shader shader)
{
    shader.useShaderProgram();

    // Create ship model matrix
    glm::mat4 shipModel = glm::mat4(1.0f);
    shipModel = glm::translate(shipModel, glm::vec3(0.0f, 800.0f, -15.0f));
    shipModel = glm::scale(shipModel, glm::vec3(1000.0f));

    // Send ship model matrix
    glUniformMatrix4fv(shipModelLoc, 1, GL_FALSE, glm::value_ptr(shipModel));

    // Send view and projection matrices
    glUniformMatrix4fv(shipViewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(shipProjectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Compute and send normal matrix
    glm::mat3 shipNormalMatrix = glm::mat3(glm::inverseTranspose(view * shipModel));
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

    glCheckError();
    // application loop
    while (!glfwWindowShouldClose(myWindow.getWindow()))
    {
        processMovement();
        renderScene();

        glfwPollEvents();
        glfwSwapBuffers(myWindow.getWindow());

        glCheckError();
    }

    cleanup();

    return EXIT_SUCCESS;
}
