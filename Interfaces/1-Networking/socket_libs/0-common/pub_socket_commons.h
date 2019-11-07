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
|         FORWARD DECLARATIONS         |
--------------------------------------*/

struct MSG_HEADER;


/*-------------------------------------
|            PUBLIC DEFINES            |
--------------------------------------*/

#define RETURN_FAILED           (-1)       /* Return value of functions that represents that that function failed to perform its task */
#define RETURN_SUCCESS          (0)        /* Return value of functions that represents that that function succeeded in performing its task */

/* First byte in msg_header struct is msg type indicator indicator */
#define MSG_TYPE_INDEPENDENT    (uint8_t)0x0F                       /* Indicates that the message received/sent is self contained (the data payload is less than MAX_DATA_MSG_SZ) */
#define MSG_TYPE_SEQUENCE       (uint8_t)0x0E                       /* Signifies that this received message is the the first or next packet in a multipacket message being received */
#define MSG_TYPE_END_SEQUENCE   (uint8_t)0x0D                       /* Signifies that this received message is the last packet in a multipacket message sequence being received */
#define MSG_HEADER_SZ           (sizeof(struct MSG_HEADER))         /* Total size used by the message headers */

/* 1st byte after msg header is a command byte */
#define COMMAND_PING            (uint8_t)0x0F                       /* If this is received, we have been pinged and request this msg be echoed back */
#define COMMAND_UUID            (uint8_t)0x0E                       /* If received, signals msg data is our UUID */
#define COMMAND_NONE            (uint8_t)0x00                       /* Signifies msg is data message and not a library level command (such as ping) */
#define COMMAND_SZ              (1)                                 /* Number of bytes a command is */

/* Defaults/Configurations */
#define DEFAULT_PORT            "5555"                              /* Port number to use if none are given */
#define MAX_HOSTNAME_SZ         (255)                               /* Max size/length a hostname can be */
#define MAX_MSG_SZ              (512)                               /* max size of the message that will be sent over socket. NOTE, this should not be used outside of this file */
#define MAX_DATA_MSG_SZ         (MAX_MSG_SZ-MSG_HEADER_SZ)          /* max size of the message payload that can be sent. Max size of any data message passed to socket_send_data() */
#define CONNECT_TIMEOUT         (1)                                 /* Set a timeout of 1 second for socket client connect attempt */
#define UUID_SZ                 (37)                                /* UUID is 4 hyphens + 32 digits. This does include termination character! */
#define TIME_BETWEEN_PINGS      (1)                                 /* Time (in seconds) between pings from the server. The server sends a ping to all clients every TIME_BETWEEN_PINGS seconds */


/*-------------------------------------
|            PUBLIC STRUCTS            |
--------------------------------------*/

/** This struct is used for sending/receiving packets over the socket. It contains
 * information pertaining to the data received, how much to expect, and more.
 */
struct MSG_HEADER
{
    char     msg_type;      /* Type specified by MSG_ prefix in #defines above */
    uint16_t msg_num_bytes; /* Number of bytes long this message is. Max value is MAX_MSG_SZ. This dictates the data_sz of data given to send() functions */
} __attribute__((packed));  /* Attribute tells compiler to not use padding in the struct. This is to prevent any additional bytes being added to the struct because
                                this struct comprises the msg header to be sent via sockets, and as such, don't want any additional bytes. See https://www.geeksforgeeks.org/how-to-avoid-structure-padding-in-c/ */

/** This struct is returned when receiving messages/calling 
* struct socket_receive_data() when given as a pointer parameter
*/
struct RECV_MSGS
{
    char** data_arrys;      /* An array of data messages received. This is because it is possible to receive multiple messages at the same time */
    uint16_t** data_arry_sz;/* The number of bytes in each data message array */
    uint n_arrys;           /* Number of arrays of data messages (number of elements in arrys above) */
};


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
    CONN_TYPE_IPV4          = AF_INET,          /* Use explicitly IPV4 */
    CONN_TYPE_IPV6          = AF_INET6,         /* Use explicitly IPV6 */
    CONN_TYPE_IPV_ANY       = AF_UNSPEC,        /* Use either IPV4 or IPV6, but neither explicitly */
    CONN_TYPE_LOCAL         = PF_LOCAL,         /* Local loopback connection only. Used for things like inter-process communication */
    DEFAULT_CONN_TYPE       = CONN_TYPE_IPV_ANY /* Default connection type (address family) to use */
};

/* Define shortnames for Protocol families. Additional families found in socket.h */
enum SOCKET_TYPES
{
    SOCKET_TYPE_TCP         = SOCK_STREAM,      /* TCP */
    SOCKET_TYPE_UDP         = SOCK_DGRAM,       /* UDP. Note code isn't setup to support this */
    DEFAULT_SOCKET_TYPE     = SOCKET_TYPE_TCP   /* Default connection type (address family) to use */
};

/* Flags used/returned by socket_receive_data() function */
enum SOCKET_RECEIVE_DATA_FLAGS
{
    RECV_DISCONNECT         = -2,               /* Indicates a disconnect/connection loss */
    RECV_HEADER_ERROR       = RETURN_FAILED,    /* Indicates an error with message headers */
    RECV_DATA_ERROR         = RETURN_FAILED,    /* Indicates an error with message data */
    RECV_NO_FLAGS           = RETURN_SUCCESS,   /* Indicates no flags/no information */
    RECV_SEQUENCE_CONTINUE,                     /* Indicates a message received is part of a sequence of messages and more are expected */
    RECV_SEQUENCE_END                           /* Indicates a message received is part of a sequence of messages and no more are expected */
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
int socket_create_socket (char* port, enum SOCKET_TYPES socket_type, const char *addr, enum SOCKET_OWNER type_serv_client );

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
 * Given a char* data array up to 2^16 in size and a socket_fd,
 * will send data array over socket.
 * 
 * Note that data_sz should include the termination character if applicable.
 * 
 * Note that calls to this are thread safe.
 * 
 * @Returns number of bytes sent or RETURN_FAILED or 0 if there's an error.
 */
int socket_send_data ( const int socket_fd, const char * data, const uint16_t data_sz );


#endif /* PUB_SOCK_COMMONS_H */
