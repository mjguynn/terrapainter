#version 430 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

layout (location = 0) out vec3 v_normalDir;
layout (location = 1) out vec3 v_fragPos;

layout (location = 0) uniform mat4 u_worldToProjection;
layout (location = 1) uniform mat4 u_modelToWorld;

void main()
{
	vec4 normalDir = u_modelToWorld * vec4(normal, 0);
	v_normalDir = normalDir.xyz;
	vec4 worldPos = u_modelToWorld * vec4(position, 1);
	v_fragPos = worldPos.xyz;
	gl_Position = u_worldToProjection * worldPos;
}