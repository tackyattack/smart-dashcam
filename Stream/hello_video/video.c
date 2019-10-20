/*
Copyright (c) 2012, Broadcom Europe Ltd
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// Video deocode demo using OpenMAX IL though the ilcient helper library

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

#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>

#define MAX 40000

int intArray[MAX];
int front = 0;
int rear = -1;
int itemCount = 0;

pthread_mutex_t queue_lock;

int peek() {
   return intArray[front];
}

bool isEmpty() {
   return itemCount == 0;
}

bool isFull() {
   return itemCount == MAX;
}

int size() {
   return itemCount;
}

void insert(int data) {
   if(!isFull()) {

      if(rear == MAX-1) {
         rear = -1;
      }

      intArray[++rear] = data;
      itemCount++;
   }
}

int removeData() {
   int data = intArray[front++];

   if(front == MAX) {
      front = 0;
   }

   itemCount--;
   return data;
}

sem_t empty;
sem_t mutex;
sem_t full;

unsigned char buf_nw[100];



static int video_decode_test(char *filename)
{
   OMX_VIDEO_PARAM_PORTFORMATTYPE format;
   OMX_TIME_CONFIG_CLOCKSTATETYPE cstate;
   COMPONENT_T *video_decode = NULL, *video_scheduler = NULL, *video_render = NULL, *clock = NULL;
   COMPONENT_T *list[5];
   TUNNEL_T tunnel[4];
   ILCLIENT_T *client;
   FILE *in;
   int status = 0;
   unsigned int data_len = 0;

   memset(list, 0, sizeof(list));
   memset(tunnel, 0, sizeof(tunnel));

   if((in = fopen(filename, "rb")) == NULL)
      return -2;

   if((client = ilclient_init()) == NULL)
   {
      fclose(in);
      return -3;
   }

   if(OMX_Init() != OMX_ErrorNone)
   {
      ilclient_destroy(client);
      fclose(in);
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
      int frame = 0;
      int byte_cnt = 0;
      int buf_cnt = 0;
      while((buf = ilclient_get_input_buffer(video_decode, 130, 1)) != NULL)
      {
         // feed data and wait until we get port settings changed
         unsigned char *dest = buf->pBuffer;

         //data_len += fread(dest, 1, buf->nAllocLen-data_len, in);
        // printf("%d\n", frame);
         // byte_cnt = 0;
         // while(byte_cnt<200)
         // {
         //   if(!isEmpty())
         //   {
         //   dest[byte_cnt] = removeData();
         //   byte_cnt++;
         //   }
         // }

       // for(int i = 0; i < 100; i++) dest[i] = buf_nw[i];
       // data_len += 100;
       // buf_ready = 0;

       if(buf->nAllocLen-data_len < 10*100) printf("error\n");

       for(int i = 0; i < 10; i++)
       {
       buf_cnt = 0;
       sem_wait(&full);
       sem_wait(&mutex);
       while(buf_cnt < 100)
       {
       dest[buf_cnt + i*100] = buf_nw[buf_cnt];
       buf_cnt++;
       }
    //    for (int ii = 0; ii < 100 ;ii++) {
    //     printf(" %2x", buf_nw[ii]);
    // }
    // printf("-----\n\n\n");
       //data_len += fread(dest, 1, buf->nAllocLen-data_len, in);
       sem_post(&mutex);
       sem_post(&empty);

       data_len += 100;
     }
       //printf("f\n");

       //data_len += fread(dest, 1, 10000, in);
       //data_len += fread(dest, 1, buf->nAllocLen-data_len, in);

       frame++;
       if(frame==100)
       {
         frame=0;
         rewind(in);
       }



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
      ilclient_wait_for_event(video_render, OMX_EventBufferFlag, 90, 0, OMX_BUFFERFLAG_EOS, 0,
                              ILCLIENT_BUFFER_FLAG_EOS, -1);

      // need to flush the renderer to allow video_decode to disable its input port
      ilclient_flush_tunnels(tunnel, 0);

   }

   fclose(in);

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
#define PORT 8080
void *myThreadFun(void *vargp)
{
  int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char *hello = "Hello from client";
    char buffer[100] = {0};
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, "131.151.175.144", &serv_addr.sin_addr)<=0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        return -1;
    }
    send(sock , hello , strlen(hello) , 0 );
    printf("Hello message sent\n");


    while(valread = read( sock , buffer, 100))
    {
      sem_wait(&empty);
      sem_wait(&mutex);
      for(int i = 0; i < 100; i++)
      {
        buf_nw[i] = buffer[i];
      }
      sem_post(&mutex);
      sem_post(&full);

    }



    return NULL;
}

int main (int argc, char **argv)
{
  sem_init(&mutex, 0, 1);
  sem_init(&full, 0, 0);
  sem_init(&empty, 0, 1);

  if (pthread_mutex_init(&queue_lock, NULL) != 0)
    {
        printf("\n mutex init has failed\n");
        return 1;
    }

    pthread_t thread_id;
    printf("Before Thread\n");
    pthread_create(&thread_id, NULL, myThreadFun, NULL);

    if (argc < 2) {
       printf("Usage: %s <filename>\n", argv[0]);
       exit(1);
    }
    bcm_host_init();
    return video_decode_test(argv[1]);

    pthread_join(thread_id, NULL);
    printf("After Thread\n");


}

#define chunk_size 100
int get_next_chunk(uint8_t *buf)
{
  if(size()>chunk_size)
  {
  pthread_mutex_lock(&queue_lock);
  for(int i=0; i<chunk_size;i++)buf[i]=removeData();
  pthread_mutex_unlock(&queue_lock);
  return 1;
  }
  return -1;

}

#define NAL_BUFFER_SIZE 0xffff
const uint8_t magic_pattern[] = {0x00, 0x00, 0x00, 0x01};
uint8_t pattern_len = 4;
uint8_t NAL_buffer[NAL_BUFFER_SIZE];
uint8_t PPS_found = 0;
uint8_t SPS_found = 0;
uint32_t NAL_buf_cnt = 0;
uint8_t pattern_pointer = 0;
uint8_t NAL_signal = 0;
void record_bytes(uint8_t *buf, uint32_t buf_size)
{
  uint8_t new_NAL = 0;
  uint32_t new_NAL_sz = 0;
  for(uint32_t i = 0; i < buf_size; i++)
  {
    new_NAL = 0;
    NAL_buffer[NAL_buf_cnt] = buf[i];
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
    //      for (int ii = 0; ii < 10 ;ii++) {
    //       printf(" %2x", NAL_buffer[ii]);
    //   }
      //printf("-----\n\n\n");
      //printf("%d\n", size());
      new_NAL_sz = NAL_buf_cnt-3;

      if(PPS_found && SPS_found)
      {
        pthread_mutex_lock(&queue_lock);
        if(MAX-size() > new_NAL_sz)
        {
        for(int i=0; i<new_NAL_sz;i++)insert(NAL_buffer[i]);
        }
        pthread_mutex_unlock(&queue_lock);
      }

      if(NAL_buffer[4] == 0x28)
      {
        if(!PPS_found)
        {
          pthread_mutex_lock(&queue_lock);
          if(MAX-size() > new_NAL_sz)
          {
          for(int i=0; i<new_NAL_sz;i++)insert(NAL_buffer[i]);
          }
          pthread_mutex_unlock(&queue_lock);
          printf("found PPS %d\n",new_NAL_sz);
       }
        PPS_found=1;
      }
      if(NAL_buffer[4] == 0x27)
      {
        if(!SPS_found)
        {
          pthread_mutex_lock(&queue_lock);
          if(MAX-size() > new_NAL_sz)
          {
          for(int i=0; i<new_NAL_sz;i++)insert(NAL_buffer[i]);
          }
          pthread_mutex_unlock(&queue_lock);
          printf("found SPS %d\n", new_NAL_sz);
       }
       SPS_found=1;
      }
      new_NAL=0;
      NAL_buffer[0]=0x00;
      NAL_buffer[1]=0x00;
      NAL_buffer[2]=0x00;
      NAL_buffer[3]=0x01;
      NAL_buf_cnt=4-1;
    }
    NAL_buf_cnt++;
  }
}
