#version 430

layout(location = 2) uniform samplerCube u_texture;
layout(location = 0) in vec3 v_fragPos;
layout(location = 0) out vec4 o_color;

void main(){
	vec3 coord = vec3(v_fragPos.x, -v_fragPos.z, v_fragPos.y);
	o_color = texture(u_texture, coord);
}