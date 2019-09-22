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


//
// #include "interface/vcos/vcos.h"
// #include "interface/mmal/mmal.h"
// #include "interface/mmal/mmal_logging.h"
// #include "interface/mmal/mmal_buffer.h"
// #include "interface/mmal/util/mmal_util.h"
// #include "interface/mmal/util/mmal_util_params.h"
// #include "interface/mmal/util/mmal_default_components.h"
// #include "interface/mmal/util/mmal_connection.h"
//
// #include "GLES2/gl2.h"
// #include "EGL/egl.h"
// #include "EGL/eglext.h"

#include "interface/vcos/vcos.h"


#include "interface/mmal/mmal.h"
#include "interface/mmal/mmal_logging.h"
#include "interface/mmal/mmal_buffer.h"
#include "interface/mmal/util/mmal_util.h"
#include "interface/mmal/util/mmal_util_params.h"
#include "interface/mmal/util/mmal_default_components.h"
#include "interface/mmal/util/mmal_connection.h"

#include "GLES2/gl2.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"

#include <bcm_host.h>
#include <GLES2/gl2.h>
#include <GLES/gl.h>
#include <GLES/glext.h>
#include "interface/vcos/vcos.h"
#include "EGL/eglext_brcm.h"


#include "egl_interface.h"
#include "ogl_mngr.h"
#include "ogl_utils.h"
#include "program_utils.h"
#include "image_loader.h"

//https://stackoverflow.com/questions/142789/what-is-a-callback-in-c-and-how-are-they-implemented
static draw_callback_t draw_callback_p = NULL;

unsigned char *img = NULL;

typedef struct IMAGE_PIPELINE_STAGE_T
{
  OGL_SHADER_VAR_T *vars;
  OGL_PROGRAM_CONTEXT_T program;
} IMAGE_PIPELINE_STAGE_T;

static IMAGE_PIPELINE_STAGE_T image_stages[MAX_IMAGE_STAGES];
// texture / fbo pairs for ping pong
static GLuint texture0, fbo0, texture1, fbo1, texture2, fbo2;
static int current_buffer_state = 0; // can be 0, 1, 3
static int last_stage_buffer_state = 0;
static int current_stage = 0;
static int last_stage_ran = 0;
static int number_of_stages = 0;
static int repeat_current_stage = 0;

static GLuint current_output_tex, current_output_fbo, current_input_tex_unit, last_output_tex_unit, last_offscreen_fbo;

//Flat quad to render to
static const GLfloat vertex_default_data[] = {
      -1.0,-1.0,1.0,1.0, // 0
      1.0,-1.0,1.0,1.0,  // 1
      1.0,1.0,1.0,1.0,   // 2
      -1.0,1.0,1.0,1.0   // 3
 };

static GLfloat vertex_data[] = {
  -1.0,-1.0,1.0,1.0, // 0
  1.0,-1.0,1.0,1.0,  // 1
  1.0,1.0,1.0,1.0,   // 2
  -1.0,1.0,1.0,1.0   // 3
 };

 // static GLfloat vertex_data[] = {
 //       -1.0,-1.0,1.0,1.0,
 //       1.0,-1.0,1.0,1.0,
 //       1.0,-0.9,1.0,1.0,
 //       -1.0,-0.9,1.0,1.0
 //  };

void register_draw_callback(draw_callback_t dc)
{
  draw_callback_p = dc;
}

// 1: top left
// 2: bottom right
void change_render_window(float x1, float y1, float x2, float y2)
{
  vertex_data[4*0 + 0] = x1;
  vertex_data[4*0 + 1] = y2;

  vertex_data[4*1 + 0] = x2;
  vertex_data[4*1 + 1] = y2;

  vertex_data[4*2 + 0] = x2;
  vertex_data[4*2 + 1] = y1;

  vertex_data[4*3 + 0] = x1;
  vertex_data[4*3 + 1] = y1;
}

static GLuint cam_ytex;

