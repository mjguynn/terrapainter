#version 430 core

layout (location = 2) uniform float u_time;
layout (location = 3) uniform sampler2D u_reflectionTexture;
layout (location = 4) uniform ivec2 u_screenSize;
layout (location = 5) uniform vec4 u_cullPlane;
layout (location = 6) uniform sampler2D u_canvasTexture;
layout (location = 7) uniform ivec2 u_canvasSize;
layout (location = 8) uniform vec3 u_sunDir;
layout (location = 9) uniform vec3 u_eyePos;
layout (location = 10) uniform sampler2D u_normalTexture1;
layout (location = 11) uniform sampler2D u_normalTexture2;
layout (location = 12) uniform sampler2D u_seafoamTexture;
layout (location = 13) uniform vec3 u_sunColor;

layout (location = 0) in vec3 v_normalDir;
layout (location = 1) in vec3 v_fragPos; // in world space

out vec4 o_color;

float foam_factor(float height) {
	return 20*height*height;
}
float wave_factor(float height, float time) {
	const float TIME_SCALE = 0.2;
	float scaledTime = time * TIME_SCALE;
	float unused;
	float waveAmount = pow(modf(scaledTime, unused), 0.7);
	float maxHeight = waveAmount * 0.1640625;
	
	float lowFade = waveAmount*waveAmount*waveAmount;
	float highFade = (1-waveAmount)*(1-waveAmount);
	
	float heightAdj = height - 0.05;
	// hehehe gpu joke
	float wavefront = sqrt(max(0, 1 - heightAdj/maxHeight));
	float waveback = clamp(heightAdj/maxHeight-0.3, 0, 1);
	return 20 * lowFade * highFade * wavefront * waveback;
}
vec3 compute_normal() {
	vec2 normal1Coords = v_fragPos.xy / 128 + 0.02*vec2(cos(u_time), sin(u_time));
	vec2 normal2Coords = v_fragPos.xy / 256 + vec2(u_time*0.03, -0.1);
	vec2 normal3Coords = v_fragPos.xy / 8 - 0.01*vec2(sin(u_time), u_time);
	vec3 normal1 = 2*texture(u_normalTexture1, normal1Coords).rgb - 1;
	vec3 normal2 = 2*texture(u_normalTexture2, normal2Coords).rgb - 1;
	vec3 normal3 = 2*texture(u_normalTexture1, normal3Coords).rgb - 1;
	
	return normalize(normal1 + normal2 + normal3);
}
float compute_height() {
	vec2 canvasCoords = v_fragPos.xy / vec2(u_canvasSize) + 0.5;
	return texture(u_canvasTexture, canvasCoords).r;
}
float foam_factor_2(vec3 normal, float height) {
	vec2 foamCoords = v_fragPos.xy / 64 + normal.xy / 32 + 0.01*vec2(u_time, 0);
	vec3 foam2 = texture(u_seafoamTexture, foamCoords).rgb;
	float foamFacPre = clamp(1-8*height, 0, 1) + foam2.r;
	return clamp(foamFacPre - 1.75, 0, 1) / 4;
}
// Normal is used to perturb the reflection
vec3 compute_mirror_reflection(vec3 viewDir, vec3 normal) {
	vec2 screenCoords = vec2(gl_FragCoord) / vec2(u_screenSize);
	vec3 refl = texture(u_reflectionTexture, screenCoords + normal.xy / 50).rgb;
	float fac = clamp(1-dot(viewDir, normal), 0.7, 1);
	return refl * fac;
}
float phong_factor(vec3 lightDir, vec3 viewDir, vec3 normal) {
	vec3 halfAngle = normalize(viewDir + lightDir);
	float phongFac = 2.5*pow(max(0, dot(normal, halfAngle)), 20);
	float geomFac = max(0, dot(normal, lightDir));
	return phongFac * geomFac;
}
void main()
{
	if (dot(vec4(v_fragPos, 1), u_cullPlane) < 0)
		discard;
	
	// Compute needed parameters
	vec3 viewDir = normalize(u_eyePos - v_fragPos);
	vec3 lightDir = normalize(u_sunDir);
	vec3 lightColor = u_sunColor;
	vec3 normal = compute_normal();
	// Note: this is the height from the heightmap, not the actual worldspace height
	float height = compute_height();
	
	vec4 base = vec4(compute_mirror_reflection(viewDir, normal), 0.85);
	vec4 foam = vec4(0.85, 0.95, 1.0, 0.8);
	vec4 foamed = mix(base, foam, foam_factor_2(normal, height));
	
	vec4 prewave = mix(foamed, vec4(1, 1, 1, 1), phong_factor(lightDir, viewDir, normal));
	prewave.a -= max(4*(height-0.05), 0);
	
	vec4 waves = vec4(1, 1, 1, 1);
	vec4 wavy = mix(prewave, waves, wave_factor(height, u_time));
	
	o_color = wavy;
}