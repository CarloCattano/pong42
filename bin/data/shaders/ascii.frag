#version 120

uniform sampler2DRect tex0;        // Source texture (particlesFbo)
uniform sampler2DRect asciiAtlas;  // ASCII texture atlas

uniform float cellSize;            // Size of each ASCII cell in pixels
uniform float scaleFont;
uniform vec2 atlasSize;           // Size of the ASCII atlas in characters (width, height)
uniform float charsetOffset;      // Offset to start from in the ASCII table

uniform float time;
uniform float shader_mix;

varying vec2 vTexCoord;

vec4 sample;
vec2 fragCoord = vTexCoord;

float cellLuma(vec2 fragCoord) {
	vec2 cellCoord = floor(fragCoord / cellSize) * cellSize;
	vec2 cellCenter = cellCoord + vec2(cellSize * 0.5);

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

	vec4 originalColor = texture2DRect(tex0, fragCoord);
	float brightness = cellLuma(fragCoord);
	float charIndex = floor(((brightness) * scaleFont) + charsetOffset);
	charIndex = clamp(charIndex, 0.0, atlasSize.x * atlasSize.y - 1.0);

	float row = floor(charIndex / atlasSize.x);
	float col = mod(charIndex, atlasSize.x);

	vec2 localUV = mod(fragCoord, cellSize) / cellSize;
	vec2 charUV = localUV;

	vec2 atlasUV = (vec2(col, row) + charUV) * cellSize;
	vec4 glyph = getGlyph(asciiAtlas, atlasUV);

	float glyphMask = glyph.r;
	vec4 asciiColor = vec4(sample.rgb * glyphMask, 1.0);

	gl_FragColor = mix(originalColor, asciiColor, shader_mix);
}

// vim:ft=glsl