static GLuint vbuffer;
void init_image_processing_pipeline(char *vertex_shader_path, IMAGE_PIPELINE_SHADER_T *pipeline_shaders, int num_stages)
{
   assert(num_stages <= MAX_IMAGE_STAGES);
   number_of_stages = num_stages;
   int verbose = 1;
   // Create one buffer for vertex data
   glGenBuffers(1, &vbuffer);
   glBindBuffer(GL_ARRAY_BUFFER, vbuffer);
   glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);
   check();

   const GLchar *vshader = ogl_load_shader(vertex_shader_path);
   for(int i = 0; i < num_stages; i++)
   {
     printf("******* %s *******\n", pipeline_shaders[i].fragment_shader_path);
     const GLchar *fshader = ogl_load_shader(pipeline_shaders[i].fragment_shader_path);

     // make space for the internal and external variables
     image_stages[i].vars = malloc(sizeof(OGL_SHADER_VAR_T)*(pipeline_shaders[i].num_vars+2));

     construct_shader_var(&image_stages[i].vars[0], "vertex", OGL_ATTRIBUTE_TYPE);
     construct_shader_var(&image_stages[i].vars[1], "input_texture", OGL_UNIFORM_TYPE);
     for(int var_cnt = 0; var_cnt < pipeline_shaders[i].num_vars; var_cnt++)
     {
       construct_shader_var(&image_stages[i].vars[var_cnt+2], pipeline_shaders[i].vars[var_cnt], OGL_UNIFORM_TYPE);
     }
     check();
     image_stages[i].program.vars = image_stages[i].vars;
     image_stages[i].program.num_vars = 2+pipeline_shaders[i].num_vars;
     create_program_context(&image_stages[i].program, &vshader, &fshader, verbose);
     check();

     // Upload vertex data to a buffer
     glVertexAttribPointer(get_program_var(&image_stages[i].program, "vertex")->location, 4, GL_FLOAT, 0, 16, 0);
     glEnableVertexAttribArray(get_program_var(&image_stages[i].program, "vertex")->location);
     check();

     glClearColor ( 0.0, 1.0, 1.0, 1.0 );

   }

   create_fbo_tex_pair(&texture0, &fbo0, GL_TEXTURE0, 1920, 1080);
   create_fbo_tex_pair(&texture1, &fbo1, GL_TEXTURE1, 1920, 1080);
   create_fbo_tex_pair(&texture2, &fbo2, GL_TEXTURE2, 1920, 1080);

   glActiveTexture(GL_TEXTURE3);
   glGenTextures(1, &cam_ytex);
   glBindTexture(GL_TEXTURE_EXTERNAL_OES, cam_ytex);

}

 static void draw_stage(OGL_PROGRAM_CONTEXT_T *program_ctx, GLuint out_tex, GLuint out_fbo,
                        GLuint input_tex_unit, GLuint output_tex_unit, GLuint vbuffer)
 {
   printf("drawing stage %d: output texture: %d   output_fbo: %d   input_tex_unit:%d   output_tex_unit:%d\n", current_stage, out_tex,out_fbo, input_tex_unit, output_tex_unit);
   // note: you only need to set the active texture unit when making a change to the texture
   //       by binding it
   // render to offscreen fbo
   // Since a texture is linked to this framebuffer, anything written in the shader will write to the texture
   glBindFramebuffer(GL_FRAMEBUFFER, out_fbo);
   last_offscreen_fbo = out_fbo;
   glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertex_data), vertex_data);

   glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT); // Having this in there may signal to GPU that framebuffer doesn't
                                                     // need to go back to CPU
   check();
   //glBindBuffer(GL_ARRAY_BUFFER, vbuffer);

   // installs the program (done after it has been linked)
   glUseProgram ( program_ctx->program );
   check();

   // NOTE: it's really important that you put this AFTER glUseProgram

   // tell shader where the input texture is
   // NOTE: DO NOT USE GL_TEXTURE<i> since that's for setting the active unit
   //       instead use integers 0, 1, 2, ... etc for whichever GL_TEXTURE<i> you used
   glUniform1i(get_program_var(program_ctx, "input_texture")->location, input_tex_unit);
   if(draw_callback_p != NULL)
   {
     draw_callback_p(program_ctx, current_stage);
   }


   // render the primitive from array data (triangle fan: first vertex is a hub, others fan around it)
   // 0 -> starting index
   // 4 -> number of indices to render
   glDrawArrays ( GL_TRIANGLE_FAN, 0, 4 );
   check();


   //glBindTexture(GL_TEXTURE_2D, input_tex_unit+1);

   // execute and BLOCK until done (in the future you'll want to check status instead so it doesn't block)
   //glFlush();
   //glFinish();
   check();
 }

void set_repeat_stage()
{
  repeat_current_stage = 1;
}

static GLuint preserved_output_texture_unit = 0;
static int preserve_last_stage_output = 0;
void set_preserve_last_stage_output()
{
  preserve_last_stage_output = 1;
}
void clear_preserve_last_stage_output()
{
  preserve_last_stage_output = 0;
}
int get_last_output_stage_unit()
{
  return preserved_output_texture_unit;
}
GLuint get_last_offscreen_fbo()
{
  return last_offscreen_fbo;
}

