// Henry Bergin 2019
#include "egl_interface.h"
#include "ogl_utils.h"
#include "EGL/eglext.h"
#include "EGL/egl.h"
#include "bcm_host.h"

void egl_interface_create_display(EGL_OBJECT_T *egl_obj)
{
	int32_t success = 0;
	EGLBoolean result;
  EGLint num_config;

  static EGL_DISPMANX_WINDOW_T nativewindow;

  DISPMANX_ELEMENT_HANDLE_T dispman_element;
  DISPMANX_DISPLAY_HANDLE_T dispman_display;
  DISPMANX_UPDATE_HANDLE_T dispman_update;
  VC_RECT_T dst_rect;
  VC_RECT_T src_rect;

  static const EGLint attribute_list[] =
  {
     EGL_RED_SIZE, 8,
     EGL_GREEN_SIZE, 8,
     EGL_BLUE_SIZE, 8,
     EGL_ALPHA_SIZE, 8,
     EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
     EGL_NONE
  };

  static const EGLint context_attributes[] =
  {
     EGL_CONTEXT_CLIENT_VERSION, 2,
     EGL_NONE
  };
  EGLConfig config;

  // get an EGL display connection
  egl_obj->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  assert(egl_obj->display!=EGL_NO_DISPLAY);
  check();

  // initialize the EGL display connection
  result = eglInitialize(egl_obj->display, NULL, NULL);
  assert(EGL_FALSE != result);
  check();

  // get an appropriate EGL frame buffer configuration
  result = eglChooseConfig(egl_obj->display, attribute_list, &config, 1, &num_config);
  assert(EGL_FALSE != result);
  check();

  // get an appropriate EGL frame buffer configuration
  result = eglBindAPI(EGL_OPENGL_ES_API);
  assert(EGL_FALSE != result);
  check();

  // create an EGL rendering context
  egl_obj->context = eglCreateContext(egl_obj->display, config, EGL_NO_CONTEXT, context_attributes);
  assert(egl_obj->context!=EGL_NO_CONTEXT);
  check();

  // create an EGL window surface
  success = graphics_get_display_size(0 /* LCD */, &egl_obj->screen_width, &egl_obj->screen_height);
  assert( success >= 0 );

  dst_rect.x = 0;
  dst_rect.y = 0;
  dst_rect.width = egl_obj->screen_width;
  dst_rect.height = egl_obj->screen_height;
  src_rect.x = 0;
  src_rect.y = 0;
  src_rect.width = egl_obj->fbo_width << 16;
  src_rect.height = egl_obj->fbo_height << 16;

  dispman_display = vc_dispmanx_display_open( 0 /* LCD */);
  dispman_update = vc_dispmanx_update_start( 0 );

  dispman_element = vc_dispmanx_element_add ( dispman_update, dispman_display,
     0/*layer*/, &dst_rect, 0/*src*/,
     &src_rect, DISPMANX_PROTECTION_NONE, 0 /*alpha*/, 0/*clamp*/, 0/*transform*/);

  nativewindow.element = dispman_element;
  nativewindow.width = egl_obj->fbo_width;
  nativewindow.height = egl_obj->fbo_height;
  vc_dispmanx_update_submit_sync( dispman_update );

  check();

  egl_obj->surface = eglCreateWindowSurface( egl_obj->display, config, &nativewindow, NULL );
  assert(egl_obj->surface != EGL_NO_SURFACE);
  check();

  // connect the context to the surface
  result = eglMakeCurrent(egl_obj->display, egl_obj->surface, egl_obj->surface, egl_obj->context);
  assert(EGL_FALSE != result);
  check();
}
