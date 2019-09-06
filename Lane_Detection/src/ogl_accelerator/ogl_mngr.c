// Henry Bergin 2019
#include "ogl_mngr.h"
#include "ogl_utils.h"
#include <stdio.h>
#include <stdlib.h>


const GLchar *ogl_load_shader(char *path_to_shader)
{
  FILE *shader_file = fopen(path_to_shader, "r");
  if (shader_file == NULL)
  {
    printf("Could not load shader file\r\n");
    return NULL;
  }

  // measure file length
  int file_size_bytes = 0;
  while (fgetc(shader_file) != EOF) file_size_bytes++;
  rewind(shader_file);

  char *shader_content = malloc(file_size_bytes);
  fread(shader_content, 1, file_size_bytes, shader_file);
  fclose(shader_file);

  return shader_content;
}

void init_ogl(EGL_OBJECT_T *egl_obj)
{
  egl_interface_create_display(egl_obj);

  // Set background color and clear buffers
  glClearColor(0.15f, 0.25f, 0.35f, 1.0f);
  glClear( GL_COLOR_BUFFER_BIT );
  check();
}

void create_program_context(OGL_PROGRAM_CONTEXT_T *program_ctx, const GLchar **vshader, const GLchar **fshader, int verbose)
{
  // --------- Create and compile shaders ----------
  program_ctx->vshader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(program_ctx->vshader, 1 /* count*/, vshader, 0 /* length*/);
  glCompileShader(program_ctx->vshader);
  check();
  if (verbose) showlog(program_ctx->vshader);

  program_ctx->fshader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(program_ctx->fshader, 1 /* count*/, fshader, 0 /* length*/);
  glCompileShader(program_ctx->fshader);
  check();
  if (verbose) showlog(program_ctx->fshader);
  // -----------------------------------

  // Create the program
  program_ctx->program = glCreateProgram();
  glAttachShader(program_ctx->program, program_ctx->vshader);
  glAttachShader(program_ctx->program, program_ctx->fshader);
  glLinkProgram(program_ctx->program);
  check();
  if (verbose) showprogramlog(program_ctx->program);

  // load the variable locations into the program context

  for(int i = 0; i < program_ctx->num_vars; i++)
  {
    GLuint location;
    switch (program_ctx->vars[i].var_type)
    {
      case OGL_ATTRIBUTE_TYPE:
        location = glGetAttribLocation(program_ctx->program, program_ctx->vars[i].name);
        break;
      case OGL_UNIFORM_TYPE:
        location = glGetUniformLocation(program_ctx->program, program_ctx->vars[i].name);
        break;
      default:
        printf("Error loading shader variables\r\n");
        break;
    }
    program_ctx->vars[i].location = location;
  }
}

void create_fbo_tex_pair(GLuint *tex, GLuint *fbo, GLuint active_texture_unit, GLuint width, GLuint height)
{
  // Create a texture
  glGenTextures(1, tex);
  check();
  glActiveTexture(active_texture_unit);
  glBindTexture(GL_TEXTURE_2D, *tex);
  check();

  // Setup the 2D texture
  // glTexImage2D(target, level, internal format, width, height, border, format, type, data)
  // target           -> 2D texture
  // level            -> 0 is the base detail level
  // internal format  -> RGB format (check out the luminance for single value!)
  // width/height     -> screen size (this is probably limited by video memory)
  // border           -> always 0
  // format           -> RGB (must match internal format)
  // type             -> data type of texel (GL_UNSIGNED_SHORT_5_6_5 stands for RGB in a 16b value R=5b, G=6b, B=5b)
  // data             -> pointer to image data (null for now)
  //glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,state->screen_width,state->screen_height,0,GL_RGB,GL_UNSIGNED_SHORT_5_6_5,0);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,width,height,0,GL_RGB,GL_UNSIGNED_BYTE,0);
  check();
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  check();

  // Prepare a framebuffer that will be linked to the texture for drawing
  glGenFramebuffers(1,fbo);
  check();
  glBindFramebuffer(GL_FRAMEBUFFER,*fbo);
  check();
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *fbo, 0);
  check();
}

void upload_texture_data(GLuint tex, GLuint width, GLuint height, char *data)
{
  //glActiveTexture(GL_TEXTURE1); // use texture unit x to store it
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,width,height,0,GL_RGB,GL_UNSIGNED_BYTE,data);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  check();
}

void download_fbo(GLuint fb_id, GLuint width, GLuint height, void *mem_ptr)
{
  glBindFramebuffer(GL_FRAMEBUFFER, fb_id);
  check();
  // start at the upper left corner
  glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, mem_ptr);
  check();
}



void construct_shader_var(OGL_SHADER_VAR_T *shader_var, char *name, char var_type)
{
  int name_bytes = strlen(name) + 1;
  shader_var->name = (char *)malloc(name_bytes);
  strcpy(shader_var->name, name);
  shader_var->var_type = var_type;
}

OGL_SHADER_VAR_T *get_program_var(OGL_PROGRAM_CONTEXT_T *program_ctx, char *name)
{
  for(int i = 0; i < program_ctx->num_vars; i++)
  {
    if(strcmp(name, program_ctx->vars[i].name) == 0)
    {
      return &program_ctx->vars[i];
    }
  }
  return NULL;
}
