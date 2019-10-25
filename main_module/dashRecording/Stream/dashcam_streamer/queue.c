// Henry Bergin 2019
#include "queue.h"
#include <assert.h>

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
    {
      printf("error: queue is full\n");
      return;
    }
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

void reset_queue(struct Queue* queue)
{
   queue->size = 0;
   queue->front = 0;
   queue->rear = queue->capacity - 1;  // This is important, see the enqueue
}

void expand_queue(struct Queue** queue, uint32_t size)
{
  struct Queue *new_queue = createQueue(size);
  struct Queue **old_queue = queue;
  uint32_t items_to_remove = (*queue)->size;
  for(uint32_t i = 0; i < items_to_remove; i++) enqueue(new_queue, dequeue(*queue));
  if((*queue)->size != 0)
  {
    printf("Error: queue size is still %d\n", (*queue)->size);
    assert((*queue)->size == 0);
  }

  destructQueue(old_queue);
  *queue = new_queue;
}
