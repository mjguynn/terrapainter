#version 430 core

layout (location = 2) uniform vec3 u_sunDir;
layout (location = 3) uniform vec3 u_viewPos;
layout (location = 4) uniform vec4 u_cullPlane;
layout (location = 5) uniform sampler2D mSand;
layout (location = 6) uniform sampler2D mGrass;
layout (location = 7) uniform sampler2D mDirt;
layout (location = 8) uniform sampler2D mMnt; // mountain
layout (location = 9) uniform sampler2D mSnow;
layout (location = 10) uniform sampler2D mGrassNorm;
layout (location = 11) uniform sampler2D mSandNorm;
layout (location = 12) uniform sampler2D mMountNorm;
layout (location = 13) uniform sampler2D mDirtNorm;
layout (location = 14) uniform sampler2D mSnowNorm;
layout (location = 15) uniform vec3 u_sunColor;

// Note that the GPU essentially linearly interpolates attributes
// when sending them to fragments. So this might not be a unit normal!
layout (location = 0) in vec3 v_normalDir;
layout (location = 1) in vec3 v_tangentDir;
layout (location = 2) in vec3 v_fragPos;
layout (location = 3) in vec2 v_texcoord;

out vec4 FragColor;

vec4 hash4( vec2 p ) { return fract(sin(vec4( 1.0+dot(p,vec2(37.0,17.0)), 
                                              2.0+dot(p,vec2(11.0,47.0)),
                                              3.0+dot(p,vec2(41.0,29.0)),
                                              4.0+dot(p,vec2(23.0,31.0))))*103.0); }

vec4 textureNoTile( sampler2D samp, in vec2 uv )
{
    vec2 iuv = floor( uv );
    vec2 fuv = fract( uv );

#ifdef USEHASH    
    // generate per-tile transform (needs GL_NEAREST_MIPMAP_LINEARto work right)
    vec4 ofa = texture( iChannel1, (iuv + vec2(0.5,0.5))/256.0 );
    vec4 ofb = texture( iChannel1, (iuv + vec2(1.5,0.5))/256.0 );
    vec4 ofc = texture( iChannel1, (iuv + vec2(0.5,1.5))/256.0 );
    vec4 ofd = texture( iChannel1, (iuv + vec2(1.5,1.5))/256.0 );
#else
    // generate per-tile transform
    vec4 ofa = hash4( iuv + vec2(0.0,0.0) );
    vec4 ofb = hash4( iuv + vec2(1.0,0.0) );
    vec4 ofc = hash4( iuv + vec2(0.0,1.0) );
    vec4 ofd = hash4( iuv + vec2(1.0,1.0) );
#endif
    
    vec2 ddx = dFdx( uv );
    vec2 ddy = dFdy( uv );

    // transform per-tile uvs
    ofa.zw = sign(ofa.zw-0.5);
    ofb.zw = sign(ofb.zw-0.5);
    ofc.zw = sign(ofc.zw-0.5);
    ofd.zw = sign(ofd.zw-0.5);
    
    // uv's, and derivarives (for correct mipmapping)
    vec2 uva = uv*ofa.zw + ofa.xy; vec2 ddxa = ddx*ofa.zw; vec2 ddya = ddy*ofa.zw;
    vec2 uvb = uv*ofb.zw + ofb.xy; vec2 ddxb = ddx*ofb.zw; vec2 ddyb = ddy*ofb.zw;
    vec2 uvc = uv*ofc.zw + ofc.xy; vec2 ddxc = ddx*ofc.zw; vec2 ddyc = ddy*ofc.zw;
    vec2 uvd = uv*ofd.zw + ofd.xy; vec2 ddxd = ddx*ofd.zw; vec2 ddyd = ddy*ofd.zw;
        
    // fetch and blend
    vec2 b = smoothstep(0.25,0.75,fuv);
    
    return mix( mix( textureGrad( samp, uva, ddxa, ddya ), 
                     textureGrad( samp, uvb, ddxb, ddyb ), b.x ), 
                mix( textureGrad( samp, uvc, ddxc, ddyc ),
                     textureGrad( samp, uvd, ddxd, ddyd ), b.x), b.y );
}

