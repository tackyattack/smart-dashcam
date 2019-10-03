#extension GL_OES_EGL_image_external:require
uniform samplerExternalOES input_texture;
uniform samplerExternalOES input_texture_y;
uniform samplerExternalOES input_texture_u;
uniform samplerExternalOES input_texture_v;
varying vec2 tcoord;

vec3 rgb2hsl(vec3 c){
  float h = 0.0;
	float s = 0.0;
	float l = 0.0;
	float r = c.r;
	float g = c.g;
	float b = c.b;
	float cMin = min( r, min( g, b ) );
	float cMax = max( r, max( g, b ) );

	l = ( cMax + cMin ) / 2.0;
	if ( cMax > cMin ) {
		float cDelta = cMax - cMin;

        //s = l < .05 ? cDelta / ( cMax + cMin ) : cDelta / ( 2.0 - ( cMax + cMin ) ); Original
		s = l < .0 ? cDelta / ( cMax + cMin ) : cDelta / ( 2.0 - ( cMax + cMin ) );

		if ( r == cMax ) {
			h = ( g - b ) / cDelta;
		} else if ( g == cMax ) {
			h = 2.0 + ( b - r ) / cDelta;
		} else {
			h = 4.0 + ( r - g ) / cDelta;
		}

		if ( h < 0.0) {
			h += 6.0;
		}
		h = h / 6.0;
	}
	return vec3( h, s, l );
}

vec3 rgb2hsv(vec3 c)
{
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

void main(void){
float x_pos = tcoord.x;
float y_pos = tcoord.y*(1640.0/922.0);
vec4 rgb_color;
float y;
float u;
float v;
if(y_pos < 1.0)
{
  y = texture2D(input_texture_y, vec2(x_pos, y_pos)).r;
  u = texture2D(input_texture_u, vec2(x_pos, y_pos)).r;
  v = texture2D(input_texture_v, vec2(x_pos, y_pos)).r;

  // YUV to RGB
  rgb_color.r = clamp((y + (1.370705 * (v-0.5))), 0.0, 1.0);
  rgb_color.g = clamp((y - (0.698001 * (v-0.5)) - (0.337633 * (u-0.5))), 0.0, 1.0);
  rgb_color.b = clamp((y + (1.732446 * (u-0.5))), 0.0, 1.0);
  rgb_color.a = 1.0;

  vec3 rgb;
  rgb.r = rgb_color.r;
  rgb.g = rgb_color.g;
  rgb.b = rgb_color.b;


  // Lane color space
  vec3 hsv;
  vec3 hsl;


  // HSL best: saturation
  // But if saturation is low, then lightness (true white) helps
  hsl = rgb2hsl(rgb);

  float s = hsl.y;
  float l = hsl.z;
  float split = 0.8;
  float lane_value = s*split + l*(1.0-split);

  gl_FragColor = vec4(lane_value, lane_value, lane_value, 1.0);

}
else
{
  gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
}
}
