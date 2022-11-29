#version 430

layout(location = 0) uniform mat4x4 u_worldToProj;
layout(location = 1) uniform mat4x4 u_modelToWorld;
layout(location = 0) in vec3 v_position;
layout(location = 0) out vec3 v_fragPos;
void main() {
	vec4 worldPos = u_modelToWorld * vec4(v_position, 1);
	v_fragPos = worldPos.xyz;
    gl_Position = u_worldToProj * worldPos;
}