void reset_pipeline()
{
  current_stage = 0;
  current_buffer_state = 0;
  last_output_tex_unit = 0;
  last_stage_ran = -1;
  for(int i = 0; i < 16; i++) vertex_data[i] = vertex_default_data[i];
}

int process_pipeline()
{
  if(current_stage != 0)
  {
    GLenum fbo_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    fbo_status = GL_FRAMEBUFFER_COMPLETE;
    if (fbo_status != GL_FRAMEBUFFER_COMPLETE)
    {
      return PIPELINE_PROCESSING;
    }
    if (current_stage >= number_of_stages)
    {
      return PIPELINE_COMPLETED;
    }
  }

  //printf("     last stage buffer state:%d\n",last_stage_buffer_state);

  //printf("stage:%d\n", current_stage);
  GLuint output_tex, output_fbo, input_tex_unit, output_tex_unit;
  // Just walks up through the buffers then wraps back around

  // if we want to preserve the last stage's output, then jump over that state
  if(preserve_last_stage_output)
  {
    if(current_buffer_state == last_stage_buffer_state)
    {
      current_buffer_state = (current_buffer_state + 1)%3; // hop over to the next state
    }
    preserved_output_texture_unit = (last_stage_buffer_state + 1)%3; // which one we are skipping over is the once
                                                                  // that is being preserved
    //printf("**** preserving texture unit: %d ****\n", preserved_output_texture_unit);
  }
  switch(current_buffer_state)
  {
    case 0:
      output_tex = texture1;
      output_fbo = fbo1;
      //input_tex_unit = 0;
      output_tex_unit = 1;
      break;
    case 1:
      output_tex = texture2;
      output_fbo = fbo2;
      //input_tex_unit = 1;
      output_tex_unit = 2;
      break;
    case 2:
      output_tex = texture0;
      output_fbo = fbo0;
      //input_tex_unit = 2;
      output_tex_unit = 0;
      break;
  }
  input_tex_unit = last_output_tex_unit;
  last_output_tex_unit = output_tex_unit;

  // are we on the last stage? If so, just render to the default FBO so it can go to screen
  // TODO: fix this in the future so you can specify if it should go to screen
  //if(current_stage == (number_of_stages-1)) output_fbo = 0;
  draw_stage(&image_stages[current_stage].program, output_tex, output_fbo, input_tex_unit, output_tex_unit, vbuffer);
  last_stage_ran = current_stage;
  current_output_tex = output_tex;
  current_output_fbo = output_fbo;
  current_input_tex_unit = input_tex_unit;

  if(!repeat_current_stage)
  {
    current_stage = (current_stage + 1); // only go to the next stage if we're not repeating
  }
  else
  {
    repeat_current_stage = 0;
  }
  if(current_stage != last_stage_ran)
  {
    last_stage_buffer_state = current_buffer_state;
  }

  current_buffer_state = (current_buffer_state + 1)%3;


  return PIPELINE_PROCESSING;
}

void load_image_to_first_stage(char *image_path)
{
  assert(current_buffer_state == 0);
  if(img != NULL)
  {
    free(img);
    img = NULL;
  }
  reset_pipeline(); // reset stage back to the beginning
  img = loadBMP(image_path);
  glActiveTexture(GL_TEXTURE0); // use texture unit x to store it
  glBindTexture(GL_TEXTURE_2D, texture0);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,1920,1080,0,GL_RGB,GL_UNSIGNED_BYTE,img);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  check();
}

static EGLImageKHR yimg = EGL_NO_IMAGE_KHR;
void load_mmal_buffer_to_first_stage(MMAL_BUFFER_HEADER_T *buf, EGL_OBJECT_T egl_device)
{
  glActiveTexture(GL_TEXTURE3);
  glBindTexture(GL_TEXTURE_EXTERNAL_OES, cam_ytex);
  check();

  if(yimg != EGL_NO_IMAGE_KHR)
  {
    eglDestroyImageKHR(egl_device.display, yimg);
  }

  yimg = eglCreateImageKHR(egl_device.display,
			EGL_NO_CONTEXT,
			EGL_IMAGE_BRCM_MULTIMEDIA_Y,
			(EGLClientBuffer) buf->data,
			NULL);
		check();
		glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, yimg);
    check();
}
