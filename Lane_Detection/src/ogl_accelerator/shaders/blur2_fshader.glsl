uniform sampler2D input_texture;
void main(void){
   float blur;
   float blur_Size = 1.0;
   float blurSize = 1.0/blur_Size; // I've chosen this size because this will result in that every step will be one pixel wide if the texture is of size 512x512
  float sum;
  sum += texture2D(input_texture, vec2(gl_FragCoord.x - 4.0*blurSize, gl_FragCoord.y)).r;
  sum += texture2D(input_texture, vec2(gl_FragCoord.x - 3.0*blurSize, gl_FragCoord.y)).r;
          sum += texture2D(input_texture, vec2(gl_FragCoord.x - 2.0*blurSize, gl_FragCoord.y)).r;
          sum += texture2D(input_texture, vec2(gl_FragCoord.x - blurSize, gl_FragCoord.y)).r;
          sum += texture2D(input_texture, vec2(gl_FragCoord.x, gl_FragCoord.y)).r;
          sum += texture2D(input_texture, vec2(gl_FragCoord.x + blurSize, gl_FragCoord.y)).r;
          sum += texture2D(input_texture, vec2(gl_FragCoord.x + 2.0*blurSize, gl_FragCoord.y)).r;
          sum += texture2D(input_texture, vec2(gl_FragCoord.x + 3.0*blurSize, gl_FragCoord.y)).r;
          sum += texture2D(input_texture, vec2(gl_FragCoord.x + 4.0*blurSize, gl_FragCoord.y)).r;

          sum += texture2D(input_texture, vec2(gl_FragCoord.x, gl_FragCoord.y - 4.0*blurSize)).r;
          sum += texture2D(input_texture, vec2(gl_FragCoord.x, gl_FragCoord.y - 3.0*blurSize)).r;
          sum += texture2D(input_texture, vec2(gl_FragCoord.x, gl_FragCoord.y - 2.0*blurSize)).r;
          sum += texture2D(input_texture, vec2(gl_FragCoord.x, gl_FragCoord.y - blurSize)).r;
          sum += texture2D(input_texture, vec2(gl_FragCoord.x, gl_FragCoord.y)).r;
          sum += texture2D(input_texture, vec2(gl_FragCoord.x, gl_FragCoord.y + blurSize)).r;
          sum += texture2D(input_texture, vec2(gl_FragCoord.x, gl_FragCoord.y + 2.0*blurSize)).r;
          sum += texture2D(input_texture, vec2(gl_FragCoord.x, gl_FragCoord.y + 3.0*blurSize)).r;
          sum += texture2D(input_texture, vec2(gl_FragCoord.x, gl_FragCoord.y + 4.0*blurSize)).r;
          gl_FragColor = vec4(sum/14.0, 0,0,1);

}
