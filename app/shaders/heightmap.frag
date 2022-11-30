#version 430 core

layout (location = 2) uniform vec3 u_sunDir;
layout (location = 3) uniform vec3 viewPos;
layout (location = 4) uniform vec4 u_cullPlane;
layout (location = 5) uniform vec3 u_sunColor;

// Note that the GPU essentially linearly interpolates attributes
// when sending them to fragments. So this might not be a unit normal!
layout (location = 0) in vec3 v_normalDir;
layout (location = 1) in vec3 v_fragPos; // in world space

out vec4 FragColor;

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
	if (dot(vec4(v_fragPos, 1), u_cullPlane) < 0)
		discard;

	vec3 color = getColorByHeight(v_fragPos.z);

	vec3 lightColor = u_sunColor;
	
	float depthFac = clamp( -v_fragPos.z/32, 0, 1);
	vec3 norm = normalize(mix(v_normalDir, vec3(0, 0, 3), depthFac));
	
	// ambiance
	float ambient = 0.2;

	// diffuse
	vec3 lightDir = normalize(u_sunDir);
	float diffuse = 0.6*max(dot(norm, lightDir), 0) - depthFac/2;

	// specular
	float specularStrength = 0.5 * clamp(v_fragPos.z - 65, 0, 3) / 3;
	vec3 viewDir = normalize(viewPos - v_fragPos);
	vec3 halfAngle = normalize(viewDir + lightDir);
	float phongFac = pow(max(0, dot(norm, halfAngle)), 32);
	float geomFac = max(dot(norm, lightDir), 0);
	float specular = specularStrength * phongFac * geomFac;

	color = (ambient + diffuse + specular) * lightColor * color;	

	FragColor = vec4(color, 1.0);
}