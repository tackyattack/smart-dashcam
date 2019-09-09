// used for taking the texture and rendering it to a FBO
uniform sampler2D input_texture;
void main(void){
vec4 color_out = vec4(0,0,0,1);
float x_pos = (gl_FragCoord.x)/1920.0;
float y_pos = (gl_FragCoord.y)/1080.0;
vec2 pos = vec2(x_pos,y_pos);
color_out = texture2D(input_texture, pos);
gl_FragColor = color_out;
}
