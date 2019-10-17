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

/** //TODO REMOVE this function is used for converting passed string parameter to port number which will be done in the tcp service rather than here in the library
 * Check parameters and return port number if valid (else NULL)
 */
char* check_parameters(int argc, char *argv[]);

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
 * appropiate to that message.
 * 
 * Blocking Function
 */
void process_recv_msg(const char* buffer, const int buffer_sz);

/**
 * 
 */
void* execute_thread(void* args)

#endif /* PRV_SOCKET_CLIENT_H */
