// Henry Bergin 2019
#include <stdio.h>
#include <stdlib.h>
#include "ogl_utils.h"

void showlog(GLint shader)
{
   // Prints the compile log for a shader
   char log[1024];
   glGetShaderInfoLog(shader, sizeof log, NULL, log);
   printf("%d:shader:\n%s\n", shader, log);
}

void showprogramlog(GLint shader)
{
   // Prints the information log for a program object
   char log[1024];
   glGetProgramInfoLog(shader, sizeof log, NULL, log);
   printf("%d:program:\n%s\n", shader, log);
}

char *create_tex_from_image_RGB(char *path, int width, int height)
{
  int tex_size_bytes = width*height*3;
  char *tex_ptr = malloc(tex_size_bytes);
  FILE *tex_file = fopen(path, "rb");
  if (tex_file && tex_ptr)
  {
     int bytes_read=fread(tex_ptr, 1, tex_size_bytes, tex_file);
     assert(bytes_read == tex_size_bytes);  // some problem with file?
     fclose(tex_file);
  }

  return tex_ptr;
}
