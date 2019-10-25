// Henry Bergin 2019

#ifndef QUEUE_H_
#define QUEUE_H_
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "bcm_host.h"

struct Queue
{
    uint32_t front, rear, size;
    uint32_t capacity;
    uint8_t* array;
};

struct Queue* createQueue(uint32_t capacity);
void destructQueue(struct Queue** queue);
int isFull(struct Queue* queue);
int isEmpty(struct Queue* queue);
void enqueue(struct Queue* queue, uint8_t item);
uint8_t dequeue(struct Queue* queue);
uint32_t front(struct Queue* queue);
uint32_t rear(struct Queue* queue);
void reset_queue(struct Queue* queue);
void expand_queue(struct Queue** queue, uint32_t size);

#endif
