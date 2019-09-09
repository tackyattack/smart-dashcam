/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include "comm_server.h"

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    int socket_fd, newsocket_fd;
    uint16_t portNum;
    char buffer[BUFFER_SZ];
    struct sockaddr_in serv_addr, client_addr;
    socklen_t client_addr_sz;
    int n;

    client_addr_sz = sizeof(client_addr);

    // Verify proper number of arguments and set port number for Server to listen on
    // Note that argc = 1 means no arguments
    if (argc < 2)
    {
        printf("WARNING, no port provided, defaulting to %d\n", DEFAULT_PORT_NUM);

        //No port number provided, use default
        portNum = DEFAULT_PORT_NUM;
    }
    else if (argc > 2)
    {
        fprintf(stderr, "ERROR, too many arguments!\n 0 or 1 arguments expected. Expected port number\n");
        exit(1);
    }
    else //1 argument
    {
        //Get port number of server from the arguments
        portNum = atoi(argv[1]);
    }

    // Open socket file descriptor
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_fd < 0)
    {
        error("ERROR opening socket");
    }

    //Clear server address struct and set values
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;         //AF_INET signifies this is an IP connection
    serv_addr.sin_addr.s_addr = INADDR_ANY; //INADDR_ANY is this machines ip address
    serv_addr.sin_port = htons(portNum);    //Converts a port number in host byte order to a port number in network byte order

    // Attempt to bind to socket
    if (bind(socket_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        error("ERROR on binding");
    }

    // Allows process to listen for incoming connection requests with backlog of up to 5
    listen(socket_fd, 5);

    // accept() is a Blocking call. 
    // Wake up the process when a new connection has been established and get the file descriptor
    newsocket_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &client_addr_sz);

    // Check that file descriptor is valid
    if (newsocket_fd < 0)
    {
        error("ERROR on accept");
    }

    // Clear buffer
    bzero(buffer, BUFFER_SZ);

    // Blocking call read(). Read data on the buffer
    n = read(newsocket_fd, buffer, 255);
    
    // Check if error receiving
    if (n < 0)
    {
        error("ERROR reading from socket");
    }

    #if DEBUG
        printf("Here is the message: %s\n", buffer);
    #endif

    n = write(newsocket_fd, "I got your message", 18);

    if (n < 0)
    {
        error("ERROR writing to socket");
    }

    // Close client sockets
    close(newsocket_fd);

    // Close server socket
    close(socket_fd);
    
    return 0;
}
