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
#include <queue.h>
 #include <time.h>

pthread_mutex_t server_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t server_write_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t client_read_mutex = PTHREAD_MUTEX_INITIALIZER;

#define VIDEO_PACKET 'v'
#define PING_REQUEST 'p'
#define PING_RESPONSE 'r'

// this is how long the server will wait until checking if the client is reading it into
// the stream playback buffer. This is needed because otherwise the server has no way
// of knowing if consumption is happening, which would end up placing it all in the
// system TCP buffer (making a large video delay).
#define PING_TIME_S 1.0

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
struct Queue *server_send_queue;
void network_server_init(int port, client_disconnected_callback_t client_disconnected_callback)
{
  server_send_queue = createQueue(PACKET_SIZE*2);
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

int32_t network_server_send_all(uint8_t *buf, uint32_t len)
{
    int32_t total_bytes = 0;
    int32_t bytes = 0;

    while (len > 0)
    {
        bytes = send(server_socket, buf + total_bytes, len, 0);
        if (bytes == -1)
            return -1;
        total_bytes += bytes;
        len -= bytes;
    }
    return total_bytes;
}

uint8_t packet_buf[PACKET_SIZE];
int32_t prepare_buffer(char code, uint8_t *buf, uint32_t size)
{
  char x;
  static uint32_t buf_pos = 0;
  static char set_code = 0;

  if(set_code == 0) set_code = code;
  int32_t valsent = 0;

  // add the ID if we're on the first position
  if(buf_pos == 0)
  {
    packet_buf[buf_pos] = set_code;
    buf_pos++;
  }

  uint32_t amount_to_move_in = (PACKET_SIZE-1) - (buf_pos) + 1; // distance between top and bottom

  // if there's less to move in than we want, just shuffle in what's available
  if(size < amount_to_move_in) amount_to_move_in = size;

  memcpy(&packet_buf[buf_pos], buf, amount_to_move_in);
  buf_pos += amount_to_move_in;

  // check if buffer is all ready to send
  // this happens when the buffer position has moved beyond the buffer space
  // (think of it like mod wrap around)
  if(buf_pos == PACKET_SIZE)
  {
    pthread_mutex_lock(&server_write_mutex);
    valsent = network_server_send_all(packet_buf, PACKET_SIZE);
    pthread_mutex_unlock(&server_write_mutex);

    // ready for the next code
    set_code = 0;
    // wrap back around
    buf_pos = 0;

    // if network didn't send, get ready for reset and return error
    if(valsent == -1)
    {
      return -1;
    }

    // if we just sent a ping request, wait here until you get a reply
    if(set_code == PING_REQUEST)
    {
      //printf("pinging\n");
      read(server_socket, &x, 1);
    }
  }

  // tell how many we actually moved in
  return amount_to_move_in;
}

int32_t record_server_bytes(char code, uint8_t *buf, uint32_t size)
{
  int32_t total_bytes = 0;
  int32_t bytes = 0;

  // move in all the bytes we have
  while (size > 0)
  {
    bytes = prepare_buffer(code, buf+total_bytes, size);
    if(bytes == -1) return -1;
    total_bytes += bytes;
    size -= bytes;
  }

  return total_bytes;
}


uint32_t network_server_write(uint8_t *buf, uint32_t size)
{
  char code = VIDEO_PACKET; // just normal video packet

  static time_t start;
  static int first_time = 1;
  if(first_time)
  {
    start = time(NULL);
    first_time = 0;
  }
  if(time(NULL) - start > PING_TIME_S)
  {
    code = PING_REQUEST;
    start=time(NULL);
  }

  int32_t bytes_written = record_server_bytes(code, buf, size);

  if(bytes_written < 0)
  {
    network_server_connect();
    client_disconnected_callback_p();
    bytes_written = 0;
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

int32_t readall_client(int socket, uint8_t *buffer, uint32_t size)
{
  int bytes_read = 0;
  int result;
  while (bytes_read < size)
  {
    result = read(socket, buffer + bytes_read, size - bytes_read);
    if (result < 1 ) return -1;
    bytes_read += result;
  }
  return bytes_read;
}

uint32_t network_client_recv(uint8_t *buf, uint32_t sz)
{
  int32_t valread = 0;
  uint8_t packet_buf[PACKET_SIZE];
  uint32_t num_packets = sz/PACKET_SIZE;

  if(num_packets < 1)
  {
    printf("error: chunk size must be bigger than at least one packet");
    exit(EXIT_FAILURE);
  }

  for(uint32_t i = 0; i < num_packets; i++)
  {
    pthread_mutex_lock(&client_mutex);
    valread = readall_client(client_socket, packet_buf, PACKET_SIZE);
    pthread_mutex_unlock(&client_mutex);
    if(valread < 1)
    {
      close(client_socket);
      network_client_connect();
      return 0;
    }
    if(packet_buf[0] == PING_REQUEST)
    {
      char x[1] = {PING_RESPONSE};
      write(client_socket, x, 1);
      //printf("ping responding\n");
    }
    memcpy(&buf[i*(PACKET_SIZE-1)], packet_buf+1, PACKET_SIZE-1);
  }

  uint32_t bytes_read = num_packets*(PACKET_SIZE-1);

  return bytes_read;
}
