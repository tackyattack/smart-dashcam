uniform sampler2D input_texture;
void main(void){
float blur_size = 10.0;
float x_pos = (gl_FragCoord.x)/1920.0;
float y_pos = (gl_FragCoord.y)/1080.0; // doing it this way scales it up to the full screen
vec2 pos = vec2(x_pos,y_pos);
vec4 input_color = texture2D(input_texture, pos);
vec2 start_pos = vec2(gl_FragCoord.x - blur_size, gl_FragCoord.y - blur_size);
vec2 current_pos;
float conv_accumulator = 0.0;
for(float i = 0.0; i < 3.0; i = i + 1.0)
{
  for(float j = 0.0; j < 3.0; j = j + 1.0)
  {
    current_pos = vec2(start_pos.x + i*blur_size, start_pos.y + j*blur_size);
    if(current_pos.x < 0.0) current_pos.x = 0.0;
    if(current_pos.x > 1920.0) current_pos.x = 1920.0;
    if(current_pos.y < 0.0) current_pos.y = 0.0;
    if(current_pos.y > 1080.0) current_pos.y = 1080.0;
    pos = vec2(current_pos.x/1920.0, current_pos.y/1080.0);
    vec4 input_color = texture2D(input_texture, pos);
    conv_accumulator = conv_accumulator + input_color.r;
  }
}

float r_color = conv_accumulator/9.0;
vec4 color_out = vec4(r_color, 0.0, 0.0, 1.0);
gl_FragColor = color_out;
}
