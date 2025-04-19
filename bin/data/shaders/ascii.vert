// ascii.vert
#version 120

varying vec2 vTexCoord;
varying vec4 vColor;

void main() {
    vTexCoord = gl_MultiTexCoord0.xy;
    vColor = gl_Color;
    gl_Position = ftransform();
}
