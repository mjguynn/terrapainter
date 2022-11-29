#version 430 core

in vec2 vTexCoord;
out vec4 outputColor;

layout (location = 4) uniform sampler2D gSampler;
layout (location = 5) uniform vec4 vColor;

layout (location = 6) uniform float fAlphaTest;
layout (location = 7) uniform float fAlphaMultiplier;

void main()
{
	vec4 vTexColor = texture(gSampler, vTexCoord);
	float fNewAlpha = vTexColor.a*fAlphaMultiplier;               
	if(fNewAlpha < fAlphaTest)
		discard;
	
	if(fNewAlpha > 1.0f)
		fNewAlpha = 1.0f;	
		
	vec4 vMixedColor = vTexColor*vColor; 
	
	outputColor = vec4(vMixedColor.zyx, fNewAlpha);
}