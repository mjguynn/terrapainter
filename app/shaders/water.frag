#version 430 core


layout (location = 3) uniform sampler2D u_reflectionTexture;
layout (location = 4) uniform ivec2 u_screenSize;
layout (location = 5) uniform vec4 u_cullPlane;

layout (location = 0) in vec3 v_normalDir;
layout (location = 1) in vec3 v_fragPos; // in world space

out vec4 o_color;

void main()
{
	if (dot(vec4(v_fragPos, 1), u_cullPlane) < 0)
		discard;
	
	vec2 coords = vec2(gl_FragCoord) / vec2(u_screenSize);
	vec4 refl = texture(u_reflectionTexture, coords);
	o_color = vec4(refl.rgb, 0.6);
}