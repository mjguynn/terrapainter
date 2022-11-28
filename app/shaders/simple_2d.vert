#version 430

layout(location = 0) uniform mat3x3 u_transform;

layout(location = 0) in vec3 v_position;
layout(location = 1) in vec2 v_texCoord;

layout(location = 0) out vec2 o_texCoord;

void main() {
	o_texCoord = v_texCoord;
    gl_Position = vec4(u_transform * v_position, 1);
}