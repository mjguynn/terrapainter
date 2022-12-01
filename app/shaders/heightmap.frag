#version 430 core

layout (location = 2) uniform vec3 LightDir;
layout (location = 3) uniform vec3 viewPos;
layout (location = 4) uniform vec4 u_cullPlane;
layout (location = 5) uniform sampler2D mSand; // sand
layout (location = 6) uniform sampler2D mGrass; // grass
layout (location = 7) uniform sampler2D mDirt; // dirt
layout (location = 8) uniform sampler2D mMnt; // mountain
layout (location = 9) uniform sampler2D mSnow;
layout (location = 10) uniform sampler2D mGrassNorm;
layout (location = 11) uniform sampler2D mSandNorm;
layout (location = 12) uniform sampler2D mMountNorm;
layout (location = 13) uniform sampler2D mDirtNorm;
layout (location = 14) uniform sampler2D mSnowNorm;

layout (location = 15) uniform bool blinn;


// Note that the GPU essentially linearly interpolates attributes
// when sending them to fragments. So this might not be a unit normal!
layout (location = 0) in vec3 v_normalDir;
layout (location = 1) in vec3 v_fragPos; // in world space
layout (location = 2) in vec3 color;
layout (location = 3) in vec2 texcoord;

layout (location = 4) in vec3 TangentLightDir;
layout (location = 5) in vec3 TangentViewPos;
layout (location = 6) in vec3 TangentFragPos;

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


void main()
{
	if (dot(vec4(v_fragPos, 1), u_cullPlane) < 0)
		discard;

	vec3 LightColor = vec3(1.0, 1.0, 1.0);

	// Colors for if we don't have the texture
	vec3 ppeak = vec3(213.0, 213.0, 213.0) / 255;
	vec3 peak = textureNoTile( mSnow, texcoord ).xyz;
	vec3 mtn = textureNoTile( mMnt, texcoord ).xyz;
	vec3 dirt = textureNoTile( mDirt, texcoord ).xyz;
	vec3 grass = textureNoTile( mGrass, texcoord ).xyz;
	vec3 sand = textureNoTile( mSand, texcoord ).xyz;
	vec3 water = vec3(0.0, 117.0, 119.0) / 255;
	vec3 dwater = vec3(35.0, 55.0, 110.0) / 255;

	vec3 texture_color;
	vec3 normal;
  float Height = v_fragPos.z;

	if (Height >= 70) {
		float blend = smoothstep(70.0, 80.0, Height);
		texture_color = peak * (1 - blend) + ppeak * blend;
		normal = texture(mSnowNorm, texcoord).rbg;
	} 
	else if (Height >= 50.0) {
		float blend = smoothstep(50.0, 70.0, Height);
		texture_color = mtn * (1 - blend) + peak * blend;
		normal = texture(mMountNorm, texcoord).rbg;
	} 
	else if (Height >= 23.5) {
		float blend = smoothstep(23.5, 50.0, Height);
		texture_color = dirt * (1 - blend) + mtn * blend;
		normal = texture(mDirtNorm, texcoord).rbg;
	} 
	else if (Height >= 3.5) {
		float blend = smoothstep(3.5, 23.5, Height);
		texture_color = grass * (1 - blend) + dirt * blend;
		normal = texture(mGrassNorm, texcoord).rbg;
	} 
	else if (Height >= 0.0) {
		float blend = smoothstep(0.0, 3.5, Height);
		texture_color = sand * (1 - blend) + grass * blend;
		normal = texture(mSandNorm, texcoord).rbg;
	} else {
		normal = vec3(0., 0., 1.);
	}

	// ambiance
	float ambientStrength = 0.05;
	vec3 ambientColor = ambientStrength * LightColor;

	// obtain normal from normal map in range [0,1]

	// diffuse
	vec3 norm = normalize(normal);
	vec3 lightDir = normalize(TangentLightDir);
	float diff = max(dot(norm, -lightDir), 0.0);
	vec3 diffuseColor = diff * LightColor;

	// specular
	float specularStrength = 1;
	if (v_fragPos.z <= 0 || v_fragPos.z >= 65) {
		specularStrength = 0.5;
	}
	vec3 viewDir = normalize(TangentViewPos - TangentFragPos);
	vec3 reflectDir = reflect(lightDir, norm);

	 float spec = 0.0;
    if(blinn)
    {
        vec3 halfwayDir = normalize(-lightDir + viewDir);  
        spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0);
    }
    else
    {
        vec3 reflectDir = reflect(lightDir, norm);
        spec = pow(max(dot(viewDir, reflectDir), 0.0), 8.0);
    }
	
	vec3 specularColor =  specularStrength * spec * LightColor;

	vec3 result_color;
	result_color = (ambientColor + diffuseColor + specularColor) * (color*0.3 + texture_color*0.7);	
	if (Height <= 0) {
		result_color = (ambientColor + diffuseColor + specularColor) * color;	
	}

	float gamma = 1.0;
  result_color = pow(result_color, vec3(1.0/gamma));
	FragColor = vec4(result_color, 1.0);
}

