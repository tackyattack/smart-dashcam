// Henry Bergin 2019
//./dash_stream.bin 192.168.0.0 8080

// https://kwasi-ich.de/blog/2017/11/26/omx/
// page 87 of spec has H264 details
// You need to grab SPS PPS NAL units to start the decoder


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bcm_host.h"
#include "ilclient.h"

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>

#define NAL_UNIT_TYPE_NON_IDR 1
#define NAL_UNIT_TYPE_IDR 5
#define NAL_UNIT_TYPE_SPS 7
#define NAL_UNIT_TYPE_PPS 8

#define CHUNK_SIZE 100
uint32_t OMX_buffer_handoff_size = 500;

void hex_block_print(uint8_t *buf, int sz)
{
    for (int i = 0; i < sz ;i++)
    {
        printf(" %2x", buf[i]);
    }
  printf("-----\n\n\n");
}


struct Queue
{
    uint32_t front, rear, size;
    uint32_t capacity;
    uint8_t* array;
};
// function to create a queue of given capacity.
// It initializes size of queue as 0
struct Queue* createQueue(uint32_t capacity)
{
    struct Queue* queue = (struct Queue*) malloc(sizeof(struct Queue));
    queue->capacity = capacity;
    queue->front = queue->size = 0;
    queue->rear = capacity - 1;  // This is important, see the enqueue
    queue->array = (uint8_t*) malloc(queue->capacity * sizeof(uint8_t));
    return queue;
}

void destructQueue(struct Queue** queue)
{
  free((*queue)->array);
  free(*queue);
}

// Queue is full when size becomes equal to the capacity
int isFull(struct Queue* queue)
{  return (queue->size == queue->capacity);  }

// Queue is empty when size is 0
int isEmpty(struct Queue* queue)
{  return (queue->size == 0); }

// Function to add an item to the queue.
// It changes rear and size
void enqueue(struct Queue* queue, uint8_t item)
{
    if (isFull(queue))
        return;
    queue->rear = (queue->rear + 1)%queue->capacity;
    queue->array[queue->rear] = item;
    queue->size = queue->size + 1;
}

// Function to remove an item from queue.
// It changes front and size
uint8_t dequeue(struct Queue* queue)
{
    if (isEmpty(queue))
    {
      printf("error: queue is empty\n");
      return 0;
    }
    int item = queue->array[queue->front];
    queue->front = (queue->front + 1)%queue->capacity;
    queue->size = queue->size - 1;
    return item;
}

// Function to get front of queue
uint32_t front(struct Queue* queue)
{
    if (isEmpty(queue))
    {
      printf("error: queue is empty\n");
      return 0;
    }
    return queue->array[queue->front];
}

// Function to get rear of queue
uint32_t rear(struct Queue* queue)
{
    if (isEmpty(queue))
    {
      printf("error: queue is empty\n");
      return 0;
    }
    return queue->array[queue->rear];
}

void reset_queue(struct Queue* queue) {
   queue->front = queue->size = 0;
   queue->rear = queue->capacity - 1;  // This is important, see the enqueue
}

void expand_queue(struct Queue** queue, uint32_t size)
{
  struct Queue *new_queue = createQueue(size);
  struct Queue **old_queue = queue;
  for(uint32_t i = 0; i < (*queue)->size; i++) enqueue(new_queue, dequeue(*queue));
  destructQueue(old_queue);
  *queue = new_queue;

}

struct Queue *client_queue;
struct Queue *server_queue;

pthread_mutex_t queue_lock;

sem_t emptied;
sem_t mutex;
sem_t filled;

int server_port = 8080;
char server_ip[40] = {0};

int video_decoder_running = 1;

