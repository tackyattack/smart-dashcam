// Henry Bergin 2019

#ifndef EGL_INTERFACE_H_
#define EGL_INTERFACE_H_
#include "EGL/egl.h"

typedef struct EGL_OBJECT_T
{
   EGLDisplay display;
   EGLSurface surface;
   EGLContext context;
   uint32_t screen_width;
   uint32_t screen_height;
   uint32_t fbo_width;
   uint32_t fbo_height;

} EGL_OBJECT_T;

void egl_interface_create_display(EGL_OBJECT_T *egl_obj);

#endif
