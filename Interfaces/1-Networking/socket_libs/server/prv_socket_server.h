#ifndef PRV_SOCKET_SERVER_H
#define PRV_SOCKET_SERVER_H

/*-------------------------------------
|           PUBLIC INCLUDES            |
--------------------------------------*/

#include <stdio.h>
#include <stdlib.h>

#include <errno.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <assert.h>
#include <time.h>
#include  <signal.h>

#include "pub_socket_commons.h" /* From static library */

/*-------------------------------------
|           PRIVATE STRUCTS            |
--------------------------------------*/

struct client_info
{
    int fd;
    char uuid[UUID_SZ+1]; /* +1 for termination char */
    struct in_addr address;
    time_t lastPing;
    struct client_info *next;
};


/*-------------------------------------
|    PRIVATE FUNCTION DECLARATIONS     |
--------------------------------------*/

/** //TODO REMOVE this function is used for converting passed string parameter to port number which will be done in the tcp service rather than here in the library
 * Check parameters and return port number if valid (else NULL)
 */
char* check_parameters(int argc, char *argv[]);

/**
 * Handle signal interupt by safely shutting down server (ctrl + c)
 * and quitting program. DOES NOT RETURN
 */
void  INThandler(int sig);

/**
 * Handle signal pipe (pipe error: happens when sending data to 
 * client but client is no longer connected). This function being
 * called ignifies client disconnected).
 * 
 * Blocking Function
 */
void  PIPEhandler(int sig);

/**
 * Given a client's socket fd, searches the static struct client_info *client_infos
 * (which is a linked list of structs) for the struct containing the 
 * desired client.
 *  
 * @Returns the pointer to the client_info struct containing
 *          the desired client socket fd number. 
 *          Returns NULL if not found.
 * 
 * Blocking Function
 */
struct client_info* find_client_by_fd(const int socket);

/**
 * Given a client's UUID, searches the static struct client_info *client_infos
 * (which is a linked list of structs) for the struct containing the 
 * desired client.
 *  
 * @Returns the pointer to the client_info struct containing
 *          the desired client UUID. Returns NULL if not found.
 * 
 * Blocking Function
 */
struct client_info* find_client_by_uuid(const char* uuid);

/**
 * Accepts incoming client connection requests.
 * 
 * @Returns the new client's socket fd (file descriptor)
 * 
 * Blocking Function
 */
int handle_conn_request();

/**
 * Given a client's fd, processes any received messages from 
 * client.
 * 
 * @Returns number of bytes received or negative number if failed
 * 
 * Blocking Function
 */
int process_recv_msg(int client_fd);

/**
 * Given a socket fd set, checks if messages have been received 
 * from any clients and handles receiving/processing any messages 
 * by calling process_recv_msg()
 * 
 * Blocking Function
 */
void service_sockets(const fd_set *read_fd_set);

/**
 * Given a client's socket fd, closes the connection to that client 
 * and removes client from client_infos struct and active_fd_set.
 * 
 * Blocking Function
 */
void close_conn(int client_fd);

#endif /* PRV_SOCKET_SERVER_H */