static int video_decode()
{
   OMX_VIDEO_PARAM_PORTFORMATTYPE format;
   OMX_TIME_CONFIG_CLOCKSTATETYPE cstate;
   COMPONENT_T *video_decode = NULL, *video_scheduler = NULL, *video_render = NULL, *clock = NULL;
   COMPONENT_T *list[5];
   TUNNEL_T tunnel[4];
   ILCLIENT_T *client;
   int status = 0;
   uint32_t data_len = 0;

   memset(list, 0, sizeof(list));
   memset(tunnel, 0, sizeof(tunnel));


   if((client = ilclient_init()) == NULL)
   {
      return -3;
   }

   if(OMX_Init() != OMX_ErrorNone)
   {
      ilclient_destroy(client);
      return -4;
   }

   // create video_decode
   if(ilclient_create_component(client, &video_decode, "video_decode", ILCLIENT_DISABLE_ALL_PORTS | ILCLIENT_ENABLE_INPUT_BUFFERS) != 0)
      status = -14;
   list[0] = video_decode;

   // create video_render
   if(status == 0 && ilclient_create_component(client, &video_render, "video_render", ILCLIENT_DISABLE_ALL_PORTS) != 0)
      status = -14;
   list[1] = video_render;

   // create clock
   if(status == 0 && ilclient_create_component(client, &clock, "clock", ILCLIENT_DISABLE_ALL_PORTS) != 0)
      status = -14;
   list[2] = clock;

   memset(&cstate, 0, sizeof(cstate));
   cstate.nSize = sizeof(cstate);
   cstate.nVersion.nVersion = OMX_VERSION;
   cstate.eState = OMX_TIME_ClockStateWaitingForStartTime;
   cstate.nWaitMask = 1;
   if(clock != NULL && OMX_SetParameter(ILC_GET_HANDLE(clock), OMX_IndexConfigTimeClockState, &cstate) != OMX_ErrorNone)
      status = -13;

   // create video_scheduler
   if(status == 0 && ilclient_create_component(client, &video_scheduler, "video_scheduler", ILCLIENT_DISABLE_ALL_PORTS) != 0)
      status = -14;
   list[3] = video_scheduler;

   set_tunnel(tunnel, video_decode, 131, video_scheduler, 10);
   set_tunnel(tunnel+1, video_scheduler, 11, video_render, 90);
   set_tunnel(tunnel+2, clock, 80, video_scheduler, 12);

   // setup clock tunnel first
   if(status == 0 && ilclient_setup_tunnel(tunnel+2, 0, 0) != 0)
      status = -15;
   else
      ilclient_change_component_state(clock, OMX_StateExecuting);

   if(status == 0)
      ilclient_change_component_state(video_decode, OMX_StateIdle);

   memset(&format, 0, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
   format.nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
   format.nVersion.nVersion = OMX_VERSION;
   format.nPortIndex = 130;
   format.eCompressionFormat = OMX_VIDEO_CodingAVC;

   if(status == 0 &&
      OMX_SetParameter(ILC_GET_HANDLE(video_decode), OMX_IndexParamVideoPortFormat, &format) == OMX_ErrorNone &&
      ilclient_enable_port_buffers(video_decode, 130, NULL, NULL, NULL) == 0)
   {
      OMX_BUFFERHEADERTYPE *buf;
      int port_settings_changed = 0;
      int first_packet = 1;

      ilclient_change_component_state(video_decode, OMX_StateExecuting);
      while(((buf = ilclient_get_input_buffer(video_decode, 130, 1)) != NULL) && video_decoder_running)
      {
       // feed data and wait until we get port settings changed
       unsigned char *dest = buf->pBuffer;

       uint32_t data_size = 0;
       int buffer_ready = 0;
       do
       {
         pthread_mutex_lock(&queue_lock);
         data_size = client_queue->size;
         pthread_mutex_unlock(&queue_lock);
         if(data_size > OMX_buffer_handoff_size)
         {
           buffer_ready = 1;
         }
         else
         {
           buffer_ready = 0;
           sem_wait(&filled);
         }

       }while(!buffer_ready);

       pthread_mutex_lock(&queue_lock);
       data_size = OMX_buffer_handoff_size;
       if(data_size > buf->nAllocLen)data_size=buf->nAllocLen;
       for(uint32_t i = 0; i < data_size; i++) dest[i] = dequeue(client_queue);
       pthread_mutex_unlock(&queue_lock);
       data_len+=data_size;
       sem_post(&emptied);
       //hex_block_print(dest, 100);



         if(port_settings_changed == 0 &&
            ((data_len > 0 && ilclient_remove_event(video_decode, OMX_EventPortSettingsChanged, 131, 0, 0, 1) == 0) ||
             (data_len == 0 && ilclient_wait_for_event(video_decode, OMX_EventPortSettingsChanged, 131, 0, 0, 1,
                                                       ILCLIENT_EVENT_ERROR | ILCLIENT_PARAMETER_CHANGED, 10000) == 0)))
         {
            port_settings_changed = 1;

            if(ilclient_setup_tunnel(tunnel, 0, 0) != 0)
            {
               status = -7;
               break;
            }

            ilclient_change_component_state(video_scheduler, OMX_StateExecuting);

            // now setup tunnel to video_render
            if(ilclient_setup_tunnel(tunnel+1, 0, 1000) != 0)
            {
               status = -12;
               break;
            }

            ilclient_change_component_state(video_render, OMX_StateExecuting);
         }
         if(!data_len)
            break;

         buf->nFilledLen = data_len;
         data_len = 0;

         buf->nOffset = 0;
         if(first_packet)
         {
            buf->nFlags = OMX_BUFFERFLAG_STARTTIME;
            first_packet = 0;
         }
         else
            buf->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN;


         if(OMX_EmptyThisBuffer(ILC_GET_HANDLE(video_decode), buf) != OMX_ErrorNone)
         {
            status = -6;
            break;
         }
      }

      buf->nFilledLen = 0;
      buf->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN | OMX_BUFFERFLAG_EOS;

      if(OMX_EmptyThisBuffer(ILC_GET_HANDLE(video_decode), buf) != OMX_ErrorNone)
         status = -20;

      // wait for EOS from render
      // ilclient_wait_for_event(video_render, OMX_EventBufferFlag, 90, 0, OMX_BUFFERFLAG_EOS, 0,
      //                         ILCLIENT_BUFFER_FLAG_EOS, -1);

      // need to flush the renderer to allow video_decode to disable its input port
      ilclient_flush_tunnels(tunnel, 0);


   }

   ilclient_disable_tunnel(tunnel);
   ilclient_disable_tunnel(tunnel+1);
   ilclient_disable_tunnel(tunnel+2);
   ilclient_disable_port_buffers(video_decode, 130, NULL, NULL, NULL);
   ilclient_teardown_tunnels(tunnel);

   ilclient_state_transition(list, OMX_StateIdle);
   ilclient_state_transition(list, OMX_StateLoaded);

   ilclient_cleanup_components(list);

   OMX_Deinit();

   ilclient_destroy(client);
   return status;
}


void *client_network_thread(void *vargp)
{
    int sock = 0, valread=0;
    struct sockaddr_in serv_addr;
    char buffer[CHUNK_SIZE] = {0};
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, server_ip, &serv_addr.sin_addr)<=0)
    {
        printf("\nInvalid address/ Address not supported \n");
        video_decoder_running = 0;
        return NULL;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        video_decoder_running = 0;
        return NULL;
    }

    uint32_t data_size = 0;
    int buffer_room = 0;
    uint32_t sz;
    uint32_t capacity = 0;
    while(valread>=0)
    {
      valread = read( sock , buffer, CHUNK_SIZE);

      do
      {
        pthread_mutex_lock(&queue_lock);
        data_size = client_queue->size;
        capacity = client_queue->capacity;
        pthread_mutex_unlock(&queue_lock);
        if(capacity-data_size > CHUNK_SIZE*2)
        {
          buffer_room = 1;
        }
        else
        {
          buffer_room = 0;

          pthread_mutex_lock(&queue_lock);
          sz = capacity*2;
          expand_queue(&client_queue, sz);
          pthread_mutex_unlock(&queue_lock);
          printf("expanded buffer to %d", sz);
          sem_wait(&emptied);
        }

      }while(!buffer_room);

      pthread_mutex_lock(&queue_lock);
      for(uint32_t i = 0; i < valread; i++) enqueue(client_queue, buffer[i]);
      pthread_mutex_unlock(&queue_lock);
      sem_post(&filled);


    }

    video_decoder_running = 0;
    return NULL;
}

