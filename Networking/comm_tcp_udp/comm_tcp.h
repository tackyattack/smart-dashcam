#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

#ifndef COMM_TCP_H
#define COMM_TCP_H

/* Define shortnames */
#define PROTOCOL_TCP SOCK_STREAM
#define PROTOCOL_UDP SOCK_DGRAM

/* Defaults */
#define DEFAULT_PORT             5555           /* Port number to use if none are given */
#define DEFAULT_PROTOCOL         PROTOCOL_TCP   /* See shortnames above. Set default protocol if none are given */
#define MAX_MSG_SZ               512            /* max size of protocol message */
#define DEFAULT_CONN_TYPE        AF_INET        /* PF_LOCAL for a local only tcp connection, or AF_INET for IP TCP (these are only 2 of the available options) */
#define DEFAULT_ADDR             INADDR_ANY     /* If no address was given, we default to this machines address */


/* Given a port number, return a socket for the machine this program is running on
    using INADDR_ANY as the host address. Pass a value of '0' for the isClient parameter 
    if setting up a Server. Else any value to setup a client.  */
int make_socket(uint16_t port, uint8_t protocol, const char *addr, uint8_t isCLient)
{
    int socket_fd;
    struct sockaddr_in name;

    /* Create the socket. */
    socket_fd = socket(PF_INET, protocol, 0);
    if (socket_fd < 0)
    {
        fprintf (stderr, "errno = %d ", errno);
        perror("socket");
        exit(EXIT_FAILURE);
    }

    /* Set the socket info. */
    name.sin_family = DEFAULT_CONN_TYPE;    //Connection type
    name.sin_port = htons(port);            //Port conn operates on

    //Set the address. If NULL, use the DEFAULT_ADDR, else attempt to use the address passed to us in addr.
    if (addr != NULL)
    {
        if (inet_pton(AF_INET, addr, &name.sin_addr) <= 0)
        {
            fprintf (stderr, "errno = %d ", errno);
            perror("ERROR: host/client address not valid");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        name.sin_addr.s_addr = htonl(DEFAULT_ADDR);
    }
    
    #if 0
    int opt = 1;
    // Forcefully attaching socket to the port 8080 
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
                                                  &opt, sizeof(opt))) 
    {
        fprintf (stderr, "errno = %d ", errno);
        perror("setsockopt"); 
        exit(EXIT_FAILURE); 
    } 
    #endif 

    if (isCLient == 0) /* We are a server wishing to bind to a port/address */
    {
        /* Bind the socket */
        if (bind(socket_fd, (struct sockaddr *)&name, sizeof(name)) < 0)
        {
            fprintf (stderr, "errno = %d ", errno);
            perror("bind");
            exit(EXIT_FAILURE);
        }
    }
    else /* We are a client and wish to connect to a server */
    {
         if (connect(socket_fd, (struct sockaddr *)&name, sizeof(name)) < 0) 
        { 
            printf("\nConnection Failed \n"); 
            return -1; 
        } 
    }
    


    return socket_fd;
} /* make_socket() */

/* Given a socket file descriptor, reads data and returns the number of bytes read. This is a blocking call. */
int receive_data(int socket_fd, char* buffer, const size_t buffer_sz)
{
    //TODO see if data received is broken into MAX_MSG_SZ chucks or is received in its entirety
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    char msg_buffer[MAX_MSG_SZ];
    int bytes_read;

    /*----------------------------------
    |             READ DATA             |
    ------------------------------------*/
    //TODO see what happens if buffer is too small for received data
    bytes_read = read(socket_fd, buffer, buffer_sz);

    /* Check for error/EOF */    
    if (bytes_read < 0)
    {
        /* Read error. */
        fprintf (stderr, "errno = %d ", errno);
        perror("read");
        exit(EXIT_FAILURE);
    }
    else if (bytes_read == 0)
    {
        /* End-of-file. This signifies to close the connection */
        return 0;
    }

    return bytes_read;

} /* receive_data() */


/* Send data. Returns number of bytes send or -1 if there's an error. Sends a string up to 2^16 in size */
int send_data ( const int socket_fd, const char * data )
{
    char buffer[MAX_MSG_SZ];
    int sent_bytes;
    uint16_t all_sent_bytes;
    uint16_t send_str_len;
    uint16_t all_bytes_to_send;
    uint16_t bytes_to_send;
    uint8_t n_msgs;
    uint8_t send_flags;

    send_flags = 0;
    all_sent_bytes = 0;
    sent_bytes = 0;
    n_msgs = 0;

    send_str_len = strlen ( data );
    all_bytes_to_send = send_str_len;
    n_msgs = ( (send_str_len + 1)/MAX_MSG_SZ ); /* +1 for the termination char */

    if ( (send_str_len + 1) % MAX_MSG_SZ != 0 )
    {
        /* There are additional bytes leftover, add one message */
        n_msgs+=1;
    }

    for (uint8_t i = 0; i < n_msgs; i++)
    {
        bzero(buffer, MAX_MSG_SZ);

        /* Determine number of bytes to send */
        if (all_bytes_to_send > MAX_MSG_SZ)
        {
            bytes_to_send = MAX_MSG_SZ;
        }
        else
        {
            bytes_to_send = all_bytes_to_send;
        }

        /* Verify there are bytes to send */
        if ( bytes_to_send == 0 )
        {
            return 0;
        }

        /* Get substring to be sent */
        memcpy( buffer, &data[all_sent_bytes], bytes_to_send);
        //TODO does this need null termination?
        
        /* if this is the last message to send */ /* For send flags, see https://linux.die.net/man/2/send */
        if (i != n_msgs-1 )
        {
            send_flags = 0;
        }
        else /*else this is not the last message to send */
        {
            send_flags = send_flags | MSG_MORE; /* signifies there is more data to send. */
        }
        
        /* Send the message */
        sent_bytes = send ( socket_fd, buffer, bytes_to_send, send_flags );
        
        /* Verify bytes were sent */
        if ( sent_bytes < 1 )
        {
            perror("ERROR sending bytes");
            exit(EXIT_FAILURE);
            return -1;
        }

        all_sent_bytes += sent_bytes;
        all_bytes_to_send -= sent_bytes;
        
    } /* For loop */

    /* Verify all bytes were sent */
    if ( all_sent_bytes != send_str_len )
    {
        perror("ERROR sending all bytes");
        exit(EXIT_FAILURE);
        return -1;
    }

    /* Info print */
    printf ("Sent data: \"%s\" \n", data);
    
    return all_sent_bytes;
} /* send_data */

// /* Fill in a sockaddr_in structure given a host name string and a port number */
// void init_sockaddr(struct sockaddr_in *name, const char *hostname, uint16_t port)
// {
//     struct hostent *hostinfo;

//     name->sin_family = AF_INET;
//     name->sin_port = htons(port);
//     hostinfo = gethostbyname(hostname);
//     if (hostinfo == NULL)
//     {
//         fprintf(stderr, "Unknown host %s.\n", hostname);
//         exit(EXIT_FAILURE);
//     }
//     name->sin_addr = *(struct in_addr *)hostinfo->h_addr;
// } /* init_sockaddr */

#endif