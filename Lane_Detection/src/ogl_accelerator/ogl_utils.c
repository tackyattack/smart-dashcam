// Henry Bergin 2019
#include <stdio.h>
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
