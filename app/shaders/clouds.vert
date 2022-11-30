#version 430 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texcoord; 

layout (location = 0) uniform mat4 u_worldToProjection;
layout (location = 1) uniform mat4 u_modelToWorld;

out vec2 uv;

void main()
{
  uv = texcoord;
	vec4 worldPos = u_modelToWorld * vec4(position, 1);
	gl_Position = u_worldToProjection * worldPos;
}