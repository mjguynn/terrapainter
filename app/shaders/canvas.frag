#version 330

uniform sampler2D u_texture;

layout(location = 0) in vec2 v_texCoord;
layout(location = 0) out vec4 o_color;

void main(){
	o_color = texture(u_texture, v_texCoord);
}