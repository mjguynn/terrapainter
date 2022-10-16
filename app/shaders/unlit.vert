#version 330

uniform mat4x4 u_transform;

layout(location = 0) in vec4 v_position;
layout(location = 1) in vec2 v_texCoord;

layout(location = 0) out vec2 o_texCoord;

void main() {
    o_texCoord = v_texCoord;
    gl_Position = u_transform * v_position;
}