void lookup_properties(float height, out vec3 color, out vec3 normal) {
	const vec4 ppeak = vec4(213.0, 213.0, 213.0, 255) / 255;
	const vec4 water = vec4(0.0, 117.0, 119.0, 255) / 255;
	const vec4 dwater = vec4(35.0, 55.0, 110.0, 255) / 255;

	float blend;
	vec4 colorA;
	vec4 colorB;
	
	if (height >= 70) {
		blend = smoothstep(70.0f, 80.0f, height);
		colorA = textureNoTile( mSnow, v_texcoord );
		colorB = ppeak;
		normal = texture( mSnowNorm, v_texcoord ).xyz;
	} else if (height >= 50.0) {
		blend = smoothstep(50.0f, 70.0f, height);
		colorA = textureNoTile( mMnt, v_texcoord );
		colorB = textureNoTile( mSnow, v_texcoord );
		normal = texture( mMountNorm, v_texcoord ).xyz;
	} else if (height >= 23.5) {
		blend = smoothstep(23.5f, 50.0f, height);
		colorA = textureNoTile( mDirt, v_texcoord );
		colorB = textureNoTile( mMnt, v_texcoord );
		normal = texture( mDirtNorm, v_texcoord ).xyz;
	} else if (height >= 3.5) {
		blend = smoothstep(3.5f, 23.5f, height);
		colorA = textureNoTile( mGrass, v_texcoord );
		colorB = textureNoTile( mDirt, v_texcoord );
		normal = texture( mGrassNorm, v_texcoord ).xyz;
	} else if (height >= 0.0) {
		blend = smoothstep(0.f, 3.5f, height);
		colorA = textureNoTile( mSand, v_texcoord );
		colorB = textureNoTile( mGrass, v_texcoord );
		normal = texture( mSandNorm, v_texcoord ).xyz;
	} else if (height >= -4.0) {
		blend = smoothstep(-4.0f, 0.0f, height);
		colorA = water;
		colorB = textureNoTile( mSand, v_texcoord );
		normal = texture( mSandNorm, v_texcoord ).xyz;
	} else {
		blend = smoothstep(-16, -4, height);
		colorA = dwater;
		colorB = water;
		normal = texture( mSandNorm, v_texcoord ).xyz;
	}
	color = mix(colorA, colorB, blend).xyz;
}
vec3 adjust_normal(vec3 normalMap) {
	vec3 real = 2*normalMap - 1; // (real)
	vec3 normal = normalize(v_normalDir);
	vec3 tangent = normalize(v_tangentDir);
	vec3 bitangent = cross(v_normalDir, v_tangentDir);
	
	mat3 tbn = mat3(bitangent, tangent, normal);
	return normalize(tbn * real);
}
void main()
{
	if (dot(vec4(v_fragPos, 1), u_cullPlane) < 0)
		discard;

	float height = v_fragPos.z;

	vec3 color;
	vec3 normalMap;
	float phongScale;
	lookup_properties(height, color, normalMap);
	
	vec3 norm = adjust_normal(normalMap);
	
	vec3 lightColor = u_sunColor;
	
	// ambiance
	float ambient = 0.2;

	// diffuse
	vec3 lightDir = normalize(u_sunDir);
	float diffuse = 0.6*max(dot(norm, lightDir), 0);

	// specular
	float specularStrength = 0.5 * (
		clamp(v_fragPos.z - 65, 0, 3) / 3
		+ 4*clamp(1-v_fragPos.z, 0, 1)
	);
	vec3 viewDir = normalize(u_viewPos - v_fragPos);
	vec3 halfAngle = normalize(viewDir + lightDir);
	float phongFac = pow(max(0, dot(norm, halfAngle)), 32);
	float geomFac = max(dot(norm, lightDir), 0);
	float specular = specularStrength * phongFac * geomFac;

	vec3 result = (ambient + diffuse + specular) * lightColor * color;

	FragColor = vec4(result, 1.0);
}

