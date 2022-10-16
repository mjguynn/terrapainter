#version 330

uniform vec2 u_center;
uniform ivec2 u_screenSize;
uniform float u_radius;
uniform float u_thickness;

layout(location = 0) out vec4 o_color;

void main(){
	vec2 offset = u_center - gl_FragCoord.xy;
	
	// 0 if and only if this pixel is perfectly on the circle
	float dist = 1.0f - abs(u_radius - length(offset));
	dist += u_thickness;
	dist = clamp(dist, 0.0f, 1.0f);
	
	o_color = vec4(1.0f, 1.0f, 1.0f, dist);
}