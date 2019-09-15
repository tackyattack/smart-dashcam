uniform sampler2D input_texture;
void main(void){
float H = 2.0; // height off the ground
float theta = 1.0; // angle of camera
float f = 1.05; // focal length
float y = (f*sin(theta)*gl_FragCoord.y - H*f*cos(theta)) / (H*sin(theta) + cos(theta)*gl_FragCoord.y);
float x = (f*sin(theta)*gl_FragCoord.x - y*cos(theta)*gl_FragCoord.x - H*f*cos(theta)) / (H*sin(theta));
x = x / 1920.0;
y = y / 1080.0;
vec4 color_out = texture2D(input_texture, vec2(x,y));
gl_FragColor = color_out;
}
