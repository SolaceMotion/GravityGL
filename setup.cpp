#include <glad/glad.h>

#include <iostream>
#include <fstream>
#include <sstream>

#include "setup.hpp"

std::string readShader(const std::string& path) {
    std::ifstream shaderFile(path);
    if (!shaderFile) {
        std::cerr << "Failed to open shader file: " << path << std::endl;
        return nullptr;
    }

    std::stringstream ss;
    ss << shaderFile.rdbuf();
    return ss.str();
}

void checkShaderCompileErrors(unsigned int shader, const char* flag) {
    int success;
    char infoLog[1024];

    if (flag == "PROGRAM") {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "Shader Program linking failed.\n" << infoLog << std::endl;
        }
    } else {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            std::cout << flag << std::endl;
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "Shader compilation failed.\n" << infoLog << std::endl; 
        }
    }
}

int createShaderProgram(const char* vertexSrc, const char* fragmentSrc) {
    std::string vShaderSrc = readShader(vertexSrc);
    std::string fShaderSrc = readShader(fragmentSrc);

    const char* vShaderCode = vShaderSrc.c_str();
    const char* fShaderCode = fShaderSrc.c_str();

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vShaderCode, NULL);
    glCompileShader(vertexShader);
    checkShaderCompileErrors(vertexShader, "VERTEX");

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fShaderCode, NULL);
    glCompileShader(fragmentShader);
    checkShaderCompileErrors(fragmentShader, "FRAGMENT");

    // Link shaders into a program
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    checkShaderCompileErrors(shaderProgram, "PROGRAM");

    // Cleanup (shaders are now linked, so they can be deleted)
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return shaderProgram;
}