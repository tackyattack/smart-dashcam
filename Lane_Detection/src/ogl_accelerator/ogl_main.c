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

// Flat quad to render to
static const GLfloat vertex_data[] = {
      -1.0,-1.0,1.0,1.0,
      1.0,-1.0,1.0,1.0,
      1.0,1.0,1.0,1.0,
      -1.0,1.0,1.0,1.0
 };

 static void draw_program(OGL_PROGRAM_CONTEXT_T *program_ctx, GLuint fbo, GLuint tex, GLuint vbuffer)
 {
   // render to offscreen fbo
   glBindFramebuffer(GL_FRAMEBUFFER, fbo);
   glBindTexture(GL_TEXTURE_2D, tex);


   glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
   check();
   glBindBuffer(GL_ARRAY_BUFFER, vbuffer);

   // installs the program (done after it has been linked)
   glUseProgram ( program_ctx->program );
   check();

   // NOTE: it's really important that you put this AFTER glUseProgram

   //glUniform1i(glGetUniformLocation(program_ctx, "tex")->location, 1); // tell shader that we're putting it in texture 1
   glUniform2f(get_program_var(program_ctx, "scale")->location, 0.003, 0.003);
   glUniform2f(get_program_var(program_ctx, "centre")->location, 1920/2, 1080/2);

   // render the primitive from array data (triangle fan: first vertex is a hub, others fan around it)
   // 0 -> starting index
   // 4 -> number of indices to render
   glDrawArrays ( GL_TRIANGLE_FAN, 0, 4 );
   check();

   // execute and BLOCK until done (in the future you'll want to check status instead so it doesn't block)
   glFlush();
   glFinish();
   check();
 }

int main ()
{
  EGL_OBJECT_T egl_device;
  egl_device.screen_width = 1920;
  egl_device.screen_height = 1080;
  bcm_host_init();
  init_ogl(&egl_device);

  const GLchar *vshader = ogl_load_shader("shaders/flat_vshader.glsl");
  const GLchar *fshader = ogl_load_shader("shaders/madelbrot_fshader.glsl");

  OGL_PROGRAM_CONTEXT_T programA;
  OGL_SHADER_VAR_T programA_vars[3];
  construct_shader_var(&programA_vars[0], "vertex", OGL_ATTRIBUTE_TYPE);
  construct_shader_var(&programA_vars[1], "scale", OGL_UNIFORM_TYPE);
  construct_shader_var(&programA_vars[2], "centre", OGL_UNIFORM_TYPE);
  programA.vars = programA_vars;
  programA.num_vars = 3;
  create_program_context(&programA, &vshader, &fshader, 0);

  GLuint vbuffer;
  // Specify the clear color for the buffers
  // Create one buffer for vertex data
  glClearColor ( 0.0, 1.0, 1.0, 1.0 );
  glGenBuffers(1, &vbuffer);
  check();

  GLuint input_tex, input_fbo, output_tex, output_fbo;
  create_fbo_tex_pair(&input_tex, &input_fbo, GL_TEXTURE0, 1920, 1080);
  create_fbo_tex_pair(&output_tex, &output_fbo, GL_TEXTURE1, 1920, 1080);


  // Upload vertex data to a buffer
  glBindBuffer(GL_ARRAY_BUFFER, vbuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);
  glVertexAttribPointer(get_program_var(&programA, "vertex")->location, 4, GL_FLOAT, 0, 16, 0);
  glEnableVertexAttribArray(get_program_var(&programA, "vertex")->location);
  check();

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  // Prepare viewport
  glViewport ( 0, 0, 1920, 1080);

  start_profiler_timer();
  draw_program(&programA, output_tex, output_fbo, vbuffer);
  long run_time = stop_profiler_timer();
  printf("time ms: %ld\r\n", run_time);

  tex_to_fb(output_tex, 0, vbuffer);

  glFlush(); // these might not be necessary since we're just changing context?
  glFinish();
  // swap the buffer to the display
  eglSwapBuffers(egl_device.display, egl_device.surface);


  int terminate = 0;
  while(!terminate)
  {


  }

   return 0;
}
