/*
Created by: Henry Bergin 2019
General-Purpose computing on GPU (GPGPU) using OpenGL|ES
*/

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>

#include "bcm_host.h"

#include "GLES2/gl2.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"

typedef struct
{
   uint32_t screen_width;
   uint32_t screen_height;
// OpenGL|ES objects
   EGLDisplay display;
   EGLSurface surface;
   EGLContext context;

   GLuint verbose;
   GLuint vshader;
   GLuint fshader;
   GLuint mshader;
   GLuint program;
   GLuint program2;
   GLuint tex_fb;
   GLuint tex;
   GLuint buf;
// julia attribs
   GLuint unif_color, attr_vertex, unif_scale, unif_offset, unif_tex, unif_centre;
// mandelbrot attribs
   GLuint attr_vertex2, unif_scale2, unif_offset2, unif_centre2;

} OBJ_STATE_T;
static OBJ_STATE_T _state, *state=&_state;

// ----------- Utilities ------------
#define check() assert(glGetError() == 0)

static void showlog(GLint shader)
{
   // Prints the compile log for a shader
   char log[1024];
   glGetShaderInfoLog(shader,sizeof log,NULL,log);
   printf("%d:shader:\n%s\n", shader, log);
}

static void showprogramlog(GLint shader)
{
   // Prints the information log for a program object
   char log[1024];
   glGetProgramInfoLog(shader,sizeof log,NULL,log);
   printf("%d:program:\n%s\n", shader, log);
}
// ----------------------------------


static void init_ogl(OBJ_STATE_T *state)
{
  int32_t success = 0;

  // -------------------- Setup EGL ----------------------
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
  state->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  assert(state->display!=EGL_NO_DISPLAY);
  check();

  // initialize the EGL display connection
  result = eglInitialize(state->display, NULL, NULL);
  assert(EGL_FALSE != result);
  check();

  // get an appropriate EGL frame buffer configuration
  result = eglChooseConfig(state->display, attribute_list, &config, 1, &num_config);
  assert(EGL_FALSE != result);
  check();

  // get an appropriate EGL frame buffer configuration
  result = eglBindAPI(EGL_OPENGL_ES_API);
  assert(EGL_FALSE != result);
  check();

  // create an EGL rendering context
  state->context = eglCreateContext(state->display, config, EGL_NO_CONTEXT, context_attributes);
  assert(state->context!=EGL_NO_CONTEXT);
  check();

  // create an EGL window surface
  success = graphics_get_display_size(0 /* LCD */, &state->screen_width, &state->screen_height);
  assert( success >= 0 );

  dst_rect.x = 0;
  dst_rect.y = 0;
  dst_rect.width = state->screen_width;
  dst_rect.height = state->screen_height;

  src_rect.x = 0;
  src_rect.y = 0;
  src_rect.width = state->screen_width << 16;
  src_rect.height = state->screen_height << 16;

  dispman_display = vc_dispmanx_display_open( 0 /* LCD */);
  dispman_update = vc_dispmanx_update_start( 0 );

  dispman_element = vc_dispmanx_element_add ( dispman_update, dispman_display,
     0/*layer*/, &dst_rect, 0/*src*/,
     &src_rect, DISPMANX_PROTECTION_NONE, 0 /*alpha*/, 0/*clamp*/, 0/*transform*/);

  nativewindow.element = dispman_element;
  nativewindow.width = state->screen_width;
  nativewindow.height = state->screen_height;
  vc_dispmanx_update_submit_sync( dispman_update );

  check();

  state->surface = eglCreateWindowSurface( state->display, config, &nativewindow, NULL );
  assert(state->surface != EGL_NO_SURFACE);
  check();

  // connect the context to the surface
  result = eglMakeCurrent(state->display, state->surface, state->surface, state->context);
  assert(EGL_FALSE != result);
  check();
  // -----------------------------------------------------

  // Set background color and clear buffers
  glClearColor(0.15f, 0.25f, 0.35f, 1.0f);
  glClear( GL_COLOR_BUFFER_BIT );

  check();
}


