uniform sampler2D input_texture;
void main(void){
float bottom_left_x = 0.0;
float bottom_left_y = 0.0;
float top_right_x = 1920.0;
float top_right_y = 500.0;
if(!(gl_FragCoord.x > bottom_left_x && gl_FragCoord.x < top_right_x && gl_FragCoord.y < top_right_y && gl_FragCoord.y > bottom_left_y))
{
return;
}
float width = top_right_x - bottom_left_x;
float height = top_right_y - bottom_left_y;
float current_x = gl_FragCoord.x - bottom_left_x;
float current_y = gl_FragCoord.y - bottom_left_y;
current_y = top_right_y - current_y;


//it's probably better to overestimate the viewing space
// so the angle is stronger (lanes will curve outward more then inward)

// calculate angle based on inverse tan of
// mounting height and distance away from top of region of interest
// then use sx to scale vertically to top and bottom (based on rotation angle in 3D space)
// Use sy to pull it out to the edges (can use distance to camera based on angle to determine how much)
// f can be arbitrary
float t = 80.0*3.14159/180.0;
float sx = 18.0/cos(t); // projection to scale back up to useable size
float sy = 0.4;
float f = 0.5;


float w = width;
float h = height;

float zshift = -500.0;
float vshift = -250.0;
float u = current_x;
float v = current_y;
float input_x = (2.0*h*vshift*w*sin(t) - 4.0*h*u*vshift*sin(t) + h*sy*w*w*sin(t) - 2.0*v*sy*w*w*sin(t) - 2.0*h*sx*w*zshift*cos(t) + 4.0*h*u*sx*zshift*cos(t) + f*h*sx*sy*w*w*cos(t))/(2.0*sy*w*(h*sin(t) - 2.0*v*sin(t) + f*h*sx*cos(t)));
float input_y = (h*h*sin(t) + 2.0*h*zshift - 4.0*v*zshift - 2.0*h*v*sin(t) + 2.0*f*h*vshift + f*h*h*sx*cos(t))/(2.0*h*sin(t) - 4.0*v*sin(t) + 2.0*f*h*sx*cos(t));

float actual_x = input_x + bottom_left_x;
float actual_y = input_y + bottom_left_y;
actual_y = height - actual_y;

vec4 color_out = vec4(0.0, 0.0, 0.0, 1.0);
if ((actual_y > 0.0) && (actual_y < top_right_y) && (input_x > 0.0) && (input_x < top_right_x))
{
  color_out = texture2D(input_texture, vec2(actual_x/1920.0, actual_y/1080.0));
}
gl_FragColor = color_out;

}