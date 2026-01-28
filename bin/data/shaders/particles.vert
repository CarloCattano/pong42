#version 150

in vec4 position;
in vec4 color;
in vec2 texcoord; // texcoord.x = glyphIndex, texcoord.y = particle size (optional)
in vec3 normal; // rotation encoded in normal.x

uniform float pointSize; // uniform point size (can be overridden by texcoord.y in the shader if desired)

out vec4 vColor;
flat out float vGlyphIndex;
out float vParticleSize;
flat out float vRotation;

void main() {
    // transform vertex
    gl_Position = modelViewProjectionMatrix * position;

    // prefer per-vertex size (texcoord.y) if provided, else fallback to uniform
    float ps = (texcoord.y > 0.0) ? texcoord.y : pointSize;
    gl_PointSize = ps;

    // pass data to fragment shader
    vColor = color;
    vGlyphIndex = texcoord.x;
    vParticleSize = texcoord.y;
    vRotation = normal.x;
}
