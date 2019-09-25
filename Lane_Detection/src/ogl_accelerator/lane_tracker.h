// Henry Bergin 2019

#ifndef LANE_TRACKER_H_
#define LANE_TRACKER_H_

#include "interface/mmal/mmal.h"
#include "interface/mmal/mmal_buffer.h"

void detect_lanes_from_buffer(int download, char *mem_ptr, int show);
void init_lane_tracker(const char* shader_dir_path);
void load_egl_image_from_buffer(MMAL_BUFFER_HEADER_T *buf);

#endif
