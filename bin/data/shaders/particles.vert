#version 150

in vec4 position;
in vec4 color;
in float size;

out vec4 vColor;

void main() {
    gl_Position = modelViewProjectionMatrix * position;
    gl_PointSize = size;
    vColor = color;
}

