uniform sampler2D input_texture;
void main(void){
float x_pos = (gl_FragCoord.x)/1920.0;
float y_pos = (gl_FragCoord.y)/1080.0; // doing it this way scales it up to the full screen
vec2 pos = vec2(x_pos,y_pos);
vec4 input_color = texture2D(input_texture, pos);
float h = 0.0;
float v = 0.0;

h -= texture2D( input_texture, vec2( (gl_FragCoord.x - 1.0)/1920.0, (gl_FragCoord.y - 1.0)/1080.0 ) ).r * 1.0;
h -= texture2D( input_texture, vec2( (gl_FragCoord.x - 1.0)/1920.0, (gl_FragCoord.y)/1080.0    ) ).r * 2.0;
h -= texture2D( input_texture, vec2( (gl_FragCoord.x - 1.0)/1920.0, (gl_FragCoord.y + 1.0)/1080.0 ) ).r * 1.0;
h += texture2D( input_texture, vec2( (gl_FragCoord.x + 1.0)/1920.0, (gl_FragCoord.y - 1.0)/1080.0) ).r * 1.0;
h += texture2D( input_texture, vec2( (gl_FragCoord.x + 1.0)/1920.0, (gl_FragCoord.y)/1080.0     ) ).r * 2.0;
h += texture2D( input_texture, vec2( (gl_FragCoord.x + 1.0)/1920.0, (gl_FragCoord.y + 1.0)/1080.0 ) ).r * 1.0;
v -= texture2D( input_texture, vec2( (gl_FragCoord.x - 1.0)/1920.0, (gl_FragCoord.y - 1.0)/1080.0 ) ).r * 1.0;
v -= texture2D( input_texture, vec2( (gl_FragCoord.x)/1920.0, (gl_FragCoord.y - 1.0)/1080.0 ) ).r * 2.0;
v -= texture2D( input_texture, vec2( (gl_FragCoord.x + 1.0)/1920.0, (gl_FragCoord.y - 1.0)/1080.0 ) ).r * 1.0;
v += texture2D( input_texture, vec2( (gl_FragCoord.x - 1.0)/1920.0, (gl_FragCoord.y + 1.0)/1080.0 ) ).r * 1.0;
v += texture2D( input_texture, vec2( (gl_FragCoord.x)/1920.0, (gl_FragCoord.y + 1.0)/1080.0 ) ).r * 2.0;
v += texture2D( input_texture, vec2( (gl_FragCoord.x + 1.0)/1920.0, (gl_FragCoord.y + 1.0)/1080.0 ) ).r * 1.0;
float edge = sqrt((h * h) + (v * v));

gl_FragColor = vec4(edge, edge, edge, 1.0);
}
