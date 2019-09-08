/*
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define PORT 5555
#define MAXMSG 512

int make_socket(uint16_t port)
{
    int sock;
    struct sockaddr_in name;

     //Create the socket. 
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Give the socket a name. 
    name.sin_family = AF_INET;
    name.sin_port = htons(port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sock, (struct sockaddr *)&name, sizeof(name)) < 0)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    return sock;
}

int read_from_client(int filedes)
{
    char buffer[MAXMSG];
    int nbytes;

    nbytes = read(filedes, buffer, MAXMSG);
    if (nbytes < 0)
    {
        // Read error. 
        perror("read");
        exit(EXIT_FAILURE);
    }
    else if (nbytes == 0)
        // End-of-file. 
        return -1;
    else
    {
        // Data read.
        fprintf(stderr, "Server: got message: `%s'\n", buffer);
        return 0;
    }
}

int main(void)
{
    extern int make_socket(uint16_t port);
    int sock;
    fd_set active_fd_set, read_fd_set;
    int i;
    struct sockaddr_in clientname;
    socklen_t size;

    // Create the socket and set it up to accept connections.
    sock = make_socket(PORT);
    if (listen(sock, 1) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    /* Initialize the set of active sockets.
    FD_ZERO(&active_fd_set);
    FD_SET(sock, &active_fd_set);

    while (1)
    {
        // Block until input arrives on one or more active sockets.
        read_fd_set = active_fd_set;
        if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0)
        {
            perror("select");
            exit(EXIT_FAILURE);
        }

        /* Service all the sockets with input pending.
        for (i = 0; i < FD_SETSIZE; ++i)
            if (FD_ISSET(i, &read_fd_set))
            {
                if (i == sock)
                {
                    // Connection request on original socket.
                    int new;
                    size = sizeof(clientname);
                    new = accept(sock, (struct sockaddr *)&clientname, &size);
                    if (new < 0)
                    {
                        perror("accept");
                        exit(EXIT_FAILURE);
                    }
                    fprintf(stderr,
                            "Server: connect from host %s, port %hd.\n",
                            inet_ntoa(clientname.sin_addr), ntohs(clientname.sin_port));
                    FD_SET(new, &active_fd_set);
                }
                else
                {
                    // Data arriving on an already-connected socket.
                    if (read_from_client(i) < 0)
                    {
                        close(i);
                        FD_CLR(i, &active_fd_set);
                    }
                }
            }
    }
}

*/



#include "comm_tcp.h"

int accept_connection(int server_socket)
{
    int client_socket;                          /* Socket descriptor for client */
    struct sockaddr_in client_address;          /* Client address */
    unsigned int client_length;                 /* Length of client address data structure */

    /* Set the size of the in-out parameter */
    client_length = sizeof(client_address);

    /* Wait for a client to connect */
    if ((client_socket = accept(server_socket, (struct sockaddr *) &client_address, &client_length)) < 0) {
        printf("accept() failed");
    }

    /* client_socket is connected to a client! */
    printf("Handling client %s\n", inet_ntoa(client_address.sin_addr));

    return client_socket;
}

void handle_client (int client_socket)
{
    char buffer [BUFFSIZE];           /* Buffer for incoming data */
    int msg_size;                     /* Size of received message  */
    int bytes, all_bytes;

    do {
        alarm (60);
        msg_size = read (client_socket, buffer, BUFFSIZE);
        alarm (0);

        if ( msg_size <= 0 ) {
            printf ( " %i ", msg_size );
            printf ( "End of data\n" );
        }
    } while ( msg_size > 0 );
    printf ("Data received: %s", buffer);
    bytes = 0;
}

int main()
{
    int clnt_sock;
    int sock = make_socket(ADRESS_PORT, SERVER_SOCKET, "none");
    clnt_sock = accept_connection (sock);
    handle_client(clnt_sock);
    close_socket(sock);
    return 0;
}