#version 430 core

in vec2 uv;
uniform float iTime;

out vec4 fragColor;

const float cloudscale = 10.1;
const float speed = 0.01;
const float clouddark = 0.5;
const float cloudlight = 0.3;
const float cloudcover = 0.3;
const float cloudalpha = 8.0;
const float skytint = 0.5;
const vec3 skycolour = vec3(75.,107.,146.) / 255;

const mat2 m = mat2( 1.6,  1.2, -1.2,  1.6 );

float random2(vec2 p) {
  return fract(sin(dot(p, vec2(12.121213, 4.1231))) * 43256.12039);
}

vec2 random2_2(vec2 p) {
  return 2.0 * fract(
  sin(
      vec2(
          dot(p, vec2(12.121213, 15.1231)),
          dot(p, vec2(2.9383, 8.1234))
          )) * 43256.12039) - 1.0;
}

float noise(vec2 p) {
  vec2 i_pos = floor(p);
  vec2 f_pos = fract(p);
      
  float col = 0.0;
  
  // value noise
  //float ll = random2(i_pos);
  //float lr = random2(i_pos + vec2(1.0, 0.0));
  //float ul = random2(i_pos + vec2(0.0, 1.0));
  //float ur = random2(i_pos + vec2(1.0, 1.0));
  
  // gradient noise
  float ll = dot(random2_2(i_pos), f_pos);
  float lr = dot(random2_2(i_pos + vec2(1.0, 0.0)), f_pos - vec2(1.0, 0.0));
  float ul = dot(random2_2(i_pos + vec2(0.0, 1.0)), f_pos - vec2(0.0, 1.0));
  float ur = dot(random2_2(i_pos + vec2(1.0, 1.0)), f_pos - vec2(1.0, 1.0));
      
  // cubic interpolation (smoothstep)
  vec2 u = f_pos*f_pos*(3.0-2.0*f_pos);
  
  // quintic interpolation
  //vec2 u = f_pos * f_pos * f_pos * (6.0 * f_pos * f_pos - 15. * f_pos + 10.);
  
  col = mix (
          mix( ll, lr, u.x),
          mix( ul, ur, u.x),
          u.y);
  
  return col;
}

float fbm (in vec2 p ) {
  const int octaves = 3;
  float lacunarity = 2.0;
  float gain = 0.5;

  float amp = 0.5;
  float freq = 1.;
  
  float h = 0.;
  
  for (int i = 0; i <= octaves; i++) {
      float b = noise(freq * p);
      h += amp * b;
      freq *= lacunarity;
      amp *= gain;
  }
  
  return h/2.;
}

float anoise(vec2 uv, float w, float sp, float sc, float q) {
  float r = 0.0;
  float time = iTime * speed * sp;
	uv *= cloudscale * sc;
  uv -= q - time;
  for (int i=0; i<7; i++){
    r += abs(w*noise(uv));
    uv = m*uv + time;
    w *= 0.7;
  }
  return r;
}

float snoise(vec2 uv, float w, float sp, float sc, float q) {
  float r = 0.0;
  float time = iTime * speed * sp;
	uv *= cloudscale * sc;
  uv -= q - time;
  for (int i=0; i<7; i++){
    r += (w*noise(uv));
    uv = m*uv + time;
    w *= 0.6;
  }
  return r;
}

void main() {

  float time = iTime * speed;
  float q = fbm(uv * cloudscale * 0.5);

	//ridged noise shape
  float r = anoise(uv, .8, 1., 1., q);
  float f = snoise(uv, 0.7, 1., 1., q);
  
  f *= r + f;
  
  //noise colour
  float c = snoise(uv, .4, 2., 2., q);
  float c1 = anoise(uv, .4, 3., 3., q);

  c += c1;
  
  vec3 cloudcolour = vec3(1.1, 1.1, 0.9) * clamp((clouddark + cloudlight*c), 0.0, 1.0);
  f = cloudcover + cloudalpha*f*r;

  // 0 treated as sky, 1 treated as cloud
  float intensity = clamp(f + c, 0.0, 1.0);
  
  // mix with skycolor and cloud color
  vec3 result = mix(skycolour, clamp(skytint * skycolour + cloudcolour, 0.0, 1.0), intensity);

  // Set alpha lower around edges
  float d = length(2.*uv - 1.);
  float al = smoothstep(0.4, 0.2, d);
  
	fragColor = vec4( result, smoothstep(0.0, 1.0, al*sqrt(intensity)));
}