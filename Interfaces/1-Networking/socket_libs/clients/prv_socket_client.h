#ifndef PRV_SOCKET_CLIENT_H
#define PRV_SOCKET_CLIENT_H

/*-------------------------------------
|           PUBLIC INCLUDES            |
--------------------------------------*/

#include "pub_socket_commons.h" /* From static library */

#include <uuid/uuid.h>
#include <unistd.h>


/*-------------------------------------
|           PRIVATE STRUCTS            |
--------------------------------------*/


/*-------------------------------------
|    PRIVATE FUNCTION DECLARATIONS     |
--------------------------------------*/

/** //TODO REMOVE this function is used for converting passed string parameter to port number which will be done in the tcp service rather than here in the library
 * Check parameters and return port number if valid (else NULL)
 */
char* check_parameters(int argc, char *argv[]);

/* Generate uuid and set UUID to the new uuid */
void uuid_create();

/* Reads UUID from file. If UUID file doesn't exist, it is created 
    -Returns -1 if failed. */
void load_uuid();

/* Send our UUID to server. 
    -Returns -1 if failed else number of bytes sent. */
int send_uuid();

/* Given a buffer of a received message from server, 
    performs actions appropiate. */
void process_recv_msg(const char* buffer, const int buffer_sz);

#endif /* PRV_SOCKET_CLIENT_H */
