// Henry Bergin 2019

#ifndef OGL_IMAGE_PROC_PIPELINE_H_
#define OGL_IMAGE_PROC_PIPELINE_H_

#define MAX_IMAGE_STAGES 10
#define PIPELINE_COMPLETED 1
#define PIPELINE_PROCESSING 0

#include "ogl_mngr.h"
// function pointer to the callback for setting shader variables before rendering starts
// You can repeat the stage by seeing what the current stage is and flagging it to repeat
typedef void (*draw_callback_t)(OGL_PROGRAM_CONTEXT_T *program_ctx, int current_render_stage);

typedef struct IMAGE_PIPELINE_SHADER_T
{
  char *fragment_shader_path;
  char **vars;
  int num_vars;
} IMAGE_PIPELINE_SHADER_T;

void init_image_processing_pipeline(char *vertex_shader_path, IMAGE_PIPELINE_SHADER_T *pipeline_shaders, int num_stages);
void register_draw_callback(draw_callback_t dc);
void reset_pipeline();
void set_repeat_stage();
int process_pipeline();
void load_image_to_first_stage(char *image_path);
// You can set it to preserve the output texture unit at any point
void set_preserve_last_stage_output();
void clear_preserve_last_stage_output();
// Which texture unit was outputted from the last stage
// when preserving output.
// You would use this in the draw callback to tell the shader
// where the image input is so it can bounce back and forth
// between the other two and accumulate.
int get_last_output_stage_unit();

#endif
