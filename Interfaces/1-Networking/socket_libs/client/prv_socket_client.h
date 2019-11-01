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
|           PRIVATE DEFINES            |
--------------------------------------*/

#define RETURN_DISCONNECT   (-2) /* Used by process_recv_msg() to indicate we disconnected from the server */


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
 * Given a server's socket fd, receives messages from server and
 * performs any appropiate actions for the message received.
 * This includes calling the rx_callback.
 * This function is called by the (client) execute thread.
 * 
 * @Returns RETURN_FAILED (indicates the connection to server
 * is broken and socket should be closed), or RETURN_SUCCESS
 * indicating message processed appropriately.
 * 
 * Blocking Function
 */
int process_recv_msg(const int socket_fd);

/**
 * Given a char* command, char* data array up to 2^16 in size, and its size,
 * this function will concatenate the command to the data array and
 * will send data array over socket to the client specific by the char* uuid.
 * 
 * Note that data_sz should include the termination character if applicable.
 * 
 * Note, data may be NULL and data_sz == 0 to send only the command
 * 
 * Note that calls to this are thread safe.
 * 
 * Blocking Function
 * 
 * @Returns number of bytes sent or RETURN_FAILED or 0 if there's an error.
 */
int send_data ( const uint8_t command, const char * data, uint data_sz );

/**
 * This thread is spawned by socket_client_execute() and calls
 * the process_recv_msg() for received messages or the discnt_callback
 * if disconnected from the server. Call socket_client_quit() to kill
 * thread.
 * 
 * @Returns always returns NULL
 */
void* execute_thread(void* args);

/**
 */
void close_and_notify();

#endif /* PRV_SOCKET_CLIENT_H */
