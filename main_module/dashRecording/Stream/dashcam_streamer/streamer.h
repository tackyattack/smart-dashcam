// Henry Bergin 2019

#ifndef STREAMER_H_
#define STREAMER_H_
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "bcm_host.h"

void streamer_init(int port, uint32_t buffer_size);
void record_bytes(uint8_t *buf, uint32_t buf_size);
void close_server();


#endif
