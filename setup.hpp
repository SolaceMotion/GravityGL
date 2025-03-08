#ifndef SETUP_HPP
#define SETUP_HPP

#include <string>

int createShaderProgram(const char* vertexSrc, const char* fragmentSrc);
std::string readShader(const std::string& path);

#endif