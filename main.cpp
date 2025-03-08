#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <math.h>

#include "setup.hpp"

#include <vector>
#include <chrono>

const float G = 6.6743E-11;

struct vec3 {
    float x, y, z;

    vec3 (float x = 0.0f, float y = 0.0f, float z = 0.0f) : x(x), y(y), z(z) {}

    vec3 operator+(const vec3& other) const {
        return vec3(this->x + other.x, this->y + other.y);
    }
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}


class Sphere {
    public:
    glm::vec3 pos;
    glm::vec3 vel;
    glm::vec3 acc;

    float radius;
    float mass;

    Sphere(glm::vec3 pos, glm::vec3 vel, float radius, long double mass) : pos(pos), vel(vel), radius(radius), mass(mass) {
        acc = glm::vec3(0.f, 0.f, 0.f);
    }

    void applyForce(glm::vec3 force) {
        acc += force / mass;
    }

    void updatePos(float dt) {
        //glm::mat4 transformation = glm::mat4(1.f); // Identity
        //transformation = glm::translate(transformation, vel);
        //pos = trans * pos;
        
        //glm::vec4 transformedPos = transformation * glm::vec4(pos, 1.0f);
        //pos = glm::vec3(transformedPos);
        vel = vel + acc * dt;
        pos = pos + vel * dt;

        // Collision detection (Window range: -1 to 1)
        if (pos.x + radius > 1.0f || pos.x - radius < -1.0f){
            vel.x = -vel.x * 0.75;
        }
        if (pos.z + radius > 1.0f || pos.z - radius < -1.0f){
            vel.z = -vel.z * 0.75;
        }
        if (pos.y + radius > 1.0f || pos.y - radius < -1.0f) {
            vel.y = -vel.y * 0.75;
            pos.y = glm::clamp(pos.y, -1.0f + radius, 1.0f - radius); // Prevent overshooting
        }
    }
};

// Generate vertex data
std::vector<float> generateSphereVertices(unsigned int stackCount, unsigned int sectorCount) {
    std::vector<float> vertices;
    
    float sectorStep = 2.f * M_PI / sectorCount;
    float stackStep = M_PI / stackCount;

    for (size_t i = 0; i <= stackCount; i++) {
        float stackAngle = i * stackStep;
        float xy = sinf(stackAngle);
        float z = cosf(stackAngle);

        // Add (sectorCount+1) vertices per stack
        // First and last vertices have same position and normal, but different tex coords.

        for (size_t j = 0; j <= sectorCount; j++) {
            float sectorAngle = M_PI / 2 - j * sectorStep;

            // vertex position
            float x = xy * cosf(sectorAngle);
            float y = xy * sinf(sectorAngle);

            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
        }
    }
    return vertices;
}

void generateSphereIndices(unsigned int stackCount, unsigned int sectorCount, std::vector<unsigned int>& indices, std::vector<unsigned int>& lineIndices) {
    for (int i = 0; i < stackCount; ++i) {
        int k1 = i * (sectorCount + 1); // beginning of current stack
        int k2 = k1 + sectorCount + 1; // beginning of next stack

        for (int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
            // 2 triangles per sector excluding first and last stacks.

            // Triangle 1
            if (i != 0) {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }

            // Triangle 2
            if (i != stackCount - 1) {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }

            // indices for lines
            // vertical lines for all stacks
            lineIndices.push_back(k1);
            lineIndices.push_back(k2);

            if (i != 0) {
                lineIndices.push_back(k1);
                lineIndices.push_back(k1 + 1);
            }
        }
    }
}

const int WIDTH = 1920;
const int HEIGHT = 1080;

// Vertex Buffer Object and Vertex Array Object
unsigned int VAO, VBO;
unsigned int EBO, EBO_LINES;

void initVAOVBO(std::vector<float> &sphereVertices, std::vector<unsigned int> &sphereIndices, std::vector<unsigned int> &sphereLineIndices) {
    // Generate VAO and bind it
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    // Generate VBO
    glGenBuffers(1, &VBO);
    // Bind the VBO buffer to the GL_ARRAY_BUFFER target
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // Buffer call on the bound buffer (VBO) to copy vertex data into its memory
    glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(float), sphereVertices.data(), GL_STATIC_DRAW);

    // Load index data into an EBO
    glGenBuffers(1, &EBO);
    // Appropriate target for index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIndices.size() * sizeof(unsigned int), sphereIndices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &EBO_LINES);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_LINES);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereLineIndices.size() * sizeof(unsigned int), sphereLineIndices.data(), GL_STATIC_DRAW);

    // Interpret vertex data determined by object bound to GL_ARRAY_BUFFER (VBO)
    // Position attribute (location = 0 in shader). Vertex is vec3 (floats). No input data normalization
    // Array is tightly packed
    // 5th argument is 'stride'. Tells the space between consecutive vertex attributes
    // Position data are 6 * sizeof(float) apart in the array (triangle, color)
    // 6th argument is offset of (position) data in the buffer. Position data is at start of the buffer
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    // Enable it
    glEnableVertexAttribArray(0);

    // Color attribute (location = 1 in shader)
    // Starts after position data in vertex buffer; appropriate offset
    //glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    //glEnableVertexAttribArray(1);

    // Unbind VAO, VBO
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// Simple camera structure
struct Camera {
    float orientationSpeed = 0.005f;
    float yaw = -90.0f;
    float pitch = 0.0f;
    float lastX = WIDTH / 2.f;
    float lastY = HEIGHT / 2.f;
    glm::vec3 pos = glm::vec3(0.f, 0.f, 1.f);
    glm::vec3 front = glm::vec3(0.f, 0.f, -1.f);
    glm::vec3 up = glm::vec3(0.f, 1.f, -1.f);
    bool firstMouse = true;
};

