// Henry Bergin 2019

#ifndef EGL_INTERFACE_H_
#define EGL_INTERFACE_H_
#include "EGL/egl.h"

typedef struct
{
   EGLDisplay display;
   EGLSurface surface;
   EGLContext context;

} EGL_OBJECT_T;

void egl_interface_create_display(EGL_OBJECT_T *egl_obj);

#endif
