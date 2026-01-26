#version 120

uniform sampler2DRect tex0;        // Source texture (particlesFbo)
uniform sampler2DRect asciiAtlas;  // ASCII texture atlas

uniform float cellSize;            // Size of each ASCII cell in the atlas (pixels per glyph)
uniform float screenCellSize;      // Size of each ASCII cell on the screen in pixels. If <= 0, falls back to atlas cell size
uniform float scaleFont;
uniform vec2 atlasSize;           // Size of the ASCII atlas in characters (width, height)
uniform float charsetOffset;      // Offset to start from in the ASCII table

uniform float time;
uniform float shader_mix;

varying vec2 vTexCoord;

vec4 sample;

float cellLuma(vec2 fragCoord) {
	// Use screen cell size when provided, otherwise fall back to atlas cell size
	float sCell = (screenCellSize > 0.0) ? screenCellSize : cellSize;
	vec2 cellCoord = floor(fragCoord / sCell) * sCell;
	vec2 cellCenter = cellCoord + vec2(sCell * 0.5);

	sample = texture2DRect(tex0, cellCenter);
	float brightness = dot(sample.rgb, vec3(0.299, 0.587, 0.114));
	return brightness;
}

vec4 getGlyph(sampler2DRect asciiAtlas, vec2 atlasUV) {
	vec4 glyph = texture2DRect(asciiAtlas, atlasUV);
	return glyph;
}

vec2 sobel(vec2 uv) {
    float gx = 0.0;
    float gy = 0.0;

    // Sobel kernels for x and y gradients
    float kernelX[9];
    float kernelY[9];

    kernelX[0] = -1.0; kernelX[1] = 0.0; kernelX[2] = 1.0;
    kernelX[3] = -2.0; kernelX[4] = 0.0; kernelX[5] = 2.0;
    kernelX[6] = -1.0; kernelX[7] = 0.0; kernelX[8] = 1.0;

    kernelY[0] = -1.0; kernelY[1] = -2.0; kernelY[2] = -1.0;
    kernelY[3] =  0.0; kernelY[4] =  0.0; kernelY[5] =  0.0;
    kernelY[6] =  1.0; kernelY[7] =  2.0; kernelY[8] =  1.0;

    int idx = 0;
    vec2 offset;
    vec2 texCoord;
    float sampleLum;

    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            offset = vec2(float(x), float(y));
            texCoord = uv + offset;
            vec3 col = texture2DRect(tex0, texCoord).rgb;
            sampleLum = dot(col, vec3(0.299, 0.587, 0.114));
            gx += sampleLum * kernelX[idx];
            gy += sampleLum * kernelY[idx];
            idx++;
        }
    }

    return vec2(gx, gy);
}

void main() {

	vec2 fragCoord = vTexCoord;

	vec4 originalColor = texture2DRect(tex0, fragCoord);
	float brightness = cellLuma(fragCoord);
	float charIndex = floor(((brightness) * scaleFont) + charsetOffset);
	charIndex = clamp(charIndex, 0.0, atlasSize.x * atlasSize.y - 1.0);

	float row = floor(charIndex / atlasSize.x);
	float col = mod(charIndex, atlasSize.x);

	float sCell = (screenCellSize > 0.0) ? screenCellSize : cellSize;
	vec2 localUV = mod(fragCoord, sCell) / sCell;   // fractional coords inside the screen cell
	vec2 charUV = localUV;

	// Map charUV (0..1) into atlas pixel coords using atlas cell size (cellSize)
	vec2 atlasUV = vec2(col * cellSize, row * cellSize) + charUV * cellSize;

	// Clamp atlas coords to atlas extents to avoid sampling outside the atlas
	vec2 atlasExtent = vec2(atlasSize.x * cellSize, atlasSize.y * cellSize);
	atlasUV = clamp(atlasUV, vec2(0.0), atlasExtent - vec2(1.0));
	vec4 glyph = getGlyph(asciiAtlas, atlasUV);

	float glyphMask = glyph.r;
	vec4 asciiColor = vec4(sample.rgb * glyphMask, 1.0);

	// gl_FragColor = mix(originalColor, asciiColor, shader_mix);

	vec4 mixed = mix(originalColor, asciiColor, shader_mix);

	// multiply again by originalColor to make a bloom accentuation

	mixed.rgb *= originalColor.rgb;
	gl_FragColor = mixed;


}

// vim:ft=glsl
