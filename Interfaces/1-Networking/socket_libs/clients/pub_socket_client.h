#ifndef PUB_SOCKET_CLIENT_H
#define PUB_SOCKET_CLIENT_H

/*-------------------------------------
|           PUBLIC INCLUDES            |
--------------------------------------*/

#include <stdint.h>

/*-------------------------------------
|            PUBLIC DEFINES            |
--------------------------------------*/

#define SERVER_ADDR                     "192.168.200.1"            /* Address of the server we are conencting to. Note, this can be a IP or hostname */
// #define SERVER_ADDR                     "raspberrypi"              /* Address of the server we are conencting to. Note, this can be a IP or hostname */
#define TIME_BETWEEN_CONNECTION_ATTEMPTS (2)                       /* Time in seconds between attempts to connect to server if we fail to connect */
#define SERVER_PING_TIMEOUT              (2*TIME_BETWEEN_PINGS)    /* Max time allowed before we assume the server has disconnected. If we haven't received a \
                                                                        message from the server within this amount of time (s), disconnect and assume failure */

/*-------------------------------------
|     PUBLIC FUNCTION DECLARATIONS     |
--------------------------------------*/

/**
 * Initialize the server 
 * @Return the server's socket_fd or RETURN_FAILED if failed
 * 
 * Blocking Function
 */
int socket_client_init(char *port);

/**
 * Calling this function will run the socket client. Includes
 * receiving messages, and pings every TIME_BETWEEN_PINGS seconds. 
 * Does not return unless major internal failure. 
 * 
 * Blocking Function
 */
void socket_client_execute();

/**
 * This function is part of the libcommon in the common folder.
 * 
 * Given a char* data array up to 2^16 in size and a socket_fd,
 * will send data array over socket.
 * 
 * Note that data_sz should include the termination character if applicable.
 * 
 * Note that calls to this are thread safe as long as the size of data is 
 *  less than MAX_MSG_SZ.
 * 
 * @Returns number of bytes sent or RETURN_FAILED or 0 if there's an error.
 */
extern int socket_send_data ( const int socket_fd, const char * data, const uint16_t data_sz );

/**
 * Given an initialied socket fd, closes the socket connection.
 * If fails, program will exit.
 * 
 * Blocking.
 */
void socket_client_quit(const int socket_fd);

//TODO Function to stop client.
//TODO execute should be non-blocking

//TODO callbacks for messages received over socket

#endif /* PUB_SOCKET_CLIENT_H */