int main (int argc, char **argv)
{
client_queue = createQueue(OMX_buffer_handoff_size);
  sem_init(&mutex, 0, 1);
  sem_init(&filled, 0, 0);
  sem_init(&emptied, 0, 1);

  // start off with 100 and resize later if network is trying to shove
  // in more than it can handle
  // queue_size = OMX_buffer_handoff_size+1;
  // queue_array = (uint8_t *)malloc(queue_size);



  if (pthread_mutex_init(&queue_lock, NULL) != 0)
    {
        printf("\n mutex init has failed\n");
        return 1;
    }

    strcpy(server_ip, argv[1]);
    server_port = atoi(argv[2]);

    printf("opening  %s  port %d\n", server_ip, server_port);

    pthread_t thread_id;
    printf("Starting network thread\n");
    pthread_create(&thread_id, NULL, client_network_thread, NULL);

    bcm_host_init();
    printf("Entering decoder\n");
    video_decode();
    printf("Decode stopped\n");

    pthread_join(thread_id, NULL);
    printf("Network thread closed\n");
    //free(queue_array);
}


int get_next_chunk(uint8_t *buf)
{
  int ret_val = 0;
  pthread_mutex_lock(&queue_lock);
  if(server_queue->size>CHUNK_SIZE)
  {
  for(uint32_t i=0; i<CHUNK_SIZE;i++)buf[i]=dequeue(server_queue);
  ret_val = 1;
  }
  else
  {
  ret_val = -1;
  }
  pthread_mutex_unlock(&queue_lock);
  return ret_val;

}
pthread_mutex_t server_lock;
sem_t server_buf_ready;
int server_socket;
int server_fd;
int server_addrlen;
struct sockaddr_in server_address;
int server_connected = 0;
int server_has_init = 0;
uint32_t chosen_server_buffer_size = 0;
void server_init(int port, uint32_t buffer_size)
{
    server_queue = createQueue(buffer_size);
    chosen_server_buffer_size = buffer_size;
    printf("xxxxxxxx%d\n", chosen_server_buffer_size);

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
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                                                  &opt, sizeof(opt)))
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
    sem_init(&server_buf_ready, 0, 1);
    if (pthread_mutex_init(&queue_lock, NULL) != 0)
      {
          printf("\n mutex init has failed\n");
      }
    if (pthread_mutex_init(&server_lock, NULL) != 0)
      {
          printf("\n mutex init has failed\n");
      }
    server_connected = 0;
    server_has_init = 1;
}

