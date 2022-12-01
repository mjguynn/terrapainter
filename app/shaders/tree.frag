#version 430 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 v_fragPos;
in vec3 v_normalDir;
in vec3 v_tangentDir;
in vec3 v_biTangentDir;

uniform sampler2D texture_diffuse0;
uniform sampler2D texture_normal0;

layout (location = 2) uniform vec3 u_sunDir;
layout (location = 3) uniform vec3 u_viewPos;
layout (location = 4) uniform vec3 u_sunColor;


vec3 adjust_normal(vec3 normalMap) {
	vec3 real = 2*normalMap - 1; // (real)
	vec3 normal = normalize(v_normalDir);
	vec3 tangent = normalize(v_tangentDir);
	vec3 bitangent = normalize(v_biTangentDir);
	
	mat3 tbn = mat3(bitangent, tangent, normal);
	return normalize(tbn * real);
}

void main()
{   
	vec4 base = texture(texture_diffuse0, TexCoords);
	if (base.a < 0.5)
		discard;
	vec3 color = base.xyz;
	
	vec4 ns = texture(texture_normal0, TexCoords);
	vec3 norm = adjust_normal(ns.xyz);
	float specularStrength = ns.w;
	
	vec3 lightColor = u_sunColor;
	
	// ambiance
	float ambient = 0.2;

	// diffuse
	vec3 lightDir = normalize(u_sunDir);
	float diffuse = 0.6*max(dot(norm, lightDir), 0);

	// specular
	vec3 viewDir = normalize(u_viewPos - v_fragPos);
	vec3 halfAngle = normalize(viewDir + lightDir);
	float phongFac = pow(max(0, dot(norm, halfAngle)), 32);
	float geomFac = max(dot(norm, lightDir), 0);
	float specular = specularStrength * phongFac * geomFac;

	vec3 result = (ambient + diffuse + specular) * lightColor * color;
	FragColor = vec4(result, 1);
}