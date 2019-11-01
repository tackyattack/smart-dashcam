#ifndef PUB_SOCKET_SERVER_H
#define PUB_SOCKET_SERVER_H

#ifdef __cplusplus
extern "C" {
#endif

/*-------------------------------------
|           PUBLIC INCLUDES            |
--------------------------------------*/

#include <stdbool.h>


/*-------------------------------------
|            PUBLIC DEFINES            |
--------------------------------------*/

#define MAX_PENDING_CONNECTIONS  (5)                        /* 5 is a standard value for the max backlogged connection requests */
#define SERVER_ADDR              (NULL)                     /* Set this value to a string that is the IP address, hostname or the server you're creating or set to NULL (0) to use this machines address (NULL recommended) */
#define BUFFER_SZ                (MAX_MSG_SZ)               /* Size of the buffer we use to send/receive data */
#define SELECT_TIMEOUT_TIME      (TIME_BETWEEN_PINGS*100)   /* How long Select() will block in milliseconds if no message is received in the execute_thread().\
                                                               In other words, this controls the how often the server checks to see if it is time to ping clients.\
                                                               Generally, this can match the value (TIME_BETWEEN_PINGS*1000).*/


/*-------------------------------------
|    FUNCTION POINTER DECLARATIONS     |
--------------------------------------*/

/* https://isocpp.org/wiki/faq/mixing-c-and-cpp */
typedef void (*socket_lib_srv_rx_msg)(const char* uuid, const char* data, const unsigned int data_sz);
typedef void (*socket_lib_srv_connected)(const char* uuid);
typedef void (*socket_lib_srv_disconnected)(const char* uuid);


/*-------------------------------------
|     PUBLIC FUNCTION DECLARATIONS     |
--------------------------------------*/

/**
 * Initialize the server and accept incoming connections 
 * given a port number as a string and 3 callbacks.
 * rx_callback is called by the execute_server thread when a
 * message is received from a client.
 * 
 * connect_callback is called
 * when a client has connected to the server.
 * 
 * discon_callback is called
 * when a client has disconnected/lost connection to the server.
 * 
 * Currently, there is only one instance of a server
 * available. As such, do not call this function more than
 * once unless it's after calling socket_server_quit();
 * 
 * The callbacks can be NULL if desired.
 * 
 * @Return Returns server's socket_fd or a negative number if 
 * failed to init the server.
 * 
 * Non-Blocking Function
 */
int socket_server_init( char* port, socket_lib_srv_rx_msg rx_callback, socket_lib_srv_connected connect_callback, socket_lib_srv_disconnected discon_callback );

/**
 * Sends data of size data_sz via the socket connection
 * to all clients.
 * 
 * @Returns total number of bytes sent 
 *          (num_clients*(data_sz + MSG_HEADER_SZ + 1))
 *          or a negative number if failed
 * 
 * Blocking Function
 *
 * Thread Safe
 */
int socket_server_send_data_all( const char* data, const unsigned int data_sz );

/**
 * Sends data of size data_sz via the socket connection
 * to a specific client specified by the given UUID.
 *
 * @Returns total number of bytes sent 
 *          (data_sz + MSG_HEADER_SZ + 1)
 *          or a negative number if failed
 *
 * Blocking Function
 *
 * Thread Safe
 */
int socket_server_send_data( const char* uuid, const char* data, unsigned int data_sz );

/**
 * Calling this function will spawn a thread to run the 
 * socket server for tasks including receiving messages,
 * sending messages, accepting client connections sending
 * pings to clients every TIME_BETWEEN_PINGS seconds, and 
 * processing messages.
 * Thread will quit if major internal error. This function 
 * indirectly handles all the callbacks as well (callback when 
 * messages received, client connects, and client disconnects).
 * 
 * Non-Blocking Function
 */
void socket_server_execute();

/**
 * @Returns True if client thread is executing, else false.
 */
bool socket_server_is_executing();

/**
 * Given an initialied socket fd, closes the socket connection and kills the 
 * socket_client_execute() thread (this may take up to SERVER_PING_TIMEOUT
 * seconds to happen). Frees all the memory associated with client_infos and
 * closes connection with all clients.
 * 
 * Non-Blocking
 */
void socket_server_quit();


#ifdef __cplusplus
}
#endif

#endif /* PUB_SOCKET_SERVER_H */
