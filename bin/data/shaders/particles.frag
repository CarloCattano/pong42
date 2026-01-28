#version 150

// Render point-sprites as ASCII glyphs sampled from an atlas.
// Each vertex provides glyph index in texcoord.x and (optionally) particle size in texcoord.y.

in vec4 vColor;
flat in float vGlyphIndex;
flat in float vRotation;
in float vParticleSize;
out vec4 outputColor;

uniform sampler2DRect asciiAtlas;
uniform float cellSize;   // pixels per glyph in atlas
uniform vec2 atlasSize;   // columns, rows (in glyphs)
uniform float time;       // time for animation (seconds)

void main() {
    // optional circular mask for nicer glyph shapes
    float dist = length(gl_PointCoord - vec2(0.5));
    if (dist > 0.5) discard;

    // compute atlas column/row from glyph index
    float charIndex = vGlyphIndex;
    float col = mod(charIndex, atlasSize.x);
    float row = floor(charIndex / atlasSize.x);

    	// local coordinates inside the point sprite [0..1]
    	vec2 charUV = gl_PointCoord;

    	// rotate coordinates around center by per-particle rotation (radians)
    	vec2 centered = charUV - vec2(0.5);
    	float ca = cos(vRotation);
    	float sa = sin(vRotation);
    	vec2 rotated = vec2(centered.x * ca - centered.y * sa, centered.x * sa + centered.y * ca);
    	charUV = rotated + vec2(0.5);
    	// clamp to [0..1] to avoid sampling outside the cell after rotation
    	charUV = clamp(charUV, vec2(0.0), vec2(1.0));

    	// map into atlas pixel coords (sampler2DRect expects pixel coords)
    	vec2 atlasUV = vec2(col * cellSize, row * cellSize) + charUV * cellSize;

    vec4 glyph = texture(asciiAtlas, atlasUV);
    float glyphMask = glyph.r;

    // time-based flicker (per-particle variation using glyph index)
    float flick = 0.90 + 0.10 * sin(time * 6.0 + vGlyphIndex * 0.271);
    // apply flicker to color intensity (alpha remains masked by glyphMask)
    vec4 asciiColor = vec4(vColor.rgb * glyphMask * flick, vColor.a * glyphMask);

    outputColor = asciiColor;
}
