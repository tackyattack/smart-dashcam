uniform sampler2D input_texture;
void main(void)
{
  i
  vec4 color_out = vec4(0.0,0.0,0.0,1.0);
  float x_pos = (gl_FragCoord.x)/1920.0*2.0;
  float y_pos = (gl_FragCoord.y)/1080.0*2.0;
  vec2 pos = vec2(x_pos, y_pos);
  color_out = texture2D(input_texture, pos);
  color_out = vec4(color_out.r, color_out.g, color_out.b, 1.0);
  gl_FragColor = color_out;
}
