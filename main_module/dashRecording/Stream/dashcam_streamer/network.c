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

pthread_mutex_t server_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t server_write_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t client_read_mutex = PTHREAD_MUTEX_INITIALIZER;

//static server_disconnected_callback_t server_disconnected_callback_p = NULL;
static client_disconnected_callback_t client_disconnected_callback_p = NULL;

int server_socket;
int server_fd;
int server_addrlen;
int server_port;
struct sockaddr_in server_address;
uint8_t server_buf[PACKET_SIZE];
int send_code;
int client_socket;
int server_running = 0;
int client_running = 0;

char *client_server_ip = NULL;
int client_server_port = 0;


void terminate_server()
{
  pthread_mutex_lock(&server_mutex);
  close(server_socket);
  server_running = 0;
  pthread_mutex_unlock(&server_mutex);
}

void terminate_client()
{
  pthread_mutex_lock(&client_mutex);
  close(client_socket);
  client_running = 0;
  if(client_server_ip != NULL) free(client_server_ip);
  pthread_mutex_unlock(&client_mutex);
}

// void block_until_ready(int fd)
// {
//   poll(server_fd, server_socket, 1000);
// }

void network_server_init(int port, client_disconnected_callback_t client_disconnected_callback)
{
  client_disconnected_callback_p = client_disconnected_callback;
  server_port = port;
  int opt = 1;
  server_addrlen = sizeof(server_address);

  // Creating socket file descriptor
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
  {
      perror("socket failed");
      exit(EXIT_FAILURE);
  }

  // Forcefully attaching socket to the port
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
  {
      perror("setsockopt");
      exit(EXIT_FAILURE);
  }
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons( server_port );

  // Forcefully attaching socket to the port 8080
  if (bind(server_fd, (struct sockaddr *)&server_address,
                               sizeof(server_address))<0)
  {
      perror("bind failed");
      exit(EXIT_FAILURE);
  }
  if (listen(server_fd, 3) < 0)
  {
      perror("listen");
      exit(EXIT_FAILURE);
  }

  server_running = 1;
}

void network_server_connect()
{
  printf("connecting\n");
  int new_server_socket = -1;
  int should_exit = 0;
  fcntl(server_fd, F_SETFL, O_NONBLOCK); // non-blocking
  while(!should_exit && (new_server_socket == -1))
  {
    pthread_mutex_lock(&server_mutex);
    if(!server_running) should_exit = 1;
    new_server_socket = accept(server_fd, (struct sockaddr *)&server_address,(socklen_t*)&server_addrlen);
    pthread_mutex_unlock(&server_mutex);
    sleep(1);
  }
  pthread_mutex_lock(&server_mutex);
  if(server_running)
  {
    server_socket = new_server_socket;
    printf("connected\n");

  }
  pthread_mutex_unlock(&server_mutex);
}


uint32_t network_server_write(uint8_t *buf, uint32_t size)
{
  // wait until client wants a frame
  char x[1] = {0};
  read(server_socket, x, 1);

  pthread_mutex_lock(&server_write_mutex);
  int32_t bytes_written = write(server_socket, buf, size);
  pthread_mutex_unlock(&server_write_mutex);
  if(bytes_written < 0)
  {
    network_server_connect();
    client_disconnected_callback_p();
  }
  return (uint32_t)bytes_written;
}

struct sockaddr_in serv_addr;

uint8_t network_client_connect()
{
  int should_exit = 0;
  if(client_server_ip == NULL) return NETWORK_INIT_ERROR;

  client_socket = 0;

  if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
      printf("\n Socket creation error \n");
  }
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(client_server_port);

  // Convert IPv4 and IPv6 addresses from text to binary form
  if(inet_pton(AF_INET, client_server_ip, &serv_addr.sin_addr)<=0)
  {
      printf("\nInvalid address/ Address not supported \n");
      return NETWORK_INIT_ERROR;
  }

  while(!should_exit)
  {
    pthread_mutex_lock(&client_mutex);
    if(!client_running) should_exit = 1;
    if(connect(client_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == 0) should_exit=1;
    pthread_mutex_unlock(&client_mutex);
    sleep(1);
  }

  return NETWORK_INIT_OK;
}

uint8_t network_client_init(char *ip, int port)
{
  client_server_port = port;
  if(client_server_ip != NULL) free(client_server_ip);
  client_server_ip = malloc((uint16_t)(strlen(ip)+1));
  strcpy(client_server_ip, ip);
  return network_client_connect();
}

uint32_t network_client_recv(uint8_t *buf, uint32_t sz)
{
  // tell server we want a frame
  char x[1] = {'v'};
  write(client_socket, x, 1);

  pthread_mutex_lock(&client_read_mutex);
  int32_t valread = read(client_socket, buf, PACKET_SIZE);
  pthread_mutex_unlock(&client_read_mutex);
  if(valread < 0)
  {
    close(client_socket);
    network_client_connect();
    valread = 0;
  }
  return valread;
}
