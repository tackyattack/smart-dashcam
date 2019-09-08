#include "../comm_tcp.h"

#include <unistd.h> /* Needed for close() */

// #define SERVER_ADDR  "192.168.200.1"    /* Address of the server we are conencting to. Note, this can be a IP or hostname */
#define SERVER_ADDR  "raspberrypi"      /* Address of the server we are conencting to. Note, this can be a IP or hostname */

/* Check for value parameters and return port number */
char* check_parameters(int argc, char *argv[])
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    char* port;


    /*----------------------------------
    |          CHECK ARGUMENTS          |
    ------------------------------------*/

    /* Verify proper number of arguments and set port number for Server to listen on
        Note that argc = 1 means no arguments*/
    if (argc < 2)
    {
        printf("WARNING, no port provided, defaulting to %s\n", DEFAULT_PORT);

        //No port number provided, use default
        // strcpy(port, (const char*)DEFAULT_PORT);
        port = (char*)DEFAULT_PORT;
        // port = DEFAULT_PORT;
    }
    else if (argc > 2)
    {
        fprintf(stderr, "ERROR, too many arguments!\n 0 or 1 arguments expected. Expected port number!\n");
        exit(EXIT_FAILURE);
    }
    else //1 argument
    {
        if ( atoi(argv[1]) < 0 || atoi(argv[1]) > 65535 )
        {
            printf("ERROR: invalid port number %s!",argv[1]);
            exit(EXIT_FAILURE);
        }
        //Get port number of server from the arguments
        port = argv[1];
    }

    return port;
} /* check_parameters() */

int main(int argc, char *argv[])
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    char buffer[MAX_MSG_SZ];
    char* port = check_parameters(argc, argv);
    const int socket_fd = make_socket(port, DEFAULT_SOCKET_TYPE, SERVER_ADDR, IS_CLIENT);

    /*----------------------------------
    |          INITIALIZATIONS          |
    ------------------------------------*/
    bzero(buffer,MAX_MSG_SZ);

    /* Info print */
    printf("Sending message to server.\n");
    send_data (socket_fd, (char*)"Hello Server!");

    //TODO is this a blocking call?
    receive_data(socket_fd, buffer, MAX_MSG_SZ);

    /* Info print */
    printf ("Received message \"%s\" from server.\n", buffer);

    /* Close socket */
    close(socket_fd);
    
    return 0;
}
