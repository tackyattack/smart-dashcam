#ifndef PRV_SOCK_COMMONS_H
#define PRV_SOCK_COMMONS_H


/*-------------------------------------
|               INCLUDES               |
--------------------------------------*/
#include <pthread.h>

/*-------------------------------------
|            PRIVATE DEFINES           |
--------------------------------------*/


/*-------------------------------------
|    PRIVATE FUNCTION DECLARATIONS     |
--------------------------------------*/

/** 
 * Modifies ip string to be IP address of hostname. IP must be a 
 * char string of length MAX_HOSTNAME_SZ. Return -1 if hostname 
 * not found. Note that hostname can be the hostname of ip of a 
 * machine. 
 * 
 * @Returns 0 is hostname found. returns -1 if failed to find 
 *          hostname and leaves ip string untouched.
 */
int hostname_to_ip(const char * hostname , char* ip);

/**
 * Connect to TCP server with a timeout parameter in seconds given
 * a valid socket, sockaddr struct, and addrlen. Called by client_connect()
 * 
 * This is a Blocking function
 * 
 * @Returns EXIT_FAILURE or EXIT_SUCCESS
 */
int connect_timeout(int sock, struct sockaddr *addr, socklen_t addrlen, uint32_t timeout);

/** 
 * This function binds a server dictated by a given addrinfo struct
 * to a socket
 * 
 * @Returns the server_fd if successful or -1 if failed. 
 */
int server_bind(struct addrinfo *address_info_set);

/**
 * Connects to a socket server given a addinfo struct containing the server's
 * info.
 * 
 * @Returns the client_fd or -1 if failed to connect.
 */
int client_connect(struct addrinfo *address_info_set);

/**
 * Removes message headers from the buffer string. Called by socket_receive_data().
 * 
 * @Returns an appropiate value from the SOCKET_RECEIVE_DATA_FLAGS enum values.
 * 
 * @Sets the given data_msg_sz parameter to the length of the payload 
 * (which is message data without headers) or RETURN_FAILED if data received is 
 * invalid (invalid message headers).
 */
enum SOCKET_RECEIVE_DATA_FLAGS \
remove_msg_header(char *buffer, int buffer_sz, int *data_msg_sz);

#endif /* PRV_SOCK_COMMONS_H */
