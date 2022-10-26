#version 450

// Simple alpha mix of colors
const int TP_CANVAS_BLEND_MIX = 0;
const int TP_CANVAS_BLEND_ADD = 1;
const int TP_CANVAS_BLEND_SUB = 2;

// The selected blend mode
uniform int u_blendMode;
// The RGB base texture
uniform sampler2D u_baseTexture;
// An alpha mask for a layer which will be composited over the base texture
uniform sampler2D u_layerMask;
// The "tint" of the composited layer, i.e. it's color, before blending mode.
uniform vec3 u_layerTint;

layout(location = 0) in vec2 v_texCoord;
layout(location = 0) out vec4 o_color;

void main(){
	float fac = texture(u_layerMask, v_texCoord).r;
	vec3 base = texture(u_baseTexture, v_texCoord).rgb;
	vec3 layer = u_layerTint;
	
	vec3 blended;
	switch (u_blendMode) {
		case TP_CANVAS_BLEND_MIX:
			blended = mix(base, layer, fac);
			break;
		case TP_CANVAS_BLEND_ADD:
			blended = base + fac * layer;
			break;
		case TP_CANVAS_BLEND_SUB:
			blended = base - fac * layer;
			break;
		default:
			blended = vec3(1.0, 0.0, 1.0);
			break;
	}
	
	o_color = vec4(blended.rgb, 1.0);
}