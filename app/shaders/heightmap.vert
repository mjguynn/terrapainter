
#version 430 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 tangent;

layout (location = 0) out vec3 v_normalDir;
layout (location = 1) out vec3 v_fragPos;
layout (location = 2) out vec3 color;
layout (location = 3) out vec2 texcoord;
layout (location = 4) out vec3 TangentLightDir;
layout (location = 5) out vec3 TangentViewPos;
layout (location = 6) out vec3 TangentFragPos;

layout (location = 2) uniform vec3 LightDir;
layout (location = 3) uniform vec3 viewPos;

layout (location = 0) uniform mat4 u_worldToProjection;
layout (location = 1) uniform mat4 u_modelToWorld;

vec3 c_smoothstep(vec3 c1, vec3 c2, float curh, float minh, float maxh) {
	float t = (curh - minh) / (maxh - minh);
	float v1 = t*t;
	float v2 = 1.0 - (1.0 - t) * (1.0 - t);
	return mix(c1, c2, mix(v1, v2, t));
}

vec3 getColorByHeight(float Height) {
	vec3 color;
	vec3 ppeak = vec3(213.0, 213.0, 213.0) / 255;
	vec3 peak = vec3(168.0,198.0,249.0) / 255;
	vec3 dirt = vec3(79.0, 68.0, 42.0) / 255;
	vec3 mtn = vec3(100.0, 100.0, 100.0) / 255;

	vec3 grass = vec3(72.0, 111.0, 56.0) / 255;
	vec3 sand = vec3(208.0,191.0,146.0) / 255;
	vec3 water = vec3(0.0, 117.0, 119.0) / 255;
	vec3 dwater = vec3(35.0, 55.0, 110.0) / 255;

	if (Height >= 70) {
		color = c_smoothstep(peak, ppeak, Height, 70.0, 80.0);
	} else if (Height >= 50.0) {
		color = c_smoothstep(mtn, peak, Height, 50.0, 70.0);
	} else if (Height >= 23.5) {
		color = c_smoothstep(dirt, mtn, Height, 23.5, 50.0);
	} else if (Height >= 3.5) {
		color = c_smoothstep(grass, dirt, Height, 3.5, 23.5);
	} else if (Height >= 0.0) {
		color = c_smoothstep(sand, grass, Height, 0, 3.5);
	} else if (Height >= -4.0) {
		color = c_smoothstep(water, sand, Height, -4, 0);
	} else {
		color = c_smoothstep(dwater, water, Height, -16, -4);
	}
	return color;
}

void main()
{
	texcoord = position.xy/4;
	vec4 normalDir = transpose(inverse(u_modelToWorld)) * vec4(normal, 0);
	v_normalDir = normalDir.xyz;
	vec3 T = (transpose(inverse(u_modelToWorld)) * vec4(tangent, 0)).xyz;
	vec3 B = cross(v_normalDir.xyz, T);

	mat3 TBN = transpose(mat3(T, B, v_normalDir));
	
	vec4 worldPos = u_modelToWorld * vec4(position, 1);
	v_fragPos = worldPos.xyz;
	color = getColorByHeight(v_fragPos.z);

	TangentLightDir = TBN * LightDir;
  TangentViewPos  = TBN * viewPos;
  TangentFragPos  = TBN * v_fragPos;

	gl_Position = u_worldToProjection * worldPos;
}