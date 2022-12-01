#version 430 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoord;

out vec2 TexCoords;

layout (location = 0) uniform mat4 viewProj;
layout (location = 1) uniform mat4 model;

void main()
{
    TexCoords = texCoord;    
    gl_Position = viewProj * model * vec4(position, 1.0);
}