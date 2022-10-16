#version 330

layout(location = 0) in vec4 v_position;

void main() {
    gl_Position = v_position;
}