static void init_shaders(OBJ_STATE_T *state)
{

  // Flat quad
  static const GLfloat vertex_data[] = {
        -1.0,-1.0,1.0,1.0,
        1.0,-1.0,1.0,1.0,
        1.0,1.0,1.0,1.0,
        -1.0,1.0,1.0,1.0
   };

  // vertex shader
  const GLchar *vshader_source =
              "attribute vec4 vertex;"
              "varying vec2 tcoord;"
              "void main(void) {"
              " vec4 pos = vertex;"
              " gl_Position = pos;"
              " tcoord = vertex.xy*0.5+0.5;"
              "}";

  //Mandelbrot fragment shader
  const GLchar *mandelbrot_fshader_source =
  "uniform vec4 color;"
  "uniform vec2 scale;"
  "uniform vec2 centre;"
  "varying vec2 tcoord;"
  "void main(void) {"
  "  float intensity;"
  "  vec4 color2;"
  "  float cr=(gl_FragCoord.x-centre.x)*scale.x;"
  "  float ci=(gl_FragCoord.y-centre.y)*scale.y;"
  "  float ar=cr;"
  "  float ai=ci;"
  "  float tr,ti;"
  "  float col=0.0;"
  "  float p=0.0;"
  "  int i=0;"
  "  for(int i2=1;i2<16;i2++)"
  "  {"
  "    tr=ar*ar-ai*ai+cr;"
  "    ti=2.0*ar*ai+ci;"
  "    p=tr*tr+ti*ti;"
  "    ar=tr;"
  "    ai=ti;"
  "    if (p>16.0)"
  "    {"
  "      i=i2;"
  "      break;"
  "    }"
  "  }"
  "  color2 = vec4(float(i)*0.0625,0,0,1);"
  "  gl_FragColor = color2;"
  "}";

  // --------- Create and compile shaders ----------
  state->vshader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(state->vshader, 1, &vshader_source, 0);
  glCompileShader(state->vshader);
  check();
  if (state->verbose) showlog(state->vshader);

  state->mshader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(state->mshader, 1, &mandelbrot_fshader_source, 0);
  glCompileShader(state->mshader);
  check();
  if (state->verbose) showlog(state->mshader);
  // -----------------------------------


  // Create the program
  state->program2 = glCreateProgram();
  glAttachShader(state->program2, state->vshader);
  glAttachShader(state->program2, state->mshader);
  glLinkProgram(state->program2);
  check();
  if (state->verbose) showprogramlog(state->program2);

  // Get attribute locations
  state->attr_vertex2 = glGetAttribLocation(state->program2, "vertex");
  state->unif_scale2  = glGetUniformLocation(state->program2, "scale");
  state->unif_offset2 = glGetUniformLocation(state->program2, "offset");
  state->unif_centre2 = glGetUniformLocation(state->program2, "centre");
  check();

  // Specify the clear color for the buffers
  // Create one buffer
  glClearColor ( 0.0, 1.0, 1.0, 1.0 );
  glGenBuffers(1, &state->buf);
  check();

  // Create a texture and bind it to a 2D target
  glGenTextures(1, &state->tex);
  check();
  glBindTexture(GL_TEXTURE_2D,state->tex);
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
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,state->screen_width,state->screen_height,0,GL_RGB,GL_UNSIGNED_SHORT_5_6_5,0);
  check();
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  check();

  // Prepare a framebuffer for rendering
  glGenFramebuffers(1,&state->tex_fb);
  check();
  glBindFramebuffer(GL_FRAMEBUFFER,state->tex_fb);
  check();
  glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,state->tex,0);
  check();
  glBindFramebuffer(GL_FRAMEBUFFER,0);
  check();

  // Prepare viewport
  glViewport ( 0, 0, state->screen_width, state->screen_height );
  check();

  // Upload vertex data to a buffer
  glBindBuffer(GL_ARRAY_BUFFER, state->buf);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);
  glVertexAttribPointer(state->attr_vertex2, 4, GL_FLOAT, 0, 16, 0);
  glEnableVertexAttribArray(state->attr_vertex2);
  check();

}


int main ()
{
   int terminate = 0;
   GLfloat cx, cy;
   bcm_host_init();

   // Clear application state
   memset( state, 0, sizeof( *state ) );

   // Start OGLES
   init_ogl(state);
   init_shaders(state);
   cx = state->screen_width/2;
   cy = state->screen_height/2;

   return 0;
}