uint8_t *PPS_buffer = NULL;
uint8_t *SPS_buffer = NULL;
uint8_t PPS_sz = 0;
uint8_t SPS_sz = 0;

int insert_PPS_SPS()
{
  int ret_val = -1;
  pthread_mutex_lock(&server_lock);
  if(PPS_sz > 0 && SPS_sz > 0)
  {
  pthread_mutex_lock(&queue_lock);
  reset_queue(server_queue);
  for(uint32_t i = 0; i < SPS_sz; i++) enqueue(server_queue, SPS_buffer[i]);
  for(uint32_t i = 0; i < PPS_sz; i++) enqueue(server_queue, PPS_buffer[i]);
  pthread_mutex_unlock(&queue_lock);
  ret_val = 1;
  }
  pthread_mutex_unlock(&server_lock);
  return ret_val;

}

uint8_t server_buf[CHUNK_SIZE];
int send_code;
void server_loop()
{
  //wait for chunk size to be ready
  sem_wait(&server_buf_ready);
  if (get_next_chunk(server_buf) == 1)
  {
    if(server_connected) send_code = write(server_socket , server_buf , CHUNK_SIZE);
    if(send_code < 0) server_connected = 0;

    if(!server_connected)
    {
      printf("waiting on connect\n");
     server_socket = accept(server_fd, (struct sockaddr *)&server_address,(socklen_t*)&server_addrlen);
     server_connected = 1;
     printf("connected\n");
     while(insert_PPS_SPS() != 1) sleep(1);
     printf("inserting PPS SPS\n");
   }
  }

}

