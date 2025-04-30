
#version 150

uniform mat4 modelViewProjectionMatrix;
uniform float particleSize;
in vec4 position;
in vec4 color;
in vec4 normal;
out vec4 vertexColor;

void main() {
    float size = normal.x * particleSize;
    gl_PointSize = size;
    gl_Position = modelViewProjectionMatrix * position;
    vertexColor = color;
}

