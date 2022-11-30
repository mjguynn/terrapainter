#version 430

layout(location = 0) uniform ivec2 u_cursor;
// (unused, outer radius, hardness, canvas scale)
layout(location = 1) uniform vec4 u_params;
layout(location = 2) uniform vec4 u_strokeColor;

layout(location = 0) out vec4 o_color;

// TODO: This is mostly copied from paint_stroke.comp
// It should be in a shared header but oh yeah GLSL doesn't have those
float hardness_factor(float distance, float radius, float hardness) {
	float normalized = clamp( (radius - distance) / radius, 0.0f, 1.0f);
	return pow(normalized, hardness);
}
float antialias_factor(float distance, float radius) {
	return clamp(radius - distance, 0.0f, 1.0f);
}

float circle(vec2 center, vec2 current, float radius, float thickness) {
	float dist = 1.0f - abs(radius - length(current - center));
	return clamp(dist + thickness, 0.0f, 1.0f);
}
void main(){
	float radius = u_params.y * u_params.w;
	float hardness = u_params.z;
	float thickness = min(1.0f, u_params.w);
	vec2 cursor = vec2(u_cursor) + vec2(0.5, 0.5);
	
	float fac = circle(gl_FragCoord.xy, cursor, radius, thickness);
	vec4 border = vec4(1-u_strokeColor.rgb, 1);
	
	vec4 preview = u_strokeColor;
	float dist = length(gl_FragCoord.xy - cursor);
	float h = hardness_factor(dist, radius, hardness);
	float a = antialias_factor(dist, radius);
	preview.a *= h*a;
	
	o_color = mix(preview, border, fac);
}