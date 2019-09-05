// Henry Bergin 2019
#define OGL_ATTRIBUTE_TYPE 1
#define OGL_UNIFORM_TYPE   2
#define OGL_VARYING_TYPE   3

#ifndef OGL_MNGR_H_
#define OGL_MNGR_H_
#include "egl_interface.h"
#include "GLES2/gl2.h"

typedef struct OGL_SHADER_VAR_T
{
  char *name;
  GLuint location;
  char var_type;
} OGL_SHADER_VAR_T;

typedef struct OGL_PROGRAM_CONTEXT_T
{
  GLuint vshader; // vertex shader
  GLuint fshader; // fragment shader
  GLuint program;
  OGL_SHADER_VAR_T *vars;
  int num_vars;
} OGL_PROGRAM_CONTEXT_T;

void init_ogl(EGL_OBJECT_T *egl_obj);
const GLchar *ogl_load_shader(char *path_to_shader);
void create_program_context(OGL_PROGRAM_CONTEXT_T *program_ctx, const GLchar **vshader, const GLchar **fshader, int verbose);
void create_fbo_tex_pair(GLuint *tex, GLuint *fbo, GLuint active_texture_unit, GLuint width, GLuint height);
void construct_shader_var(OGL_SHADER_VAR_T *shader_var,char *name, char var_type);
OGL_SHADER_VAR_T *get_program_var(OGL_PROGRAM_CONTEXT_T *program_ctx, char *name);
void tex_to_fb(GLuint tex_id, GLuint fb_id, GLuint vbuffer);

#endif
