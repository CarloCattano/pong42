#version 120

uniform sampler2DRect tex0;        // Source texture (particlesFbo)
uniform sampler2DRect asciiAtlas;  // ASCII texture atlas
uniform float cellSize;            // Size of each ASCII cell in pixels
uniform vec2 atlasSize;           // Size of the ASCII atlas in characters (width, height)
uniform float scaleFont;
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

	 if (asciiColor.r <= 0.01 && asciiColor.g <= 0.01 && asciiColor.b <= 0.01) {
	 	float radius = 3.0;
	 	vec2 center = fragCoord - vec2(radius, radius);
	 	float dist = distance(center, fragCoord);
	 	if (dist < radius) {
	 		asciiColor = vec4(0.0, 0.0, 0.0, 1.0);
	 	}
	 }

	 // rudimentary contours
	 float threshold = 0.1;
	 float edge = step(threshold, abs(sample.r - originalColor.r));
	 edge += step(threshold, abs(sample.g - originalColor.g));
	 edge += step(threshold, abs(sample.b - originalColor.b));
	 edge = clamp(edge, 0.0, 1.0);
	 asciiColor = mix(asciiColor, vec4(0.0, 0.0, 0.0, 1.0), edge);

	gl_FragColor = mix(originalColor, asciiColor, shader_mix);
}

