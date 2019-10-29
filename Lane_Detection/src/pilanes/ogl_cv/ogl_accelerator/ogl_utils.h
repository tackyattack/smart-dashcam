// Henry Bergin 2019

#ifndef OGL_UTILS_H_
#define OGL_UTILS_H_

#include <assert.h>
#include "GLES2/gl2.h"

#define check() assert(glGetError() == 0)

void showlog(GLint shader);
void showprogramlog(GLint shader);

#endif
