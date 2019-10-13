#ifndef PUB_SOCKET_SERVER_H
#define PUB_SOCKET_SERVER_H

/*-------------------------------------
|           PUBLIC INCLUDES            |
--------------------------------------*/


/*-------------------------------------
|            PUBLIC DEFINES            |
--------------------------------------*/

#define MAX_PENDING_CONNECTIONS  (5)        /* 5 is a standard value for the max backlogged connection requests */
#define SERVER_ADDR              (NULL)     /* Set this value to a string that is the IP address, hostname or the server you're creating or set to NULL (0) to use this machines address (NULL recommended) */
#define BUFFER_SZ                (1024)     /* Size of the buffer we use to send/receive data */
#define SELECT_TIMEOUT_TIME      (100)      /* How long Select() will block if no message is received */


/*-------------------------------------
|     PUBLIC FUNCTION DECLARATIONS     |
--------------------------------------*/

/**
 * Initialize the server 
 * @Return the server's socket_fd or asserts if failed
 * 
 * Blocking Function
 */
int init_server(char* port);

/**
 * Sends a message via the socket connection to all
 * connected clients.
 * 
 * @Returns total number of bytes sent 
 *          (num_clients*(buffer_sz + MSG_HEADER_SZ + 1))
 *          or a negative number if failed
 * 
 * Blocking Function
 *
 * Thread Safe: For messages smaller than MAX_MSG_SZ
 */
int send_to_all(const char* buffer, const int buffer_sz);

/**
 * Calling this function will run the socket server. Includes
 * accepting client connections, receiving messages, sending messages,
 * pinging clients every TIME_BETWEEN_PINGS seconds, and determines if
 * a client has disconnected. Does not return unless major internal 
 * failure. 
 * 
 * Blocking Function
 */
void execute_server();

//TODO Function to stop server. Look at INThandler to see how to safely close server
//TODO add callbacks for client connected, disconnected, and message received
//TODO get list of clients connected

#endif /* PUB_SOCKET_SERVER_H */
