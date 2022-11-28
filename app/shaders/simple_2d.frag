#version 430

layout(location = 1) uniform sampler2D u_texture;
layout(location = 2) uniform vec4 u_tint;

layout(location = 0) in vec2 v_texCoord;
layout(location = 0) out vec4 o_color;

void main(){
	o_color = u_tint * texture(u_texture, v_texCoord);
}