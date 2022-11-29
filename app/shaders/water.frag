#version 430 core

layout (location = 2) uniform vec4 u_cullPlane;
layout (location = 3) uniform sampler2D u_reflectionTexture;

layout (location = 0) in vec3 v_normalDir;
layout (location = 1) in vec3 v_fragPos; // in world space

out vec4 o_color;

void main()
{
	o_color = vec4(1, 0, 0, 1);
}