#version 150

// TODO: Implement


in vec4 vColor;
out vec4 outputColor;

void main() {
    float dist = length(gl_PointCoord - vec2(0.5));
    if (dist > 0.5) discard;

    outputColor = vColor;
}


