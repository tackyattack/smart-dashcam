#ifndef PRV_SOCK_COMMONS_H
#define PRV_SOCK_COMMONS_H


/*-------------------------------------
|               INCLUDES               |
--------------------------------------*/
#include <pthread.h>

/*-------------------------------------
|           PRIVATE STRUCTS            |
--------------------------------------*/

/** This struct is used for sending/receiving packets over the socket. It contains
 * information pertaining to the data received, how much to expect, and more.
 */
struct MSG_HEADER
{
    char     msg_type;      /* Type specified by MSG_ prefix in #defines above. Currently not used */
    uint16_t msg_num_bytes; /* Number of bytes long this message is. Max value is MAX_MSG_SZ. This dictates the data_sz of data given to send() functions */
    uint16_t crc16_checksum;/* Checksum for header */
} __attribute__((packed));  /* Attribute tells compiler to not use padding in the struct. This is to prevent any additional bytes being added to the struct because
                                this struct comprises the msg header to be sent via sockets, and as such, don't want any additional bytes. See https://www.geeksforgeeks.org/how-to-avoid-structure-padding-in-c/ */


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
 * Given a data array and its length, generates a 16 bit CRC. Asserts
 * if given parameters are NULL or 0.
 * 
 * @Returns: 16-bit CRC
 */
uint16_t crc16(const unsigned char* data_p, uint16_t length);

#endif /* PRV_SOCK_COMMONS_H */
