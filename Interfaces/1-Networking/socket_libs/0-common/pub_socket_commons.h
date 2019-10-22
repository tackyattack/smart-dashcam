#ifndef PUB_SOCK_COMMONS_H
#define PUB_SOCK_COMMONS_H


/*-------------------------------------
|               INCLUDES               |
--------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <assert.h>


/*-------------------------------------
|            PUBLIC DEFINES            |
--------------------------------------*/

#define RETURN_FAILED           (-1)       /* Return value of functions that represents that that function failed to perform its task */
#define RETURN_SUCCESS          (0)        /* Return value of functions that represents that that function succeeded in performing its task */

/* First/Last byte received is msg start/end/cont indicator */
#define MSG_START               (uint8_t)0x0F                       /* Message separator. Used to detect begining of message */
#define MSG_START__SEQUENCE     (uint8_t)0x0C                       /* Signifies that this received message is the next packet in a multipacket message to be received */
#define MSG_END                 (uint8_t)0x0E                       /* Message separator. Used to detect end of message. When used with MSG_START__SEQUENCE, indicates no more multipacket messages to receive. */
#define MSG_END__MORE           (uint8_t)0x0D                       /* Indicates this is a multipacket message and we expect more messages to be received to form the entire multipacket message. When a MSG_START__SEQUENCE is received and a MSG_END is used, there are no more message in sequence */
#define MSG_SZ                  (1)                                 /* Number of bytes a MSG is */
#define MSG_HEADER_SZ           (MSG_SZ + MSG_SZ)                   /* Total size used by the message headers */

/* 2nd byte received is a command byte */
#define COMMAND_PING            (uint8_t)0x0F                       /* If this is received, we have been pinged and request this msg be echoed back */
#define COMMAND_UUID            (uint8_t)0x0E                       /* If received, signals msg data is our UUID */
#define COMMAND_NONE            (uint8_t)0x00                       /* If received, signals msg data is our UUID */
#define COMMAND_SZ              (1)                                 /* Number of bytes a command is */

/* Defaults/Configurations */
#define DEFAULT_PORT            "5555"                              /* Port number to use if none are given */
#define MAX_HOSTNAME_SZ         (255)                               /* Max size/length a hostname can be */
#define MAX_TX_MSG_SZ           (512)                               /* max size of the message that will be sent over socket. NOTE, this should not be used outside of this file */
#define MAX_MSG_SZ              (MAX_TX_MSG_SZ-MSG_HEADER_SZ)       /* max size of the message payload that can be sent */
#define CONNECT_TIMEOUT         (1)                                 /* Set a timeout of 1 second for socket client connect attempt */
#define UUID_SZ                 (36)                                /* UUID is 4 hyphens + 32 digits. This does not include termination character! */
#define TIME_BETWEEN_PINGS      (1)                                 /* Time (in seconds) between pings from the server. The server sends a ping to all clients every TIME_BETWEEN_PINGS seconds */


/*-------------------------------------
|             PUBLIC ENUMS             |
--------------------------------------*/

enum SOCKET_OWNER
{
    SOCKET_OWNER_IS_SERVER  = 0,
    SOCKET_OWNER_IS_CLIENT  = 1
};

/* Define shortnames for Address families. Additional types in socket.h */
enum CONN_TYPES
{
    CONN_TYPE_IPV4          = AF_INET,             /* Use explicitly IPV4 */
    CONN_TYPE_IPV6          = AF_INET6,            /* Use explicitly IPV6 */
    CONN_TYPE_IPV_ANY       = AF_UNSPEC,           /* Use either IPV4 or IPV6, but neither explicitly */
    CONN_TYPE_LOCAL         = PF_LOCAL,            /* Local loopback connection only. Used for things like inter-process communication */
    DEFAULT_CONN_TYPE       = CONN_TYPE_IPV_ANY    /* Default connection type (address family) to use */
};

/* Define shortnames for Protocol families. Additional families found in socket.h */
enum SOCKET_TYPES
{
    SOCKET_TYPE_TCP         = SOCK_STREAM,       /* TCP */
    SOCKET_TYPE_UDP         = SOCK_DGRAM,        /* UDP. Note code isn't setup to support this */
    DEFAULT_SOCKET_TYPE     = SOCKET_TYPE_TCP    /* Default connection type (address family) to use */
};

/* Flags used/returned by socket_receive_data() function */
enum SOCKET_RECEIVE_DATA_FLAGS
{
    RECV_ERROR      = RETURN_FAILED,    /* Indicates an error with message headers */
    RECV_NO_FLAGS   = RETURN_SUCCESS,   /* Indicates no flags/no information */
    RECV_SEQUENCE_CONTINUE,             /* Indicates a message received is part of a sequence of messages and more are expected */
    RECV_SEQUENCE_END                   /* Indicates a message received is part of a sequence of messages and no more are expected */
};

/*-------------------------------------
|     PUBLIC FUNCTION DECLARATIONS     |
--------------------------------------*/

/**
 * Function will print the host IP address and port number of a
 * given addrinfo struct
 */
void socket_print_addrinfo( const struct addrinfo *addr );

/**
 * Given a port number, socket type, an address (this can be IP or hostname),
 * and a value for the type_serv_client parameter (SOCKET_OWNER_IS_SERVER or SOCKET_OWNER_IS_CLIENT), 
 * will setup a socket appropiate for either a server or client and either 
 * bind the server to the socket or connect the socket to the socket server.
 * 
 * If setting up a Server use SOCKET_OWNER_IS_SERVER. Else use SOCKET_OWNER_IS_CLIENT for the 
 * type_serv_client parameter. If setting up a server, addr can/should be NULL.
 * 
 * @Returns the socket or RETURN_FAILED if failed.
 */
int socket_create_socket (char* port, enum SOCKET_TYPES socket_type,  const char *addr, enum SOCKET_OWNER type_serv_client );

/**
 * Given a socket file descriptor and buffer, receives data from socket up to
 * MAX_MSG_SZ.
 * 
 * This is a blocking call.
 * 
 * @Returns an appropiate value from the SOCKET_RECEIVE_DATA_FLAGS enum values.
 * 
 * @Sets received_bytes to a negative number if failed to receive data 
 *       else returns number of bytes received.
 *       Note: Receiving 0 bytes is generally indicative of the transmitter
 *       closing their socket and hence connection to the server.
 */
enum SOCKET_RECEIVE_DATA_FLAGS \
socket_receive_data( const int socket_fd, char* buffer, const size_t buffer_sz, int *received_bytes );

/**
 * Given a open socket_fd, will return the number of bytes available to be read
 * from the socket.
 * 
 * @Returns number of bytes to be read from socket_fd or RETURN_FAILED.
 */
int socket_bytes_to_recv( const int socket_fd );

/**
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
int socket_send_data ( const int socket_fd, const char * data, const uint16_t data_sz );


#endif /* PUB_SOCK_COMMONS_H */
