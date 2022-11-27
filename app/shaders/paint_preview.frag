#version 430

layout(location = 0) uniform ivec2 u_cursor;
// (unused, outer radius, hardness, canvas scale)
layout(location = 1) uniform vec4 u_params;
layout(location = 2) uniform vec4 u_strokeColor;

layout(location = 0) out vec4 o_color;

float circle(vec2 center, vec2 current, float radius, float thickness) {
	float dist = 1.0f - abs(radius - length(current - center));
	return clamp(dist + thickness, 0.0f, 1.0f);
}
void main(){
	float radius = u_params.y * u_params.w;
	float hardness = u_params.z;
	float thickness = min(1.0f, u_params.w);
	vec2 cursor = vec2(u_cursor) + vec2(0.5, 0.5);
	
	vec4 color = u_strokeColor;
	color.a *= circle(gl_FragCoord.xy, cursor, radius, thickness);
	o_color = color;
}