#version 330 core
out vec4 FragColor;

in vec3 color;
in vec3 Normal;
in vec3 FragPos; // in world coordinates
in vec2 texcoord;
in float Height;

uniform sampler2D texture_diff1;
uniform vec3 LightDir;
uniform vec3 viewPos;

void main()
{
	vec3 texture_color = vec3(texture(texture_diff1, texcoord));
	vec3 LightColor = vec3(250.0, 239.0, 205.0)/255;

	// ambiance
	float ambientStrength = 0.05;
	vec3 ambientColor = ambientStrength * LightColor;

	// diffuse
	vec3 norm = normalize(Normal);
	vec3 lightDir = normalize(LightDir);
	float diff = max(dot(norm, -lightDir), 0.0);
	vec3 diffuseColor = diff * LightColor;

	// specular
	float specularStrength = 0.1;
	if (Height <= 0 || Height >= 65) {
		specularStrength = 0.5;
	}
	vec3 viewDir = normalize(viewPos - FragPos);
	vec3 reflectDir = reflect(lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
	vec3 specularColor = specularStrength * spec * LightColor;

	vec3 color = (ambientColor + diffuseColor + specularColor) * color + texture_color;	
	FragColor = vec4(color, 1.0);
}