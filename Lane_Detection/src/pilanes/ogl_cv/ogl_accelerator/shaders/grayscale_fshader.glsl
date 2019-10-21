uniform sampler2D input_texture;
void main(void){
float x_pos = (gl_FragCoord.x)/1920.0;
float y_pos = (gl_FragCoord.y)/1080.0; // doing it this way scales it up to the full screen
vec2 pos = vec2(x_pos,y_pos);
vec4 input_color = texture2D(input_texture, pos);
float grayscale = (input_color.r)*0.3 + (input_color.g)*0.59 + (input_color.b)*0.11;
float r_color = grayscale; // store it in red
float g_color = 0.0;
float b_color = 0.0;
vec4 color_out = vec4(r_color, g_color, b_color, 1.0);
gl_FragColor = color_out;
}
