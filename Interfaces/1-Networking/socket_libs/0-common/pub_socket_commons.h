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

/* Message header used internally to keep track of sending/receiving data */
#define MSG_HEADER_SZ           (sizeof(struct MSG_HEADER))         /* Total size used by the message headers */

/* Defaults/Configurations */
#define DEFAULT_PORT            "5555"                              /* Port number to use if none are given */
#define MAX_HOSTNAME_SZ         (255)                               /* Max size/length a hostname can be */
#define MAX_MSG_SZ              (65536)                             /* max size of the message that can be sent over sockets. */
#define CONNECT_TIMEOUT         (1)                                 /* Set a timeout of 1 second for socket client connect attempt */
#define UUID_SZ                 (37)                                /* UUID is 4 hyphens + 32 digits. This does include termination character! */
#define TIME_BETWEEN_PINGS      (1)                                 /* Time (in seconds) between pings from the server. The server sends a ping to all clients every TIME_BETWEEN_PINGS seconds */

/* 1st byte after msg header is a command byte. used by server/client code */
#define COMMAND_PING            (uint8_t)0x0F                       /* If this is received, we have been pinged and request this msg be echoed back */
#define COMMAND_UUID            (uint8_t)0x0E                       /* If received, signals msg data is our UUID */
#define COMMAND_NONE            (uint8_t)0x00                       /* Signifies msg is data message and not a library level command (such as ping) */
#define COMMAND_SZ              (1)                                 /* Number of bytes a command is */

/* Normally used function return values */
#define RETURN_FAILED           (-1)       /* Return value of functions that represents that that function failed to perform its task */
#define RETURN_SUCCESS          (0)        /* Return value of functions that represents that that function succeeded in performing its task */


/*-------------------------------------
|            PUBLIC STRUCTS            |
--------------------------------------*/

/**
 * This struct is returned when receiving messages/calling 
 * struct socket_receive_data() when given as a double pointer parameter.
 * Each struct represents one data message received and contains the data arry,
 * the array size, the receive flag for that message (if any), and a pointer to
 * the next msg struct (NULL if none).
 */
struct socket_msg_struct
{
    char* data;                     /* Data array of message */
    uint16_t data_sz;               /* The number of bytes in data message array */
    uint8_t recv_flag;              /* The receive flag for this data from the SOCKET_TX_RX_FLAGS enum */
    struct socket_msg_struct* next; /* Pointer to next msg if applicable. This is because it is possible to receive multiple messages at the same time */
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

/* Flags used/returned by by various functions but specifically send and recv */
enum SOCKET_TX_RX_FLAGS
{
    FLAG_DISCONNECT         = -2,               /* Indicates a disconnect/connection loss */
    FLAG_HEADER_ERROR       = RETURN_FAILED,    /* Indicates an error with message headers */
    FLAG_DATA_ERROR         = RETURN_FAILED,    /* Indicates an error with message data */
    FLAG_SUCCESS            = RETURN_SUCCESS     /* Indicates no problems */
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
 * Given a socket file descriptor and pointer to a pointer to a socket_msg_struct
 * struct used to hold msg information and the message itself. Receives data from 
 * socket up to MAX_MSG_SZ. Note, this function will block until ALL of a message
 * has been received.
 * 
 * The msg parameter should be a pointer to a NULL pointer of type struct socket_msg_struct
 * ie ( struct socket_msg_struct* msg; msg = NULL; socket_receive_data(fd, &msg); )
 * 
 * This is a blocking call.
 * 
 * @Returns an appropiate value from the SOCKET_TX_RX_FLAGS enum values. 
 *          If FLAG_DISCONNECT is returned, the socket sender has declared the socket
 *          to be closed and as such, the socket that received a FLAG_DISCONNECT
 *          should be closed.
 * 
 * @Sets sets msg NULL if failed to receive. Else, *msg is a valid socket_msg_struct
 *       that contains the received message(s) and are linked together via linked list.
 */
enum SOCKET_TX_RX_FLAGS \
socket_receive_data( const int socket_fd, struct socket_msg_struct** msg );

/**
 * Given a char* data array up to 2^16 (MAX_MSG_SZ) in size and a socket_fd,
 * will send data array over socket. This function will block until all data
 * has been sent/entered into the system buffer. It will block as needed.
 * 
 * Note that data_sz should include the termination character if applicable.
 * 
 * Note that calls to this are thread safe.
 * 
 * @Returns number of bytes sent (excluding msg headers) which should be == data_sz, 
 *          or a flag from the SOCKET_TX_RX_FLAGS enum if failed
 */
int socket_send_data ( const int socket_fd, const char * data, const uint16_t data_sz );

#endif /* PUB_SOCK_COMMONS_H */
