#ifndef PRV_SOCKET_CLIENT_H
#define PRV_SOCKET_CLIENT_H

/*-------------------------------------
|           PUBLIC INCLUDES            |
--------------------------------------*/

#include "pub_socket_commons.h" /* From static library */

#include <uuid/uuid.h>
#include <unistd.h>
#include <pthread.h> 
#include <stdbool.h>


/*-------------------------------------
|    PRIVATE FUNCTION DECLARATIONS     |
--------------------------------------*/

/**
 * Generate uuid and set UUID variable to the new uuid.
 * 
 * Blocking Function
 */
void uuid_create();

/**
 * Reads UUID from file. If UUID file doesn't exist, a UUID is generated
 * and the file is created for future use.
 * Asserts if failed to read file that exists.
 */
void load_uuid();

/**
 * Send our UUID to socket server.
 * @Returns number of bytes sent
 *          or RETURN_FAILED if failed.
 * 
 * Blocking Function
 */
int send_uuid();

/**
 * Given a received message, determines and performs any actions
 * appropiate to that message. This includes calling the rx_callback.
 * This function is called by the (client) execute thread.
 * 
 * Blocking Function
 */
void process_recv_msg(const char* buffer, const int buffer_sz);

/**
 * This thread is spawned by socket_client_execute() and calls
 * the process_recv_msg() for received messages or the discnt_callback
 * if disconnected from the server. Call socket_client_quit() to kill
 * thread.
 */
void* execute_thread(void* args);

#endif /* PRV_SOCKET_CLIENT_H */
