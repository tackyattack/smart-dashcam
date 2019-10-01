uniform sampler2D input_texture;
uniform float top_right_y;
uniform float bottom_left_y;
void main(void)
{
  float bottom_left_x = 0.0;
  float top_right_x = 1024.0;
  if(!(gl_FragCoord.x > bottom_left_x && gl_FragCoord.x < top_right_x && gl_FragCoord.y < top_right_y && gl_FragCoord.y > bottom_left_y))
  {
  return;
  }
  float width = top_right_x - bottom_left_x;
  float height = top_right_y - bottom_left_y;

  float stride = (top_right_y - bottom_left_y)/20.0;

  float accumulator = 0.0;
  for(float i = 0.0; i < 20.0; i++)
  {
    accumulator+= texture2D(input_texture, vec2((gl_FragCoord.x)/1024.0, (gl_FragCoord.y + bottom_left_y + stride*i)/1024.0)).r;
  }
  accumulator = accumulator/10.0;
  gl_FragColor = vec4(accumulator, accumulator, accumulator, 1.0);


}
