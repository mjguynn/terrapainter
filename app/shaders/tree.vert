#version 430 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 tangent;
layout (location = 3) in vec3 biTangent;
layout (location = 4) in vec2 texCoord;
layout (location = 5) in mat4 aInstanceMatrix;

out vec3 v_normalDir;
out vec3 v_tangentDir;
out vec3 v_biTangentDir;
out vec2 TexCoords;
out vec3 v_fragPos;

layout (location = 0) uniform mat4 viewProj;
layout (location = 1) uniform mat4 model;

void main()
{
    v_normalDir = (transpose(inverse(aInstanceMatrix)) * vec4(normal, 0)).xyz;
	v_tangentDir = (aInstanceMatrix * vec4(tangent, 0)).xyz;
    v_biTangentDir = (aInstanceMatrix * vec4(biTangent, 0)).xyz;
    TexCoords = texCoord;   
     
    vec4 worldPos = aInstanceMatrix * vec4(position, 1);
    v_fragPos = worldPos.xyz;
    gl_Position = viewProj * worldPos;
}