const uint8_t magic_pattern[] = {0x00, 0x00, 0x00, 0x01};
uint8_t pattern_len = 4;
uint8_t *NAL_buffer = NULL;
uint32_t NAL_buffer_sz = 0;
uint32_t NAL_buf_cnt = 0;
uint8_t pattern_pointer = 0;
uint8_t NAL_signal = 0;
void record_bytes(uint8_t *buf, uint32_t buf_size)
{
  uint8_t new_NAL = 0;
  uint32_t new_NAL_sz = 0;
  if(NAL_buffer == NULL)
  {
    NAL_buffer = (uint8_t *)malloc(buf_size);
    NAL_buffer_sz = buf_size;
  }
  else
  {
    // check if we're going to probe further than what
    // the NAL buffer can hold
    uint32_t probe_size = buf_size+NAL_buf_cnt+1;
    if (probe_size > NAL_buffer_sz)
    {
      uint8_t *temp = (uint8_t *)realloc(NAL_buffer, probe_size);
      if(temp != NULL)
      {
        NAL_buffer = temp;
        NAL_buffer_sz = probe_size;
        printf("probe resized: %d\n", NAL_buffer_sz);
      }
      else
      {
        printf("failed memory realloc\n");
        NAL_buf_cnt = 0;
        pattern_pointer = 0;
        NAL_signal = 0;
        return;
      }
    }
  }
  for(uint32_t i = 0; i < buf_size; i++)
  {
    sem_post(&server_buf_ready); // kick server
    new_NAL = 0;
    NAL_buffer[NAL_buf_cnt] = buf[i];
    NAL_buf_cnt++;
    if(magic_pattern[pattern_pointer] == buf[i])
    {
      pattern_pointer++;
    }
    else
    {
      pattern_pointer=0;
    }
    if(pattern_pointer == pattern_len)
    {
      NAL_signal++;
      pattern_pointer=0;
    }

    if(NAL_signal==2)
    {
      NAL_signal=1;
      new_NAL=1;
    }

    if(new_NAL)
    {

      pthread_mutex_lock(&server_lock);
      new_NAL_sz = NAL_buf_cnt-4;

      pthread_mutex_lock(&queue_lock);

      if(new_NAL_sz > chosen_server_buffer_size)
      {
        uint32_t new_sz = new_NAL_sz*2;
        printf("expanding buffer size to accommodate at least one NAL unit: %d\n", new_sz);
        expand_queue(&server_queue, new_sz);
        chosen_server_buffer_size = server_queue->capacity;
      }

      static int need_to_send_I_frame = 0;
      if(server_queue->capacity-server_queue->size > new_NAL_sz)
      {
        //printf("moving in %d\n",new_NAL_sz);
        if(need_to_send_I_frame)
        {
          // Push in an I frame if there's a buffer overrun
          // so that it doesn't distort from macroblocks too far
          // into the future
          if(((NAL_buffer[4])&0x1F) == NAL_UNIT_TYPE_IDR)
          {
          for(uint32_t i=0; i<new_NAL_sz;i++)enqueue(server_queue, NAL_buffer[i]);
          need_to_send_I_frame = 0;
          }
        }
        else
        {
          for(uint32_t i=0; i<new_NAL_sz;i++)enqueue(server_queue, NAL_buffer[i]);
        }
      }
      else
      {
        //printf("buffer overrun\n");
        need_to_send_I_frame = 1;
      }
      pthread_mutex_unlock(&queue_lock);


      if(((NAL_buffer[4])&0x1F) == NAL_UNIT_TYPE_PPS)
      {
        if(PPS_sz == 0)
        {
          PPS_sz = new_NAL_sz;
          PPS_buffer = (uint8_t *)malloc(PPS_sz);
          for(uint32_t i=0; i<PPS_sz;i++) PPS_buffer[i] = NAL_buffer[i];
          printf("found PPS %d\n",PPS_sz);
       }
      }
      if(((NAL_buffer[4])&0x1F) == NAL_UNIT_TYPE_SPS)
      {
        if(SPS_sz == 0)
        {
          SPS_sz = new_NAL_sz;
          SPS_buffer = (uint8_t *)malloc(SPS_sz);
          for(uint32_t i=0; i<SPS_sz;i++) SPS_buffer[i] = NAL_buffer[i];
          printf("found SPS %d\n", SPS_sz);
       }
      }
      new_NAL=0;
      NAL_buffer[0]=0x00;
      NAL_buffer[1]=0x00;
      NAL_buffer[2]=0x00;
      NAL_buffer[3]=0x01;
      NAL_buf_cnt=4;
      pthread_mutex_unlock(&server_lock);
    }
  }
}
