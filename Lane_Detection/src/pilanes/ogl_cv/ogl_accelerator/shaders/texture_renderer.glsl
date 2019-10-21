// used for taking the texture and rendering it to a FBO
uniform sampler2D input_texture;
uniform int fps_state;
void main(void)
{
  vec4 color_out = vec4(0.0,0.0,0.0,1.0);
  float x_pos = (gl_FragCoord.x)/1024.0;
  float y_pos = (gl_FragCoord.y)/1024.0;
  vec2 pos = vec2(x_pos, y_pos);
  color_out = texture2D(input_texture, pos);
  color_out = vec4(color_out.r, color_out.g, color_out.b, 1.0);
  if((gl_FragCoord.x < 25.0) && (gl_FragCoord.y < 25.0))
  {
    if(fps_state == 1)
    {
    color_out = vec4(1.0, 0.0, 0.0, 1.0);
    }
    else
    {
    color_out = vec4(0.0, 0.0, 1.0, 1.0);
    }
  }
  gl_FragColor = color_out;
}
