/*
Created by: Henry Bergin 2019
*/

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>

#include "ogl_image_proc_pipeline.h"
#include "bcm_host.h"

#include "GLES2/gl2.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"

#include "egl_interface.h"
#include "ogl_mngr.h"
#include "ogl_utils.h"
#include "program_utils.h"
#include "image_loader.h"

typedef struct IMAGE_PIPELINE_STAGE_T
{
  OGL_SHADER_VAR_T vars[2];
  OGL_PROGRAM_CONTEXT_T program;
} IMAGE_PIPELINE_STAGE_T;

static IMAGE_PIPELINE_STAGE_T image_stages[MAX_IMAGE_STAGES];
// texture / fbo pairs for ping pong
static GLuint texture1, fbo1, texture2, fbo2;
static int current_input_buffer = 0; // can be 0 or 1
static int current_stage = 0;
static int completed_stage = 0;
static GLuint completed_fbo = 0;
static int number_of_stages = 0;

static GLuint current_output_tex, current_output_fbo, current_input_tex_unit;

// Flat quad to render to
static const GLfloat vertex_data[] = {
      -1.0,-1.0,1.0,1.0,
      1.0,-1.0,1.0,1.0,
      1.0,1.0,1.0,1.0,
      -1.0,1.0,1.0,1.0
 };

GLuint vbuffer;
void init_image_processing_pipeline(char *vertex_shader_path, char **fragment_shader_paths, int num_stages)
{
   assert(num_stages <= MAX_IMAGE_STAGES);
   number_of_stages = num_stages;
   int verbose = 1;
   // Create one buffer for vertex data
   glGenBuffers(1, &vbuffer);
   glBindBuffer(GL_ARRAY_BUFFER, vbuffer);
   glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);
   check();

   for(int i = 0; i < num_stages; i++)
   {
     const GLchar *vshader = ogl_load_shader(vertex_shader_path);
     const GLchar *fshader = ogl_load_shader(fragment_shader_paths[i]);

     construct_shader_var(&image_stages[i].vars[0], "vertex", OGL_ATTRIBUTE_TYPE);
     construct_shader_var(&image_stages[i].vars[1], "input_texture", OGL_UNIFORM_TYPE);
     check();
     image_stages[i].program.vars = image_stages[i].vars;
     image_stages[i].program.num_vars = 2;
     create_program_context(&image_stages[i].program, &vshader, &fshader, verbose);
     check();

     // Upload vertex data to a buffer
     glVertexAttribPointer(get_program_var(&image_stages[i].program, "vertex")->location, 4, GL_FLOAT, 0, 16, 0);
     glEnableVertexAttribArray(get_program_var(&image_stages[i].program, "vertex")->location);
     check();

     glClearColor ( 0.0, 1.0, 1.0, 1.0 );

   }

   create_fbo_tex_pair(&texture1, &fbo1, GL_TEXTURE0, 1920, 1280);
   create_fbo_tex_pair(&texture2, &fbo2, GL_TEXTURE1, 1920, 1080);
}

 static void draw_stage(OGL_PROGRAM_CONTEXT_T *program_ctx, GLuint out_tex, GLuint out_fbo, GLuint input_tex_unit, GLuint vbuffer)
 {
   printf("drawing: output texture: %d   output_fbo: %d   input_tex_unit:%d\n", out_tex,out_fbo,input_tex_unit);
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
   glUniform1i(get_program_var(program_ctx, "input_texture")->location, input_tex_unit);

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

void reset_pipeline()
{
  current_stage = 0;
}

int process_pipeline()
{
  printf("stage:%d\n", current_stage);
  if(current_stage != 0)
  {
    // GLenum fbo_status = glCheckFramebufferStatus(completed_fbo);
    // if (fbo_status != GL_FRAMEBUFFER_COMPLETE)
    // {
    //   return PIPELINE_PROCESSING;
    // }
    if (current_stage >= number_of_stages)
    {
      return PIPELINE_COMPLETED;
    }
  }

  GLuint output_tex, output_fbo, input_tex_unit;
  if(current_input_buffer == 0)
  {
    output_tex = texture2;
    output_fbo = fbo2;
    input_tex_unit = 0;
  }
  else
  {
    output_tex = texture1;
    output_fbo = fbo1;
    input_tex_unit = 1;
  }
  draw_stage(&image_stages[current_stage].program, output_tex, output_fbo, input_tex_unit, vbuffer);
  completed_stage = current_stage;
  completed_fbo = output_fbo;
  current_output_tex = output_tex;
  current_output_fbo = output_fbo;
  current_input_tex_unit = input_tex_unit;

  current_stage = (current_stage + 1);
  current_input_buffer = (current_input_buffer + 1)%2;
  return PIPELINE_PROCESSING;
}

void load_image_to_first_stage(char *image_path)
{
  reset_pipeline(); // reset stage back to the beginning
  unsigned char *img = loadBMP(image_path);
  glActiveTexture(GL_TEXTURE0+current_input_buffer); // use texture unit x to store it
  glBindTexture(GL_TEXTURE_2D, current_input_buffer);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,1920,1280,0,GL_RGB,GL_UNSIGNED_BYTE,img);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  check();
}

void render_final_stage_to_default_fbo()
{
  // draw to default framebuffer (for screen)
  draw_stage(&image_stages[number_of_stages-1].program, current_output_fbo, 0, current_input_tex_unit, vbuffer);
}
