// Henry Bergin 2019
#include "ogl_mngr.h"
#include <stdio.h>

// Flat quad to render to
static const GLfloat vertex_data[] = {
      -1.0,-1.0,1.0,1.0,
      1.0,-1.0,1.0,1.0,
      1.0,1.0,1.0,1.0,
      -1.0,1.0,1.0,1.0
 };


GLchar *load_shader(char *path_to_shader)
{
  FILE *shader_file = fopen(path_to_shader, "r");
  if (shader == NULL)
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

void init_ogl(EGL_OBJ_T *egl_obj)
{
  egl_interface_create_display(egl_obj);

  // Set background color and clear buffers
  glClearColor(0.15f, 0.25f, 0.35f, 1.0f);
  glClear( GL_COLOR_BUFFER_BIT );
  check();
}

void create_program_context(OGL_PROGRAM_CONTEXT_T *program_ctx, OGL_SHADER_VAR_T *vars, int num_vars,
                            GLchar *vshader, Glchar *fshader, int verbose)
{
  OGL_PROGRAM_CONTEXT_T program_ctx;
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
  if (verbose) showprogramlog(state->program);

  // load the variable locations into the program context
  program_ctx->vars = (OGL_SHADER_VAR_T *)malloc(num_vars*sizeof(vars[0]));

  for(int i = 0; i < num_vars; i++)
  {
    program_ctx->vars[i].var_type = vars[i].var_type;
    int name_sz = strlen(vars[i].name) + 1;
    program_ctx->vars[i].name = (char *)malloc(name_sz);
    strcpy(program_ctx->vars[i].name, vars[i].name);
    GLuint location;
    switch (vars[i].var_type)
    {
      case OGL_ATTRIBUTE_TYPE:
        location = glGetAttribLocation(program_ctx->program, program_ctx->vars[i].name);
      case OGL_UNIFORM_TYPE:
        location = glGetUniformLocation(program_ctx->program, program_ctx->vars[i].name);
      default:
        printf("Error loading shader variables\r\n");
    }
    program_ctx->vars[i].location = location;
  }
}

void ogl_setup()
{
  // Create a texture for the output to screen
  glGenTextures(1, &state->tex);
  check();
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D,state->tex);
  check();
}
