#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out float Height;
out vec3 Normal;
out vec3 FragPos;
out vec2 texcoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection; 


void main()
{
	Height = aPos.y;
	Normal = mat3(transpose(inverse(model))) * aNormal;
	FragPos = vec3(model * vec4(aPos, 1.0));
	texcoord = 0.05 * (aPos.xz + vec2(aPos.y,aPos.y)); 

	gl_Position = projection * view * model * vec4(aPos, 1.0f);
}