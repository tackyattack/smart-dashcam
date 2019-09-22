#extension GL_OES_EGL_image_external:require
uniform samplerExternalOES input_texture;
varying vec2 tcoord;
void main(void){
gl_FragColor = texture2D(camera_tex, tcoord);
}
