#version 430

layout(location = 2) uniform samplerCube u_texture;
layout(location = 3) uniform vec3 u_viewPos;

layout(location = 0) in vec3 v_fragPos;
layout(location = 0) out vec4 o_color;

void main(){
	// eye-relative coords
	// the height is a hack
	vec3 pos = vec3(v_fragPos.xy - u_viewPos.xy, v_fragPos.z);
	vec3 coord = vec3(pos.x, -pos.z, pos.y);
	o_color = texture(u_texture, coord);
}