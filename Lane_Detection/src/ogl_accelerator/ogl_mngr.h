// Henry Bergin 2019

#ifndef OGL_MNGR_H_
#define OGL_MNGR_H_
#include "egl_interface.h"
#include "GLES2/gl2.h"

#define OGL_ATTRIBUTE_TYPE
#define OGL_UNIFORM_TYPE
#define OGL_VARYING_TYPE

typedef struct
{
  char *name;
  GLuint location;
  char var_type;
} OGL_SHADER_VAR_T;

typedef struct
{
  GLuint vshader; // vertex shader
  GLuint fshader; // fragment shader
  GLuint program;
  OGL_SHADER_VAR_T *vars;
  int num_vars;
  GLuint vbuffer;
} OGL_PROGRAM_CONTEXT_T;

void init_ogl(EGL_OBJ_T *egl_obj);

#endif
