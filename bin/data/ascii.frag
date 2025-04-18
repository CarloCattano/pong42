#version 120

uniform sampler2DRect tex0;        // Source texture (particlesFbo)
uniform sampler2DRect asciiAtlas;  // ASCII texture atlas
uniform float cellSize;            // Size of each ASCII cell in pixels
uniform vec2 atlasSize;           // Size of the ASCII atlas in characters (width, height)
uniform float scaleFont;
uniform float charsetOffset;      // Offset to start from in the ASCII table

varying vec2 vTexCoord;

void main() {
    vec2 fragCoord = vTexCoord;

    // Find the current cell center and sample the image for brightness and color
    vec2 cellCoord = floor(fragCoord / cellSize) * cellSize;
    vec2 cellCenter = cellCoord + vec2(cellSize * 0.5);

    vec4 sample = texture2DRect(tex0, cellCenter);
    float brightness = dot(sample.rgb, vec3(0.299, 0.587, 0.114));

    float charIndex = floor(((1.0 - brightness) * scaleFont));

	charIndex = mod(charIndex, scaleFont);

	charIndex = charIndex + charsetOffset;

	// clamp to max
	charIndex = clamp(charIndex, 0.0, atlasSize.x * atlasSize.y - 2.0);

	float row = floor(charIndex / atlasSize.x);
    float col = mod(charIndex, atlasSize.x);

    // UVs within the character cell
    vec2 charUV = (mod(fragCoord, cellSize) / cellSize );

    // Final UVs to sample the character glyph
    vec2 atlasUV = (vec2(col, row) + charUV) * cellSize;

    // Sample the glyph's shape (we use the red channel or alpha depending on the atlas)
    vec4 glyph = texture2DRect(asciiAtlas, atlasUV);

    // Apply the original sampled color to the glyph shape
    // Assuming glyph uses red channel (or .a if your atlas has alpha)
    float glyphMask = glyph.r;
    gl_FragColor = vec4(sample.rgb * glyphMask, 1.0);
}

