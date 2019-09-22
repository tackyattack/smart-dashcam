uniform sampler2D input_texture;
void main(void)
{
  float bottom_left_x = 0.0;
  float bottom_left_y = 0.0;
  float top_right_x = 1920.0;
  float top_right_y = 500.0;
  if(!(gl_FragCoord.x > bottom_left_x && gl_FragCoord.x < top_right_x && gl_FragCoord.y < top_right_y && gl_FragCoord.y > bottom_left_y))
  {
  return;
  }
  float width = top_right_x - bottom_left_x;
  float height = top_right_y - bottom_left_y;
  float current_x = gl_FragCoord.x - bottom_left_x;
  float current_y = gl_FragCoord.y - bottom_left_y;

  float stride = (top_right_y - bottom_left_y)/20.0;
  if(current_y > stride)
  {
    gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    return;
  }

  float accumulator = 0.0;
  for(float i = 0.0; i < 20.0; i++)
  {
    accumulator+= texture2D(input_texture, vec2((current_x)/1920.0, (current_y+stride*i)/1080.0)).r;
  }
  //accumulator = accumulator/20.0;
  // if(accumulator < 0.1)
  // {
  //   accumulator = 0.0;
  // }
  // else
  // {
  //   accumulator = 1.0;
  // }
  gl_FragColor = vec4(accumulator, accumulator, accumulator, 1.0);


}
