#version 330 core
in vec3 vertexColor; // Color from vertex shader (same name and type as output in vertex shader)
// vertexColor in fragment shader is thereby linked to vertexColor in vertex shader

out vec4 FragColor;

void main() {
    FragColor = vec4(vertexColor, 1.0); // Use vertex color
}