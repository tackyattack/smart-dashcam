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

// Flat quad to render to
static const GLfloat vertex_data[] = {
      -1.0,-1.0,1.0,1.0,
      1.0,-1.0,1.0,1.0,
      1.0,1.0,1.0,1.0,
      -1.0,1.0,1.0,1.0
 };

 static void draw_program(OGL_PROGRAM_CONTEXT_T *program_ctx, GLuint out_tex, GLuint out_fbo, GLuint input_tex_unit, GLuint vbuffer)
 {
   // note: you only need to set the active texture unit when making a change to the texture
   //       by binding it
   // render to offscreen fbo
   glBindFramebuffer(GL_FRAMEBUFFER, out_fbo);



   glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
   check();
   glBindBuffer(GL_ARRAY_BUFFER, vbuffer);

   // installs the program (done after it has been linked)
   glUseProgram ( program_ctx->program );
   check();

   // NOTE: it's really important that you put this AFTER glUseProgram

   // tell shader where the input texture is
   // NOTE: DO NOT USE GL_TEXTURE<i> since that's for setting the active unit
   //       instead use integers 0, 1, 2, ... etc for whichever GL_TEXTURE<i> you used
   glUniform1i(get_program_var(program_ctx, "tex")->location, input_tex_unit);

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
  int verbose = 1;
  EGL_OBJECT_T egl_device;
  egl_device.screen_width = 1920;
  egl_device.screen_height = 1080;
  bcm_host_init();
  init_ogl(&egl_device);

  const GLchar *vshader = ogl_load_shader("shaders/flat_vshader.glsl");
  const GLchar *fshader = ogl_load_shader("shaders/test_image_shader.glsl");

  OGL_PROGRAM_CONTEXT_T programA;
  OGL_SHADER_VAR_T programA_vars[2];
  construct_shader_var(&programA_vars[0], "vertex", OGL_ATTRIBUTE_TYPE);
  construct_shader_var(&programA_vars[1], "tex", OGL_UNIFORM_TYPE);
  check();
  programA.vars = programA_vars;
  programA.num_vars = 2;
  create_program_context(&programA, &vshader, &fshader, verbose);
  check();


  const GLchar *fshader_renderer = ogl_load_shader("shaders/texture_renderer.glsl");

  OGL_PROGRAM_CONTEXT_T render_program;
  OGL_SHADER_VAR_T render_program_vars[2];
  construct_shader_var(&render_program_vars[0], "vertex", OGL_ATTRIBUTE_TYPE);
  construct_shader_var(&render_program_vars[1], "tex", OGL_UNIFORM_TYPE);
  check();
  render_program.vars = render_program_vars;
  render_program.num_vars = 2;
  create_program_context(&render_program, &vshader, &fshader_renderer, verbose);
  check();

  GLuint vbuffer;
  // Specify the clear color for the buffers
  // Create one buffer for vertex data
  glClearColor ( 0.0, 1.0, 1.0, 1.0 );
  glGenBuffers(1, &vbuffer);
  check();

  GLuint input_tex, input_fbo, output_tex, output_fbo;
  create_fbo_tex_pair(&input_tex, &input_fbo, GL_TEXTURE0, 1920, 1280);
  create_fbo_tex_pair(&output_tex, &output_fbo, GL_TEXTURE1, 1920, 1080);



  unsigned char *img = loadBMP("sample_images/road2.bmp");
  glActiveTexture(GL_TEXTURE0); // use texture unit x to store it
  glBindTexture(GL_TEXTURE_2D, input_tex);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,1920,1280,0,GL_RGB,GL_UNSIGNED_BYTE,img);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  check();



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
  draw_program(&programA, output_tex, output_fbo, 0, vbuffer);
  long run_time = stop_profiler_timer();
  printf("time ms: %ld\r\n", run_time);

  // render to the main framebuffer
  draw_program(&render_program, output_tex, 0, 1, vbuffer);


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
