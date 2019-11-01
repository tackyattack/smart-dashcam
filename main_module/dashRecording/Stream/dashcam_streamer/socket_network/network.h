// Henry Bergin 2019

#ifndef NETWORK_H_
#define NETWORK_H_
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "bcm_host.h"

#define PACKET_SIZE 100

#define NETWORK_INIT_ERROR 0
#define NETWORK_INIT_OK 1


typedef void (*server_disconnected_callback_t)();
typedef void (*client_disconnected_callback_t)();

void network_server_init(int port, client_disconnected_callback_t client_disconnected_callback);
uint8_t network_client_init(char *ip, int port);
void network_server_connect();
uint32_t network_server_write(uint8_t *buf, uint32_t size);
uint32_t network_client_recv(uint8_t *buf, uint32_t sz);
void terminate_client();
void terminate_server();

#endif
