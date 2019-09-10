uniform sampler2D input_texture;
varying vec2 tcoord;
void main(void){
vec4 color_out;
vec2 pos;
float accum = 0.0;
float kernel_size = 10.0;
for(float i = -10.0; i < 10.0; i = i + 1.0)
{
  pos = vec2((gl_FragCoord.x + i)/1920.0, (gl_FragCoord.y)/1080.0);
  accum += texture2D(input_texture, pos).r*1.0;
}
accum = accum / (kernel_size*2.0);
color_out = vec4(accum, 0,0,1);
gl_FragColor = color_out;
}
