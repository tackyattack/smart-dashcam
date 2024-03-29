#ifndef PUB_SOCKET_CLIENT_H
#define PUB_SOCKET_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

/*-------------------------------------
|           PUBLIC INCLUDES            |
--------------------------------------*/

#include <stdint.h>
#include <stdbool.h>

/*-------------------------------------
|            PUBLIC DEFINES            |
--------------------------------------*/
#define MAX_MSG_SZ                          (65536)                 /* max size of the message that can be sent over sockets. This is defined in pub_socket_commons.h */
#define SERVER_ADDR                         "192.168.200.1"         /* Address of the server we are conencting to. Note, this can be a IP or hostname */
// #define SERVER_ADDR                     "raspberrypi"            /* Address of the server we are conencting to. Note, this can be a IP or hostname */
#define SERVER_PORT                         "5555"                  /* This should match the value in pub_socket_commons.h */
#define TIME_BETWEEN_CONNECTION_ATTEMPTS    (2)                     /* Time in seconds between attempts to connect to server if we fail to connect */
#define SERVER_PING_TIMEOUT                 (2*TIME_BETWEEN_PINGS)  /* Max time allowed before we assume the server has disconnected. If we haven't received a \
                                                                       message from the server within this amount of time (s), disconnect and assume failure */


/*-------------------------------------
|    FUNCTION POINTER DECLARATIONS     |
--------------------------------------*/

/* https://isocpp.org/wiki/faq/mixing-c-and-cpp */
typedef void (*socket_lib_clnt_rx_msg)(const char* data, const unsigned int data_sz);
typedef void (*socket_lib_clnt_disconnected)(void);


/*-------------------------------------
|     PUBLIC FUNCTION DECLARATIONS     |
--------------------------------------*/

/**
 * Initialize the client and connect to server given the 
 * server's ip or hostname as a string, the port number 
 * as a string and the callback functions. 
 * 
 * rx_callback is called by the execute_client thread when
 * a message is received from the server 
 * 
 * discnt_callback is called when the connection
 * has disconnected/we lost connection to the server.
 * 
 * Currently, there is only one instance of a client
 * available. As such, do not call this function more than
 * once unless it's after calling socket_client_quit();
 * 
 * The callbacks can be NULL if desired.
 * 
 * @Return the client's socket_fd or RETURN_FAILED (-1) if 
 * the server is unavailable.
 * 
 * Non-Blocking Function
 */
int socket_client_init(char *server_addr, char *port, socket_lib_clnt_rx_msg rx_callback, socket_lib_clnt_disconnected discnt_callback);

/**
 * Calling this function will spawn a thread to run the 
 * socket client for tasks including receiving messages,
 * and responding to pings every TIME_BETWEEN_PINGS
 * seconds. Thread will quit if major internal error 
 * or the connection to the server is lost (which the
 * user will be notified via the socket_lib_clnt_disconnected
 * callback). The socket_lib_clnt_rx_msg is called when a
 * a message is received that isn't an internal command.
 * 
 * Non-Blocking Function
 */
void socket_client_execute();

/**
 * Given a char* data array up to 2^16 (MAX_MSG_SZ) in size and its size,
 * will send data array over socket.
 * 
 * Note that data_sz should include the termination character if applicable.
 * 
 * Note that calls to this are thread safe.
 * 
 * @Returns number of bytes sent RETURN_FAILED if there's an error.
 */
int socket_client_send_data ( const char * data, const uint data_sz );

/**
 * Closes the socket connection to server and kills the 
 * socket_client_execute() thread (this may take up to SERVER_PING_TIMEOUT
 * seconds to happen).
 * 
 * Non-Blocking
 */
void socket_client_quit();

/**
 * @Returns True if client thread is executing, else false.
 */
bool socket_client_is_executing();


#ifdef __cplusplus
}
#endif

#endif /* PUB_SOCKET_CLIENT_H */
