uniform sampler2D input_texture;
void main(void){
vec4 color_out = vec4(0,0,0,1);
// if(gl_FragCoord.x > 300.0)
// {
// return;
// }
// if(gl_FragCoord.y > 300.0)
// {
// return;
// }
//float x_pos = (gl_FragCoord.x+500.0)/1920.0;
//float y_pos = (gl_FragCoord.y+500.0)/1080.0;
float x_pos = (gl_FragCoord.x+100.0)/1920.0;
float y_pos = (gl_FragCoord.y+100.0)/1080.0;
vec2 pos = vec2(x_pos,y_pos);
color_out = texture2D(input_texture, pos);
color_out = vec4(color_out.r, color_out.r, color_out.r, 1);
//color_out = vec4(1, 0,0,1);
gl_FragColor = color_out;
}
