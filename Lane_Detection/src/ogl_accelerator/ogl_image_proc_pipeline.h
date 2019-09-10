// Henry Bergin 2019

#ifndef OGL_IMAGE_PROC_PIPELINE_H_
#define OGL_IMAGE_PROC_PIPELINE_H_

#define MAX_IMAGE_STAGES 10
#define PIPELINE_COMPLETED 1
#define PIPELINE_PROCESSING 0

void init_image_processing_pipeline(char *vertex_shader_path, char **fragment_shader_paths, int num_stages);
void reset_pipeline();
int process_pipeline();
void load_image_to_first_stage(char *image_path);

#endif
