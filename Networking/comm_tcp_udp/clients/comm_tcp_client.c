#include "../comm_tcp.h"

#include <unistd.h> /* Needed for close() */

#define SERVER_ADDR  "192.168.200.1"    /* Address of the server we are conencting to */
#define BUFFER_SZ    1024               /* Size of the send/receive message buffer */
#define IS_CLIENT    1                  /* a value of 0/false indicates we are a server. Used as a parameter to make_socket function. */


/* Check for value parameters and return port number */
uint16_t check_parameters(int argc, char *argv[])
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    uint16_t port;


    /*----------------------------------
    |          CHECK ARGUMENTS          |
    ------------------------------------*/

    /* Verify proper number of arguments and set port number for Server to listen on
        Note that argc = 1 means no arguments*/
    if (argc < 2)
    {
        printf("WARNING, no port provided, defaulting to %d\n", DEFAULT_PORT);

        //No port number provided, use default
        port = DEFAULT_PORT;
    }
    else if (argc > 2)
    {
        fprintf(stderr, "ERROR, too many arguments!\n 0 or 1 arguments expected. Expected port number!\n");
        exit(1);
    }
    else //1 argument
    {
        //Get port number of server from the arguments
        port = atoi(argv[1]);
    }

    return port;
} /* check_parameters() */

int main(int argc, char *argv[])
{
    extern int make_socket(uint16_t port, uint8_t protocol, const char *addr, uint8_t isClient);
    char buffer[BUFFER_SZ];
    uint16_t port = check_parameters(argc, argv);
    const int socket_fd = make_socket(port, DEFAULT_PROTOCOL, SERVER_ADDR, IS_CLIENT);

    bzero(buffer,BUFFER_SZ);

    /* Info print */
    printf("Sending message to server.\n");
    send_data (socket_fd, (char*)"Hello Server!");

    //TODO is this a blocking call?
    receive_data(socket_fd, buffer, BUFFER_SZ);

    /* Info print */
    printf ("Received message \"%s\" from server.\n", buffer);

    /* Close socket */
    close(socket_fd);
    
    return 0;
}
