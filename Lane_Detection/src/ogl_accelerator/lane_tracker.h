// Henry Bergin 2019
// Current pipeline:
  // pipeline_shaders[0].fragment_shader_path = concat(shader_dir_path, "/camera_fshader.glsl");
  // pipeline_shaders[0].num_vars = 0;
  // pipeline_shaders[1].fragment_shader_path = concat(shader_dir_path, "/birds_eye_fshader.glsl");
  // pipeline_shaders[1].num_vars = 3;
  // pipeline_shaders[2].fragment_shader_path = concat(shader_dir_path, "/blur2_fshader.glsl");
  // pipeline_shaders[2].num_vars = 2;
  // pipeline_shaders[3].fragment_shader_path = concat(shader_dir_path, "/blur3_fshader.glsl");
  // pipeline_shaders[3].num_vars = 2;
  // pipeline_shaders[4].fragment_shader_path = concat(shader_dir_path, "/sobel_fshader.glsl");
  // pipeline_shaders[4].num_vars = 0;
  // pipeline_shaders[5].fragment_shader_path = concat(shader_dir_path, "/vreduce_fshader.glsl");
  // pipeline_shaders[5].num_vars = 2;
  // pipeline_shaders[6].fragment_shader_path = concat(shader_dir_path, "/blur2_fshader.glsl");
  // pipeline_shaders[6].num_vars = 2;
  // pipeline_shaders[7].fragment_shader_path = concat(shader_dir_path, "/blur3_fshader.glsl");
  // pipeline_shaders[7].num_vars = 2;
  // pipeline_shaders[8].fragment_shader_path = concat(shader_dir_path, "/texture_renderer.glsl");

#ifndef LANE_TRACKER_H_
#define LANE_TRACKER_H_

#include "interface/mmal/mmal.h"
#include "interface/mmal/mmal_buffer.h"

void detect_lanes_from_buffer(int download, char *mem_ptr, int bottom_y_boundry, int top_y_boundry, float angle, int show, int stage_to_show);
void init_lane_tracker(const char* shader_dir_path);
void load_egl_image_from_buffer(MMAL_BUFFER_HEADER_T *buf);

#endif
