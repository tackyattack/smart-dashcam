#include <stdio.h>
#include <stdlib.h>

#include <errno.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "../comm_tcp.h"

#define MAX_PENDING_CONNECTIONS  5      /* 5 is a standard value for the max backlogged connection requests */
#define SOCKET_ADDR             NULL    /* Set this value to a string of the address (such as "192.168.0.10"), or set to NULL (0) to use this machines address */
#define BUFFER_SZ               1024    /* Size of the buffer we use to send/receive data */

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

/* Initialize the server and return the server's server_socket_fd */
int init_server(char* port)
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    extern int make_socket(char* port, uint8_t protocol, 
                            const char* addr, uint8_t isServer);      /* external function in comm_tcp.h to create socket for server */
    int server_socket_fd;                              /* generic socket variable */
       
    /* Info print */
    printf("Creating server on port %s\n", port);

    /*----------------------------------
    |       CREATE SERVER SOCKET        |
    ------------------------------------*/
    server_socket_fd = make_socket(port, DEFAULT_SOCKET_TYPE, (const char*)SOCKET_ADDR, IS_SERVER);
    
    /*----------------------------------
    |   BLOCK UNTIL FIRST CONNECTION    |
    ------------------------------------*/
    if ( listen(server_socket_fd, MAX_PENDING_CONNECTIONS) < 0 )
    {
        fprintf (stderr, "errno = %d ", errno);
        perror("listen");
        exit(EXIT_FAILURE);
    }

    /* Info print */
    printf("Created server on port %s\n", port);

    return server_socket_fd;
}

/* Handles accepting an incoming connection 
   -returns the new clients file descriptor */
int handle_conn_request(int server_socket_fd)
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    struct sockaddr_in new_client_info;         /* socketaddr_in struct containing connecting client info */
    socklen_t new_client_sz;                    /* Size of the new clients socketaddr_in info struct */    
    int new_client_fd;                          /* File descriptor for new client */
    new_client_sz = sizeof(new_client_info);    /* Size of client info struct */

    /* Accept connection request on original server_socket_fd. */
    new_client_fd = accept(server_socket_fd, (struct sockaddr *)&new_client_info, &new_client_sz);

    /* Verify connection was successful */
    if (new_client_fd < 0)
    {
        fprintf (stderr, "errno = %d ", errno);
        perror("accept");
        exit(EXIT_FAILURE);
    }

    /* Info print */
    fprintf(stderr, "Server: connect from host %s, port %u.\n", inet_ntoa(new_client_info.sin_addr), ntohs(new_client_info.sin_port));

    return new_client_fd;
} /* handle_conn_request() */

void close_conn(int client_fd, const fd_set* active_fd_set)
{
    /* Info print */
    printf("Closing conenction to client.");
    close(client_fd);
    FD_CLR(client_fd, (fd_set*)active_fd_set);

} /* close_conn() */

void execute_server(int server_socket_fd)
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    int i;                                      /* generic i used in for loop */
    fd_set active_fd_set, read_fd_set;          /* generic file descriptor set for sockets */
    int new_client_fd;                          /* File descriptor for connecting clients */
    int n_received_msgs;                        /* Number of bytes received from a client */
    char buffer[BUFFER_SZ+1];                   /* Buffer for send/receive of data. Extra byte for termination */
    
    
    /*----------------------------------
    |            INITIALIZE             |
    ------------------------------------*/

    /* Initialize the set of active sockets. */
    FD_ZERO(&active_fd_set);
    FD_SET(server_socket_fd, &active_fd_set);
    bzero(buffer,BUFFER_SZ);
    n_received_msgs = 0;
    
    
    /*----------------------------------
    |           INFINITE LOOP           |
    ------------------------------------*/
    while (1)
    {
        /* Block until input arrives on one or more active sockets. */
        read_fd_set = active_fd_set;
        if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0)
        {
            fprintf (stderr, "errno = %d ", errno);
            perror("select");
            exit(EXIT_FAILURE);
        }

        /* Service all the sockets with input pending. */
        for (i = 0; i < FD_SETSIZE; ++i)
        {
            /* Check if clients fd is set */
            if (FD_ISSET(i, &read_fd_set))
            {
                /* Check if the server's fd is set, 
                if so, handle conenction request */
                if (i == server_socket_fd)
                {
                    /*----------------------------------
                    |          SETUP NEW CLIENT         |
                    ------------------------------------*/
                    new_client_fd = handle_conn_request(server_socket_fd);
                    FD_SET(new_client_fd, &active_fd_set);
                }
                else /* A client has sent us a message */
                {
                    bzero(buffer,BUFFER_SZ);
                    /* Receive data from client. Returns number of bytes received. */
                    n_received_msgs = receive_data( i, buffer, BUFFER_SZ );
                    
                    //TODO buffer may not be null terminated in all cases
                    /* Info print. Data read. */    
                    fprintf(stderr, "Server: received message: \"%s\"\n", buffer);

                    //REVIEW TODO Remove this line. It is for testing
                    /* Echo message back to client */
                    send_data(i, buffer);

                    /* Check if received connection close request. */
                    //TODO FIXME  This method of detecting disconnect doesn't work
                    //Implement another method such as tcp keepalive
                    if (n_received_msgs <= 0)
                    {
                        close_conn(i, &active_fd_set);
                    }
                } /* else */

            } /* if (FD_ISSET) */

        } /* For loop */

    } /* while (1) */

} /* execute_server() */

int main(int argc, char *argv[])
{
    char* port;                              /* Port number for socket server */
    int server_socket_fd;                              /* generic socket variable */
    
    /* Check and parse input parameters */
    port = check_parameters(argc, argv);

    /* Initialize/create the server */
    server_socket_fd = init_server(port);

    /* Run the server. 
        Accept connection requests, and receive messages. 
        Run forever. */
    execute_server(server_socket_fd);

    /* If we ever return from execute_server, there was a serious error */
    return 1;
} /* main() */
