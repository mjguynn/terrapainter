#version 430 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 tangent;

layout (location = 0) out vec3 v_normalDir;
layout (location = 1) out vec3 v_tangentDir;
layout (location = 2) out vec3 v_fragPos;
layout (location = 3) out vec2 v_texcoord;

layout (location = 0) uniform mat4 u_worldToProjection;
layout (location = 1) uniform mat4 u_modelToWorld;

void main()
{
	v_normalDir = (transpose(inverse(u_modelToWorld)) * vec4(normal, 0)).xyz;
	v_tangentDir = (u_modelToWorld * vec4(tangent, 0)).xyz;
	vec4 worldPos = u_modelToWorld * vec4(position, 1);
	v_fragPos = worldPos.xyz;
	v_texcoord = position.xy/16;
	gl_Position = u_worldToProjection * worldPos;
}