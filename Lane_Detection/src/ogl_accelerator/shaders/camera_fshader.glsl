#extension GL_OES_EGL_image_external:require
uniform samplerExternalOES input_texture;
varying vec2 tcoord;
void main(void){
float x = tcoord.x;
float y = tcoord.y*(1640.0/922.0);
if(y < 1.0)
{
  gl_FragColor = texture2D(input_texture, vec2(x, y));
}
else
{
  gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
}
}
