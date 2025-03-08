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

glm::vec3 cameraPos = glm::vec3(0.f, 0.f, 1.f);
glm::vec3 cameraFront = glm::vec3(0.f, 0.f, -1.f);
glm::vec3 cameraUp = glm::vec3(0.f, 1.f, -1.f);


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
std::vector<float> generateCircleVertices(unsigned int segments) {
    std::vector<float> vertices;
    
    // Angle step per segment
    float angleStep = 2.0f * M_PI / segments;

    for (size_t i = 0; i <= segments; i++) {
        float theta = M_PI * (float)i/segments;
        //float theta1 = (i+1)/segments * glm::pi<float>();

        for (size_t j = 0; j < segments; j++) {
            float phi = 2.f * M_PI * (float)j/segments;

            float x = cos(phi)*sin(theta);
            float y = sin(phi)*sin(theta);
            float z = cos(theta);

            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);

            // Add simple color (gradient based on latitude)
            float r = sin(phi);
            float g = cos(theta);
            float b = sin(theta);
            vertices.push_back(r);
            vertices.push_back(g);
            vertices.push_back(b);

        }
        //float angle = i * angleStep;
        // Relative to (0,0,0), unit circle
        //float x = cos(angle);
        //float y = sin(angle);
        
        //vertices.push_back(x);
        //vertices.push_back(y);
        //vertices.push_back(0.0f); // Z = 0
        
        // Add color (Change to your preferred color logic)
        //float r = 0.5f + 0.5f * cos(angle); // Example: gradient color
        //float g = 0.5f + 0.5f * sin(angle);
        //float b = 1.0f;
        //vertices.push_back(r);
        //vertices.push_back(g);
        //vertices.push_back(b);
    }
    return vertices;
}

std::vector<unsigned int> generateSphereIndices(unsigned int segments) {
    std::vector<unsigned int> indices;

    for (unsigned int lat = 0; lat < segments; ++lat) {
        for (unsigned int lon = 0; lon < segments; ++lon) {
            unsigned int current = lat * (segments + 1) + lon;
            unsigned int next = current + segments + 1;

            // Triangle 1
            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);

            // Triangle 2
            indices.push_back(current + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }

    return indices;
}

const int WIDTH = 600;
const int HEIGHT = 600;

// Generate VAO & VBO
unsigned int VAO, VBO;
unsigned int EBO;

void initVAOVBO(std::vector<float> &sphereVertices, std::vector<unsigned int> &sphereIndices) {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    //
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    // Load vertex data
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(float), sphereVertices.data(), GL_STATIC_DRAW);

    // Load index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIndices.size() * sizeof(unsigned int), sphereIndices.data(), GL_STATIC_DRAW);

    // Position attribute (location = 0 in shader)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Color attribute (location = 1 in shader)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

float cameraSpeed = 0.05f;

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
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

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "OpenGL Window", nullptr, nullptr);
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

    int segments = 100;

    glm::vec3 initPos1(0.f, 0.f, 0.f);
    glm::vec3 initPos2(0.7f, 0.2f, 0.f);

    std::vector<Sphere> circles = {
        Sphere(glm::vec3{0,0,0}, glm::vec3{0,-0.2f,0}, 0.1f, 7.35E17),
        Sphere(glm::vec3{0.7f,0.2f,0}, glm::vec3{0,0.2f,0}, 0.07f, 7.35E17),
        Sphere(glm::vec3{-0.8f,0.3f,0}, glm::vec3{0.1f,0.1f,0}, 0.04f, 7.35E17),

    };

    int shaderProgram = createShaderProgram("vertex_shader.glsl", "fragment_shader.glsl");
    glUseProgram(shaderProgram);

    std::vector<float> circleVertices = generateCircleVertices(50);
    std::vector<unsigned int> sphereIndices = generateSphereIndices(50);
    initVAOVBO(circleVertices, sphereIndices);
    
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    int viewLoc = glGetUniformLocation(shaderProgram, "view");

    auto lastTime = std::chrono::high_resolution_clock::now();
    auto currentTime = lastTime;

    float angle = 0;

    // Main loop
    while (!glfwWindowShouldClose(window)) {

        processInput(window);

        currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> dt = currentTime - lastTime;
        lastTime = currentTime;

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        for (auto &circle : circles)
        {
            // Transformation matrix
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, circle.pos);

            // Upload uniforms
            int modelLoc = glGetUniformLocation(shaderProgram, "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            int radiusLoc = glGetUniformLocation(shaderProgram, "radius");
            glUniform1f(radiusLoc, circle.radius); // Send radius to shader
            
            angle += dt.count() * 0.5f;


            view = glm::lookAt(cameraPos, glm::vec3(0,0,0), glm::vec3(0,1,0));

            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

            circle.acc = glm::vec3(0.f);

            for (auto &circle2 : circles) {
                if (&circle == &circle2) continue;

                glm::vec3 dr = circle2.pos - circle.pos;
                float dist = glm::length(dr);
                
                
                if (dist < circle.radius + circle2.radius) {
                    std::cout << "COL" << std::endl;
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
            glBufferSubData(GL_ARRAY_BUFFER, 0, circleVertices.size() * sizeof(float), circleVertices.data());

            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, circleVertices.size(), GL_UNSIGNED_INT, 0); // Equivalent of glBegin(GL_TRIANGLES) + glVertex3f calls
        }
        
        glfwSwapBuffers(window);
        glfwPollEvents(); // Input events
    }

    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
