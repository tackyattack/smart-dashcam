/*
Created by: Henry Bergin 2019
General-Purpose computing on GPU (GPGPU) using OpenGL|ES
*/

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>
#include <unistd.h>

#include "bcm_host.h"

#include "GLES2/gl2.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"

#include "interface/mmal/mmal.h"
#include "interface/mmal/mmal_logging.h"
#include "interface/mmal/mmal_buffer.h"
#include "interface/mmal/util/mmal_util.h"
#include "interface/mmal/util/mmal_util_params.h"
#include "interface/mmal/util/mmal_default_components.h"
#include "interface/mmal/util/mmal_connection.h"

#include "lane_tracker.h"
#include "egl_interface.h"
#include "ogl_mngr.h"
#include "ogl_utils.h"
#include "program_utils.h"
#include "image_loader.h"
#include "ogl_image_proc_pipeline.h"

#define NUM_SHADERS 9

static GLuint data_fbo = 0;

// this will be called right before drawing so you can set variables
// or tell it to repeat this stage
void lane_draw_callback(OGL_PROGRAM_CONTEXT_T *program_ctx, int current_render_stage)
{

  if(current_render_stage == 0)
  {
    glUniform1i(get_program_var(program_ctx, "input_texture")->location, 3);
  }


  if(current_render_stage == 1)
  {
    change_render_window(-1.0, 500.0/1024.0*2 - 1.0, 1.0, -1.0);
  }
  if(current_render_stage == 5)
  {
    change_render_window(-1.0, 25.0/1024.0*2 - 1.0, 1.0, -1.0);
  }

  if(current_render_stage == 2 || current_render_stage == 3)
  {
    glUniform1f(get_program_var(program_ctx, "top_right_y")->location, 500.0);
  }
  if(current_render_stage == 6 || current_render_stage == 7)
  {
    glUniform1f(get_program_var(program_ctx, "top_right_y")->location, 25.0);
  }

  static GLuint toggle = 0;
  if(current_render_stage == (NUM_SHADERS-1))
  {
    glUniform1i(get_program_var(program_ctx, "fps_state")->location, toggle);
    toggle = !toggle;
  }

  if(current_render_stage == (NUM_SHADERS-2))
  {
    data_fbo = get_last_offscreen_fbo();
  }

  if(current_render_stage == (NUM_SHADERS-1))
  {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }
  if(current_render_stage == 1)
  {
    //glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }


}

char* concat(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1) + strlen(s2) + 1); // +1 for the null-terminator
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

static EGL_OBJECT_T egl_device;
static int has_init = 0;

void init_lane_tracker(const char* shader_dir_path)
{
  egl_device.screen_width = 1920;
  egl_device.screen_height = 1080;
  bcm_host_init();
  init_ogl(&egl_device);

  IMAGE_PIPELINE_SHADER_T pipeline_shaders[NUM_SHADERS];
  pipeline_shaders[0].fragment_shader_path = concat(shader_dir_path, "/camera_fshader.glsl");
  pipeline_shaders[0].num_vars = 0;
  pipeline_shaders[1].fragment_shader_path = concat(shader_dir_path, "/birds_eye_fshader.glsl");
  pipeline_shaders[1].num_vars = 0;
  pipeline_shaders[2].fragment_shader_path = concat(shader_dir_path, "/blur2_fshader.glsl");
  pipeline_shaders[2].num_vars = 1;
  pipeline_shaders[3].fragment_shader_path = concat(shader_dir_path, "/blur3_fshader.glsl");
  pipeline_shaders[3].num_vars = 1;
  pipeline_shaders[4].fragment_shader_path = concat(shader_dir_path, "/sobel_fshader.glsl");
  pipeline_shaders[4].num_vars = 0;
  pipeline_shaders[5].fragment_shader_path = concat(shader_dir_path, "/vreduce_fshader.glsl");
  pipeline_shaders[5].num_vars = 0;
  pipeline_shaders[6].fragment_shader_path = concat(shader_dir_path, "/blur2_fshader.glsl");
  pipeline_shaders[6].num_vars = 1;
  pipeline_shaders[7].fragment_shader_path = concat(shader_dir_path, "/blur3_fshader.glsl");
  pipeline_shaders[7].num_vars = 1;
  pipeline_shaders[8].fragment_shader_path = concat(shader_dir_path, "/texture_renderer.glsl");
  pipeline_shaders[8].num_vars = 1;

  char *texture_renderer_vars[] = {"fps_state"};
  pipeline_shaders[8].vars = texture_renderer_vars;
  char *blur_vars[] = {"top_right_y"};
  pipeline_shaders[2].vars = blur_vars;
  pipeline_shaders[3].vars = blur_vars;
  pipeline_shaders[6].vars = blur_vars;
  pipeline_shaders[7].vars = blur_vars;

  char* vshader_path = concat(shader_dir_path, "/flat_vshader.glsl");
  init_image_processing_pipeline(vshader_path, pipeline_shaders, NUM_SHADERS);
  register_draw_callback(lane_draw_callback);
  printf("pipeline created\n");
  for(int i = 0; i < NUM_SHADERS; i++) free(pipeline_shaders[i].fragment_shader_path);
  free(vshader_path);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport ( 0, 0, 1024, 1024);
  has_init = 1;
}

void load_egl_image_from_buffer(MMAL_BUFFER_HEADER_T *buf)
{
  if(!has_init)
  {
    printf("Error: init has not been called first\n");
    return;
  }
  EGLBoolean result = eglMakeCurrent(egl_device.display, egl_device.surface, egl_device.surface, egl_device.context);
  assert(EGL_FALSE != result);
  check();
  reset_pipeline();
  load_mmal_buffer_to_first_stage(buf, egl_device);
  eglMakeCurrent(NULL, NULL, NULL, NULL);
}

void detect_lanes_from_buffer(int download, char *mem_ptr, int show)
{

  if(!has_init)
  {
    printf("Error: init has not been called first\n");
    return;
  }

  EGLBoolean result = eglMakeCurrent(egl_device.display, egl_device.surface, egl_device.surface, egl_device.context);
  assert(EGL_FALSE != result);
  check();


  reset_pipeline();
  while(process_pipeline() != PIPELINE_COMPLETED)
  {

  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  if(show)
  {
    eglSwapBuffers(egl_device.display, egl_device.surface);
  }

  if(download)
  {
    download_fbo(data_fbo,0,10,1024,1,mem_ptr);
  }

  eglMakeCurrent(NULL, NULL, NULL, NULL);
}
