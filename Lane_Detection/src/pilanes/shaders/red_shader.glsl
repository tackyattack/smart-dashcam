void main(void){
vec4 color_out = vec4(1,0,0,1);
if(gl_FragCoord.y > 540.0)
{
color_out = vec4(0,1,0,1);
}
gl_FragColor = color_out;
}
