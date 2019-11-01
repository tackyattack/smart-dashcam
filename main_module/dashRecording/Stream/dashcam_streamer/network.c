// Henry Bergin 2019

#include "network.h"
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "queue.h"

/* includes needed for dbus client */
#include <dashcam_dbus/pub_tcp_dbus_clnt.h>
#include <dashcam_dbus/pub_dbus.h>


pthread_mutex_t server_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t server_write_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t client_read_mutex = PTHREAD_MUTEX_INITIALIZER;

//static server_disconnected_callback_t server_disconnected_callback_p = NULL;
static client_disconnected_callback_t client_disconnected_callback_p = NULL;

// int server_socket;
// int server_fd;
// int server_addrlen;
// int server_port;
// struct sockaddr_in server_address;
// uint8_t server_buf[PACKET_SIZE];
// int send_code;
// int client_socket;
int server_running = 0;
int client_running = 0;

struct Queue *server_recv_command_queue;

void terminate_server()
{
  pthread_mutex_lock(&server_mutex);
  server_running = 0;
  pthread_mutex_unlock(&server_mutex);
}

void terminate_client()
{
  pthread_mutex_lock(&client_mutex);
  client_running = 0;
  pthread_mutex_unlock(&client_mutex);
}

sem_t viewer_command;
uint8_t start_command = 0;
uint8_t video_packet_command = 0;
char viewer_uuid[100] = {0};
void tcp_rx_data_callback_server(const char* tcp_clnt_uuid, const char* data, unsigned int data_sz)
{
  printf("recv: %c\n", data[0]);
  pthread_mutex_lock(&server_mutex);

  // if(strstr(tcp_clnt_uuid, viewer_uuid) != NULL)
  // {
  //   if(data[0] == 'v') video_packet_command = 1;
  //   if(data[0] == 's') start_command = 1;
  // }

  if(data[0] == 'v') video_packet_command = 1;
  if(data[0] == 's')
  {
    strcpy(viewer_uuid, tcp_clnt_uuid);
    start_command = 1;
  }
  if (video_packet_command || start_command) sem_post(&viewer_command);
  pthread_mutex_unlock(&server_mutex);
}

dbus_clnt_id server_id;
void network_server_init(int port, client_disconnected_callback_t client_disconnected_callback)
{
  client_disconnected_callback_p = client_disconnected_callback;

  int val;
  const char* server_version;

  sem_init(&viewer_command, 0, 1);

  server_id = tcp_dbus_client_create();
  do
  {
    val = tcp_dbus_client_init(server_id, &server_version);
    if (EXIT_FAILURE == val)
    {
      printf("Failed to initialize client!\nExiting.....\n");
      exit(EXIT_FAILURE);
    }
    else if (val == DBUS_SRV_NOT_AVAILABLE)
    {
      printf("WARNING: DBUS server is unavailable. Trying again in 2 seconds...\n");
      sleep(2);
    }
  }while(EXIT_SUCCESS != val);

  if (EXIT_FAILURE == tcp_dbus_client_Subscribe2Recv(server_id, DBUS_TCP_RECV_SIGNAL, &tcp_rx_data_callback_server))
  {
    printf("Failed to subscribe to DBUS_TCP_RECV_SIGNAL signal!\nExiting.....\n");
    exit(EXIT_FAILURE);
  }

  server_running = 1;
}

uint32_t network_server_write(uint8_t *buf, uint32_t size)
{
  uint8_t data_sent = 0;
  char *send_packet = malloc(size + 1);
  send_packet[0] = 'v';
  memcpy(send_packet+1, buf, size);
  while(!data_sent)
  {
    sem_wait(&viewer_command);
    pthread_mutex_lock(&server_mutex);
    if(start_command)
    {
      // flush stream buffer so PPS/SPS can be inserted
      client_disconnected_callback_p();
      start_command = 0;
      data_sent = 1;
    }
    else if(video_packet_command)
    {
      tcp_dbus_send_msg(server_id, NULL, send_packet, size+1);
      video_packet_command = 0;
      data_sent = 1;
    }
    pthread_mutex_unlock(&server_mutex);
  }

  free(send_packet);
  return 1;
}



sem_t new_frame;
dbus_clnt_id client_id;

char *client_recv_packet = NULL;
uint16_t client_recv_packet_sz = 0;
void tcp_rx_data_callback_client(const char* tcp_clnt_uuid, const char* data, unsigned int data_sz)
{
  printf("recv: %d  %c\n", data_sz, data[0]);
  pthread_mutex_lock(&client_mutex);
//  if(strstr(tcp_clnt_uuid, viewer_uuid) != NULL)
//  {
    if(data[0] == 'v')
    {
      if(client_recv_packet != NULL) free(client_recv_packet);
      client_recv_packet_sz = data_sz - 1;
      client_recv_packet = malloc(client_recv_packet_sz);
      memcpy(client_recv_packet, &data[1], client_recv_packet_sz);
      sem_post(&new_frame);
      printf("posted\n");
    }
  //}
  pthread_mutex_unlock(&client_mutex);
}


uint8_t network_client_init(char *ip, int port)
{
  int val;
  const char* server_version;
  sem_init(&new_frame, 0, 1);

  strcpy(viewer_uuid, ip);

  client_id = tcp_dbus_client_create();
  do
  {
    val = tcp_dbus_client_init(client_id, &server_version);
    if (EXIT_FAILURE == val)
    {
      printf("Failed to initialize client!\nExiting.....\n");
      exit(EXIT_FAILURE);
    }
    else if (val == DBUS_SRV_NOT_AVAILABLE)
    {
      printf("WARNING: DBUS server is unavailable. Trying again in 2 seconds...\n");
      sleep(2);
    }
  }while(EXIT_SUCCESS != val);

  if (EXIT_FAILURE == tcp_dbus_client_Subscribe2Recv(client_id, DBUS_TCP_RECV_SIGNAL, &tcp_rx_data_callback_client))
  {
    printf("Failed to subscribe to DBUS_TCP_RECV_SIGNAL signal!\nExiting.....\n");
    exit(EXIT_FAILURE);
  }
  char start_msg[1];
  start_msg[0] = 's';
  tcp_dbus_send_msg(client_id, NULL, start_msg, 1);

  return NETWORK_INIT_OK;
}


uint32_t network_client_recv(uint8_t *buf, uint32_t sz)
{
  printf("enter client recv\n");
  char frame_request_msg[1];
  frame_request_msg[0] = 'v';
  tcp_dbus_send_msg(client_id, NULL, frame_request_msg, 1);
  printf("waiting for more\n");
  sem_wait(&new_frame);
  //if(client_recv_packet_sz < 1) return -1;
  pthread_mutex_lock(&client_mutex);
  memcpy(buf, client_recv_packet, client_recv_packet_sz);
  printf(" putting %d\n", client_recv_packet_sz);
  pthread_mutex_unlock(&client_mutex);
  return client_recv_packet_sz;
}
