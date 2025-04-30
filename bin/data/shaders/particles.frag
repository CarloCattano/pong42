
#version 150

in vec4 vertexColor;
out vec4 outputColor;

void main() {

    vec2 circCoord = 2.0 * gl_PointCoord - 1.0;
    float dist = dot(circCoord, circCoord);
    if (dist > 1.0) {
        discard;
    }
    float alpha = 1.0 - smoothstep(0.7, 1.0, dist);
    outputColor = vec4(vertexColor.rgb, vertexColor.a * alpha);
}
