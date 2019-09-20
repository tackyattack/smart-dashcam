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

#include "bcm_host.h"

#include "GLES2/gl2.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"

#include "egl_interface.h"
#include "ogl_mngr.h"
#include "ogl_utils.h"
#include "program_utils.h"
#include "image_loader.h"
#include "ogl_image_proc_pipeline.h"

#define NUM_SHADERS 10


// this will be called right before drawing so you can set variables
// or tell it to repeat this stage
void draw_callback(OGL_PROGRAM_CONTEXT_T *program_ctx, int current_render_stage)
{
  //if stage == x, set_repeat_stage();
  // glUniform1i(get_program_var(program_ctx, "my_var")->location, my_value);
  // printf("******* stage%d\n", current_render_stage);
  // static int repeat_flag_A = 4;
  // static int repeat_flag_B = 4;
  // if(current_render_stage == 0)
  // {
  //   repeat_flag_A = 4;
  //   repeat_flag_B = 4;
  // }
  // if((current_render_stage == 2) &&(repeat_flag_A))
  // {
  //   set_repeat_stage();
  //   repeat_flag_A--;
  // }
  // if((current_render_stage == 3) &&(repeat_flag_B))
  // {
  //   set_repeat_stage();
  //   repeat_flag_B--;
  // }

  // set_preserve_last_stage_output();
  // static int repeat_flag_A = 2;
  // if(current_render_stage == 0)
  // {
  //   repeat_flag_A = 2;
  // }
  //
  // if((current_render_stage == 2)&&(repeat_flag_A))
  // {
  //   //set_repeat_stage();
  //   printf("   the last stage's output unit is:%d\n", get_last_output_stage_unit());
  //   repeat_flag_A--;
  // }

  if(current_render_stage == 2)
  {
    change_render_window(-1.0, 500.0/1080.0*2 - 1.0, 1.0, -1.0);
  }
  if(current_render_stage == 6)
  {
    change_render_window(-1.0, 25.0/1080.0*2 - 1.0, 1.0, -1.0);
  }

  if(current_render_stage == 2 || current_render_stage == 3)
  {
    glUniform1f(get_program_var(program_ctx, "top_right_y")->location, 500.0);
  }
  if(current_render_stage == 7 || current_render_stage == 8)
  {
    glUniform1f(get_program_var(program_ctx, "top_right_y")->location, 25.0);
  }

  static GLuint toggle = 0;
  if(current_render_stage == (NUM_SHADERS-1))
  {
    glUniform1i(get_program_var(program_ctx, "fps_state")->location, toggle);
    toggle = !toggle;
  }


}

int main ()
{
  //int verbose = 1;
  EGL_OBJECT_T egl_device;
  egl_device.screen_width = 1920;
  egl_device.screen_height = 1080;
  bcm_host_init();
  init_ogl(&egl_device);

  // char*  fragment_shaders[] = {"shaders/grayscale_fshader.glsl",
  //                             "shaders/blur2_fshader.glsl",
  //                             "shaders/blur3_fshader.glsl",
  //                             "shaders/red_to_grayscale_fshader.glsl",
  //                             "shaders/texture_renderer.glsl"};
  IMAGE_PIPELINE_SHADER_T pipeline_shaders[NUM_SHADERS];
  pipeline_shaders[0].fragment_shader_path = "shaders/birds_eye_fshader.glsl";
  pipeline_shaders[0].num_vars = 0;
  pipeline_shaders[1].fragment_shader_path = "shaders/grayscale_fshader.glsl";
  pipeline_shaders[1].num_vars = 0;
  pipeline_shaders[2].fragment_shader_path = "shaders/blur2_fshader.glsl";
  pipeline_shaders[2].num_vars = 1;
  pipeline_shaders[3].fragment_shader_path = "shaders/blur3_fshader.glsl";
  pipeline_shaders[3].num_vars = 1;
  pipeline_shaders[4].fragment_shader_path = "shaders/red_to_grayscale_fshader.glsl";
  pipeline_shaders[4].num_vars = 0;
  pipeline_shaders[5].fragment_shader_path = "shaders/sobel_fshader.glsl";
  pipeline_shaders[5].num_vars = 0;
  pipeline_shaders[6].fragment_shader_path = "shaders/vreduce_fshader.glsl";
  pipeline_shaders[6].num_vars = 0;
  pipeline_shaders[7].fragment_shader_path = "shaders/blur2_fshader.glsl";
  pipeline_shaders[7].num_vars = 1;
  pipeline_shaders[8].fragment_shader_path = "shaders/blur3_fshader.glsl";
  pipeline_shaders[8].num_vars = 1;
  pipeline_shaders[9].fragment_shader_path = "shaders/texture_renderer.glsl";
  pipeline_shaders[9].num_vars = 1;

  char *texture_renderer_vars[] = {"fps_state"};
  pipeline_shaders[9].vars = texture_renderer_vars;
  char *blur_vars[] = {"top_right_y"};
  pipeline_shaders[2].vars = blur_vars;
  pipeline_shaders[3].vars = blur_vars;
  pipeline_shaders[7].vars = blur_vars;
  pipeline_shaders[8].vars = blur_vars;

  init_image_processing_pipeline("shaders/flat_vshader.glsl", pipeline_shaders, NUM_SHADERS);
  register_draw_callback(draw_callback);
  load_image_to_first_stage("sample_images/road2.bmp");
  while(process_pipeline() != PIPELINE_COMPLETED)
  {

  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport ( 0, 0, 1920, 1080);
  eglSwapBuffers(egl_device.display, egl_device.surface);
  // while(1)
  // {
  //
  // }
  while(1)
  {
    reset_pipeline();
    load_image_to_first_stage("sample_images/road2.bmp");
    //glFlush(); // these are purely for getting an accurate time measurement (though forcing a sync could incur time too)
    //glFinish();
    start_profiler_timer();
    while(process_pipeline() != PIPELINE_COMPLETED)
    {

    }
    //glFlush(); // these are purely for getting an accurate time measurement (though forcing a sync could incur time too)
    //glFinish();
    eglSwapBuffers(egl_device.display, egl_device.surface);
    long run_time = stop_profiler_timer();
    printf("time ms: %ld\r\n", run_time);
  }


  while(1)
  {

  }
  return 0;
}
