#version 330

uniform vec2 u_center;
uniform ivec2 u_screenSize;
uniform float u_radius;
uniform float u_thickness;

layout(location = 0) out vec4 o_color;

void main(){
	float offset = length(u_center - gl_FragCoord.xy);
	float inner_radius = u_radius - 0.5f * u_thickness;
	float outer_radius = u_radius + 0.5f * u_thickness;
	float opacity;
	if (offset >= inner_radius && offset <= outer_radius) {
		opacity = 1.0f;
	} else {
		opacity = 0.0f;
	}
	o_color = vec4(1.0f, 1.0f, 1.0f, opacity);
}