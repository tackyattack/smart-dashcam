uniform sampler2D input_texture;
uniform float top_right_y;
varying vec2 tcoord;
void main(void){
float bottom_left_x = 0.0;
float bottom_left_y = 0.0;
float top_right_x = 1024.0;
//float top_right_y = 500.0;
if(!(gl_FragCoord.x > bottom_left_x && gl_FragCoord.x < top_right_x && gl_FragCoord.y < top_right_y && gl_FragCoord.y > bottom_left_y))
{
return;
}


float width = top_right_x - bottom_left_x;
float height = top_right_y - bottom_left_y;
float current_x = gl_FragCoord.x - bottom_left_x;
float current_y = gl_FragCoord.y - bottom_left_y;

vec4 color_out;
vec2 pos;
float accum = 0.0;
float kernel_size = 10.0;
for(float i = -5.0; i < 5.0; i = i + 1.0)
{
  pos = vec2((current_x)/1024.0, (current_y + i)/1024.0);
  accum += texture2D(input_texture, pos).r*1.0;
}


accum = accum / (kernel_size);
color_out = vec4(accum,accum,accum,1.0);
gl_FragColor = color_out;
}
