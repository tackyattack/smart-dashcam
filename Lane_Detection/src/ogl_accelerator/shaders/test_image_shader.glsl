uniform sampler2D tex;
void main(void){
vec4 color_out = vec4(0,0,0,1);
float x_pos = (gl_FragCoord.x)/1920.0;
float y_pos = (gl_FragCoord.y)/1080.0; // doing it this way scales it up to the full screen
                                         // dividing it by 128 instead would scale it so it's the actual size
// if(gl_FragCoord.x < 128.0 && gl_FragCoord.y < 128.0){
// vec2 pos = vec2(x_pos,y_pos);
// color_out = texture2D(tex, pos);
// }
vec2 pos = vec2(x_pos,y_pos);
color_out = texture2D(tex, pos);
//color_out = vec4(x_pos, y_pos, 0, 1);
float r_color = color_out.r;
float g_color = color_out.g;
float b_color = color_out.b;
vec4 color_out2 = vec4(r_color, g_color, b_color, 1.0);
gl_FragColor = color_out2;
}
