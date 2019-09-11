#include "../comm_tcp.h"

#include <unistd.h> /* Needed for close() */

#define SERVER_ADDR  "192.168.200.1"            /* Address of the server we are conencting to. Note, this can be a IP or hostname */
// #define SERVER_ADDR  "raspberrypi"           /* Address of the server we are conencting to. Note, this can be a IP or hostname */
#define TIME_BETWEEN_CONNECTION_ATTEMPTS (1)    /* Time in seconds between attempts to connect to server if we fail to connect */

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

void execute_client(const int *socket_fd)
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    char buffer[MAX_MSG_SZ];

    /*----------------------------------
    |          INITIALIZATIONS          |
    ------------------------------------*/
    bzero(buffer,MAX_MSG_SZ);

    /*----------------------------------
    |           INFINITE LOOP           |
    ------------------------------------*/
    while(1)
    {
        receive_data(*socket_fd, buffer, MAX_MSG_SZ);

        /* Info print TODO remove this */
        printf ("Received message \"%s\" from server.\n", buffer);

    } /* while(1) */
} /* execute_client */

int main(int argc, char *argv[])
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    char* port = check_parameters(argc, argv);
    int socket_fd; 
    
    /* Loop until successful connection with server */
    do
    {
        /* Info print */
        printf ("\nAttempt to open socket to server....\n");

        sleep(TIME_BETWEEN_CONNECTION_ATTEMPTS);
        socket_fd = make_socket(port, DEFAULT_SOCKET_TYPE, SERVER_ADDR, IS_CLIENT);
    } while (socket_fd < 0);
    
    /* Info print */
    printf ("Successfully opened socket to server \"%s\" on port %s.\n", SERVER_ADDR, port);
    
    /* Main loop for client to receive/process data */
    execute_client(&socket_fd);

    /* Close socket */
    close(socket_fd);
    
    return 0;
}
