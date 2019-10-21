uniform sampler2D input_texture;
uniform float top_right_y;
uniform float bottom_left_y;
uniform float transform_angle;
uniform mat3 transform_matrix;
void main(void){
float bottom_left_x = 0.0;
float top_right_x = 1024.0;
if(!(gl_FragCoord.x > bottom_left_x && gl_FragCoord.x < top_right_x && gl_FragCoord.y < top_right_y && gl_FragCoord.y > bottom_left_y))
{
return;
}
float width = top_right_x - bottom_left_x;
float height = top_right_y - bottom_left_y;
float current_x = gl_FragCoord.x - bottom_left_x;
float current_y = top_right_y - gl_FragCoord.y;
current_y = current_y;



float w = width;
float h = height;

float u = current_x;
float v = current_y;

vec3 pixel_pos = vec3(u,v,1.0);
vec3 world_pos = transform_matrix*pixel_pos;

float input_x = world_pos.x/world_pos.z + w/2.0;
float input_y = world_pos.y/world_pos.z + h/2.0;


float actual_x = input_x;
float actual_y = (top_right_y - input_y);

vec4 color_out = vec4(0.0, 0.0, 0.0, 1.0);
if ((actual_y > 0.0) && (actual_y < top_right_y) && (input_x > 0.0) && (input_x < top_right_x))
{
  color_out = texture2D(input_texture, vec2(actual_x/1024.0, actual_y/1024.0));
}
gl_FragColor = color_out;

}
