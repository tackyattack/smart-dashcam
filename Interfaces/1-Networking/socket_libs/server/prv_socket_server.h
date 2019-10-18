#ifndef PRV_SOCKET_SERVER_H
#define PRV_SOCKET_SERVER_H

/*-------------------------------------
|           PRIVATE INCLUDES           |
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
#include <signal.h>
#include <pthread.h>

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

/**
 * Handle signal interupt by safely shutting down server (ctrl + c)
 * and quitting program. DOES NOT RETURN
 */
void  INThandler(int sig);

/**
 * Handle signal pipe (pipe error: happens when sending data to 
 * client but client is no longer connected). This function being
 * called signifies client disconnected).
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
 * Calls socket_lib_srv_connected callback with client UUID
 * if new client connects.
 * 
 * @Returns the new client's socket fd (file descriptor)
 * 
 * Blocking Function
 */
int handle_conn_request();

/**
 * Given a client's fd, processes any received messages from 
 * client. Calls socket_lib_srv_rx_msg callback if message 
 * received from client.
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
 * This thread is spawned by execute_thread() and calls
 * the process_recv_msg() for received messages or the 
 * discnt_callback/connect_callback if client connected/disconnected
 * from the server. Call socket_client_quit() to kill thread.
 */
void* execute_thread(void* args);

/**
 * Given a client's socket fd, closes the connection to that client 
 * and removes client from client_infos struct and active_fd_set.
 * Calls socket_lib_srv_disconnected callback with client UUID.
 * 
 * Blocking Function
 */
void close_client_conn(int client_fd);

#endif /* PRV_SOCKET_SERVER_H */
