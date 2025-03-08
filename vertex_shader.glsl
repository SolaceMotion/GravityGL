#version 330 core
layout(location=0) in vec3 aPos; // Vertex position
layout(location=1) in vec3 aColor; // Vertex color

uniform mat4 model; // Object transformation
uniform mat4 view; // Camera transformation
uniform mat4 projection; // Perspective projection
uniform float radius; // Circle radius

out vec3 vertexColor; // Passed to fragment shader

void main() {
    vec3 scaledPos = aPos * radius; // Scale the circle vertices
    gl_Position = view * model * vec4(scaledPos, 1.0);
    vertexColor = aColor;
}