void processInput(GLFWwindow *window, Camera* camera) {
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera->pos += camera->orientationSpeed * camera->front;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera->pos -= camera->orientationSpeed * camera->front;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera->pos -= glm::normalize(glm::cross(camera->front, camera->up)) * camera->orientationSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera->pos += glm::normalize(glm::cross(camera->front, camera->up)) * camera->orientationSpeed;
}

// Mouse orienting callback
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    // Retrieve the user pointer
    Camera* camera = static_cast<Camera*>(glfwGetWindowUserPointer(window));
    
    if (camera->firstMouse) { // Prevent a jump when first moving the mouse
        camera->lastX = xpos;
        camera->lastY = ypos;
        camera->firstMouse = false;
    }
    float xoffset = xpos - camera->lastX;
    float yoffset = camera->lastY - ypos; // Reversed since y-coordinates go from bottom to top
    camera->lastX = xpos;
    camera->lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    camera->yaw += xoffset;
    camera->pitch += yoffset;

    // Constrain pitch to prevent flipping
    if (camera->pitch > 89.0f) camera->pitch = 89.0f;
    if (camera->pitch < -89.0f) camera->pitch = -89.0f;

    // Update `cameraFront`
    glm::vec3 front;
    front.x = cos(glm::radians(camera->yaw)) * cos(glm::radians(camera->pitch));
    front.y = sin(glm::radians(camera->pitch));
    front.z = sin(glm::radians(camera->yaw)) * cos(glm::radians(camera->pitch));
    camera->front = glm::normalize(front);
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }
    // Configure OpenGL version (4.6 Core)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Window
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "OpenGL Window", monitor, nullptr);
    if (window == NULL) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Load OpenGL funfctions using GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    std::vector<Sphere> spheres = {
        Sphere(glm::vec3{0,0,0}, glm::vec3{0,0,0}, 0.3f, 7.35E17)
    };

    int shaderProgram = createShaderProgram("vertex_shader.glsl", "fragment_shader.glsl");

    int sectorCount = 18;
    int stackCount = 9;
    std::vector<unsigned int> sphereIndices;
    std::vector<unsigned int> sphereLineIndices;

    std::vector<float> sphereVertices = generateSphereVertices(stackCount, sectorCount);
    generateSphereIndices(stackCount, sectorCount, sphereIndices, sphereLineIndices);

    initVAOVBO(sphereVertices, sphereIndices, sphereLineIndices);

    // Create a camera and set the window's user pointer to this.
    // Is done since 'glfwSetCursorPosCallback' signature limits argument list to this callback only.
    Camera camera;
    glfwSetWindowUserPointer(window, &camera);

    glm::mat4 view = glm::lookAt(camera.pos, camera.pos + camera.front, camera.up);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WIDTH / HEIGHT, 0.1f, 100.0f);

    // Get view location in a shader program
    int projLoc = glGetUniformLocation(shaderProgram, "projection");
    int viewLoc = glGetUniformLocation(shaderProgram, "view");

    auto lastTime = std::chrono::high_resolution_clock::now();
    auto currentTime = lastTime;

    // Bind callback
    glfwSetCursorPosCallback(window, mouse_callback);

    glUseProgram(shaderProgram);

    while (!glfwWindowShouldClose(window)) {
        currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> dt = currentTime - lastTime;
        lastTime = currentTime;

        processInput(window, &camera);

        // Update transformation matrix
        view = glm::lookAt(camera.pos, camera.pos + camera.front, camera.up);
        projection = glm::perspective(glm::radians(45.0f), (float)WIDTH / HEIGHT, 0.1f, 100.0f);
        // Send it to shader
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        for (auto &circle : spheres)
        {
            // Transformation matrix
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, circle.pos);

            // Upload uniforms
            int modelLoc = glGetUniformLocation(shaderProgram, "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            int radiusLoc = glGetUniformLocation(shaderProgram, "radius");
            glUniform1f(radiusLoc, circle.radius); // Send radius to shader

            circle.acc = glm::vec3(0.f);

            for (auto &circle2 : spheres) {
                if (&circle == &circle2) continue;

                glm::vec3 dr = circle2.pos - circle.pos;
                float dist = glm::length(dr);
                
                if (dist < circle.radius + circle2.radius) {
                    float overlap = circle.radius + circle2.radius - dist;
                    glm::vec3 correction = dr/dist * (overlap * 0.5f);

                    circle.pos = circle.pos - correction;
                    circle2.pos = circle2.pos + correction;
                }
                else {
                    glm::vec3 direction = dr / dist;
                    glm::vec3 Gforce = direction * (G * circle.mass * circle2.mass) / (powf(10,9)*dist*dist);
                    circle.applyForce(Gforce);
                }
            }

            circle.updatePos(dt.count());

            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sphereVertices.size() * sizeof(float), sphereVertices.data());

            // Draw sphere (wireframe). Bind VAO first
            glBindVertexArray(VAO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glDrawElements(GL_TRIANGLES, sphereIndices.size(), GL_UNSIGNED_INT, (void*)0);
            //glDrawElements(GL_LINES, sphereLineIndices.size(), GL_UNSIGNED_INT, (void*)0);

            // unbind VAO
            glBindVertexArray(0);
        }
        
        glfwSwapBuffers(window);
        glfwPollEvents(); // IO events
    }

    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
