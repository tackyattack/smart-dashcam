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

#ifndef COMM_TCP_H
#define COMM_TCP_H

/* Define shortnames */
#define SOCKET_TYPE_TCP SOCK_STREAM
#define SOCKET_TYPE_UDP SOCK_DGRAM

#define IS_SERVER (0)
#define IS_CLIENT (1)

/* Defaults */
#define DEFAULT_CONN_TYPE        AF_UNSPEC        /* PF_LOCAL for a local only connection, or AF_INET for IP connection or AF_INET6 for ipv6 or AF_UNSPEC for ipv4 or piv6 */
#define DEFAULT_SOCKET_TYPE      SOCKET_TYPE_TCP  /* */
#define DEFAULT_PORT             "5555"           /* Port number to use if none are given */
#define MAX_HOSTNAME_SZ          255              /* Max size/length a hostname can be */
#define MAX_MSG_SZ               512              /* max size of protocol message */
#define CONNECT_TIMEOUT          1                /* Set a timeout of 1 second for socket client connect attempt */

/* Modifies ip string to be IP address of hostname found.
    ip must be a char string of length MAX_HOSTNAME_SZ.
    Return -1 if hostname not found */
int hostname_to_ip(const char * hostname , char* ip)
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
	struct hostent *he;
	struct in_addr **addr_list;
	int i;
    
    /*----------------------------------
    |          INITIALIZATIONS          |
    ------------------------------------*/
    bzero(ip, MAX_HOSTNAME_SZ);
		

    /*----------------------------------
    |       GET IP FROM HOSTNAME        |
    ------------------------------------*/
	he = gethostbyname( hostname );
    if ( he == NULL) 
	{
		herror("gethostbyname");
		return -1;
	}

	addr_list = (struct in_addr **) he->h_addr_list;
	
    /*----------------------------------
    |            RETURN IP              |
    ------------------------------------*/
	for(i = 0; addr_list[i] != NULL; i++) 
	{
		//Return the first one;
		strcpy(ip , inet_ntoa(*addr_list[i]) );
		printf("Found IP address \"%s\" for hostname \"%s\"\n",ip,hostname);

        /* Return successfully */
		return 0;
	}

	return -1;
} /* hostname_to_ip() */

/* print_addrinfo will print the host IP address and port number 
    of a passed addrinfo struct */
void print_addrinfo(const struct addrinfo *addr)
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];


    /*----------------------------------
    |        PRINT IF ADDR VALID        |
    ------------------------------------*/
    if (getnameinfo(addr->ai_addr, addr->ai_addrlen, hbuf, sizeof(hbuf), sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV) == 0)
    {
        printf("host=%s, serv=%s\n", hbuf, sbuf);
    }
} /* print_addrinfo() */

int connect_timeout(int sock, struct sockaddr *addr, socklen_t addrlen, uint32_t timeout)
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    int res;
    long flags;
    fd_set myset;
    struct timeval tv;
    int valopt;
    socklen_t lon;

    /*----------------------------------
    |       SET NOT-BLOCKING MODE       |
    ------------------------------------*/
    if ((flags = fcntl(sock, F_GETFL, NULL)) < 0)
    {
        fprintf(stderr, "Error fcntl(..., F_GETFL) (%s)\n", strerror(errno));
        exit(0);
    }

    flags |= O_NONBLOCK;

    if (fcntl(sock, F_SETFL, flags) < 0)
    {
        fprintf(stderr, "Error fcntl(..., F_SETFL) (%s)\n", strerror(errno));
        exit(0);
    }

    /*----------------------------------
    |  ATTEMPT TO CONNECT WITH TIMEOUT  |
    ------------------------------------*/
    res = connect(sock, addr, addrlen);
    if (res < 0)
    {
        if (errno == EINPROGRESS)
        {
            fprintf(stderr, "EINPROGRESS in connect() - selecting\n");
            do
            {
                tv.tv_sec = timeout;
                tv.tv_usec = 0;
                FD_ZERO(&myset);
                FD_SET(sock, &myset);

                res = select(sock + 1, NULL, &myset, NULL, &tv);
                if (res < 0 && errno != EINTR)
                {
                    fprintf(stderr, "Error connecting %d - %s\n", errno, strerror(errno));
                    exit(0);
                }
                else if (res > 0)
                {
                    // Socket selected for write
                    lon = sizeof(int);
                    if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (void *)(&valopt), &lon) < 0)
                    {
                        fprintf(stderr, "Error in getsockopt() %d - %s\n", errno, strerror(errno));
                        exit(0);
                    }

                    // Check the value returned...
                    if (valopt)
                    {
                        fprintf(stderr, "Error in delayed connection() %d - %s\n", valopt, strerror(valopt));
                        exit(0);
                    }
                    break;
                }
                else
                {
                    fprintf(stderr, "Timeout in select() - Cancelling!\n");
                    exit(0);
                }
            } while (1);
        }
        else
        {
            fprintf(stderr, "Error connecting %d - %s\n", errno, strerror(errno));
            exit(0);
        }
    }

    /*----------------------------------
    |         SET BLOCKING MODE         |
    ------------------------------------*/
    if ((flags = fcntl(sock, F_GETFL, NULL)) < 0)
    {
        fprintf(stderr, "Error fcntl(..., F_GETFL) (%s)\n", strerror(errno));
        exit(0);
    }

    flags &= (~O_NONBLOCK);
    
    if (fcntl(sock, F_SETFL, flags) < 0)
    {
        fprintf(stderr, "Error fcntl(..., F_SETFL) (%s)\n", strerror(errno));
        exit(0);
    }

    /* We succeeded in connecting */
    return 0;
} /* connect_timeout() */

/* Returns the server_fd */
int server_bind(struct addrinfo *address_info_set)
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    int server_fd;
    struct addrinfo *i;


    /*----------------------------------
    |          ATTEMPT TO BIND          |
    ------------------------------------*/

    for (i = address_info_set; i != NULL; i = i->ai_next)
    {
        server_fd = socket(i->ai_family, i->ai_socktype, i->ai_protocol);
        if (server_fd == -1)
        {
            continue;
        }
        if (bind(server_fd, i->ai_addr, i->ai_addrlen) == 0)
        {
            break; /* Success */
        }

        close(server_fd);
    } /* for loop */


    /*----------------------------------
    |           VERIFICATION            |
    ------------------------------------*/
    /* Verify we successfully binded to an address */
    if (i == NULL)
    {
        fprintf(stderr, "Could not bind\n");
        exit(EXIT_FAILURE);
    }

    return server_fd;
} /* server_bind() */

/* Returns the client_fd */
int client_connect(struct addrinfo *address_info_set)
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    int client_fd;
    struct addrinfo *i;


    /*----------------------------------
    |         ATTEMPT TO CONNECT        |
    ------------------------------------*/

    for (i = address_info_set; i != NULL; i = i->ai_next)
    {
        client_fd = socket(i->ai_family, i->ai_socktype, i->ai_protocol);
        if (client_fd == -1)
        {
            continue;
        }
        if (connect_timeout(client_fd, i->ai_addr, i->ai_addrlen, CONNECT_TIMEOUT) != -1)
        {
            break; /* Success */
        }

        close(client_fd);
    } /* for loop */


    /*----------------------------------
    |           VERIFY SUCCESS          |
    ------------------------------------*/

    if (i == NULL)
    {
        fprintf(stderr, "Could not bind\n");
        exit(EXIT_FAILURE);
    }

    /* Print info */
    print_addrinfo(i);

    return client_fd;
} /* client_connect() */

/* Given a port number, socket type, an address (this can be IP or hostname), 
    and a value for the type_serv_client parameter, this will return a socket.
    If setting up a Server use IS_SERVER. Else use IS_CLIENT for the type_serv_client parameter.
    If setting up a server, addr can be NULL to use that machines IP.  */
int make_socket(char* port, uint8_t socket_type,  const char *addr, uint8_t type_serv_client)
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    int socket_fd;
    struct addrinfo hints;
    struct addrinfo *name;
    char ip[MAX_HOSTNAME_SZ];


    /*----------------------------------
    |          INITIALIZATIONS          |
    ------------------------------------*/
    memset(&hints, 0, sizeof(struct addrinfo));

    /* Set the socket info. */
    hints.ai_family = DEFAULT_CONN_TYPE;        /* ipv4/ipv6/any/local */
    hints.ai_socktype = socket_type;    /* TCP/UDP/local.... */
    hints.ai_protocol = 0;                      /* Any Port number. Don't worry, we set the desired port later */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    
    /* Initialize for addr. Check if addr is a valid hostname/IP address and get the IP address if it's a hostname. */
    if ( addr != NULL && hostname_to_ip(addr,ip) != -1 )
    {
        addr = ip;
    } 

    /* Settings/initializations specific to being a server/client */
    if ( type_serv_client == IS_SERVER ) /* If we are a server */
    {
        /* Set passive flag on server to signify we want to use this machine's IP/addr */
        hints.ai_flags = AI_PASSIVE;
        
        /* If we are a server, use addr NULL */
        addr = NULL;
    }
    else /* We are a client */
    {
        hints.ai_flags = 0; /* Flags for clients */
    }


    /*----------------------------------
    |       GET NETWORK ADDR INFO       |
    ------------------------------------*/
    int err = getaddrinfo(addr,port,&hints, &name);
    if ( err != 0 )
    {
        fprintf(stderr, "%s: %s\n", addr, gai_strerror(err));
        perror("ERROR: host/client address not valid");
        exit(EXIT_FAILURE);
    }

   
    /*----------------------------------
    |   GET SOCKET_FD AND BIND/CONNECT  |
    ------------------------------------*/
    
    /* Get socket file descriptor and bind if server or connect if client */
    if ( type_serv_client == IS_SERVER ) /* If we are a server wishing to bind */
    {
        socket_fd = server_bind( name );
    } 
    else /* We are a client wishing to connect to a server */
    {
        socket_fd = client_connect( name );
    }

    /*----------------------------------
    |            FREE MEMORY            |
    ------------------------------------*/
    freeaddrinfo(name);

    return socket_fd;
} /* make_socket() */

/* Given a socket file descriptor, reads data and returns the number of bytes read. This is a blocking call. */
int receive_data(int socket_fd, char* buffer, const size_t buffer_sz)
{
    //TODO see if data received is broken into MAX_MSG_SZ chucks or is received in its entirety
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
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
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    char buffer[MAX_MSG_SZ];        /* Buffer used to send data */
    uint16_t total_bytes_to_send;   /* Length of string we are to send */
    uint16_t all_sent_bytes;        /* Total number of bytes sent */
    uint16_t bytes_left_to_send;    /* Tally of the number of bytes left to sent */
    int bytes_sent;                 /* Number of bytes sent for a loop interation */
    uint16_t bytes_to_send;         /* Bytes to send during a loop */
    uint8_t n_buffers_to_send;      /* Number of loop interations (number of buffers) needed to send data */
    uint8_t send_flags;             /* Send Flags */

    /*----------------------------------
    |          INITIALIZATIONS          |
    ------------------------------------*/
    send_flags = 0;
    all_sent_bytes = 0;
    bytes_sent = 0;
    n_buffers_to_send = 0;

    total_bytes_to_send = strlen ( data ) + 1; /* Plus 1 to include termination terminal */
    bytes_left_to_send = total_bytes_to_send;
    n_buffers_to_send = ( (total_bytes_to_send)/MAX_MSG_SZ );

    if ( (total_bytes_to_send) % MAX_MSG_SZ != 0 )
    {
        /* There are additional bytes leftover, add one message */
        n_buffers_to_send+=1;
    }


    /*----------------------------------
    |             SEND DATA             |
    ------------------------------------*/
    for (uint8_t i = 0; i < n_buffers_to_send; i++)
    {
        bzero(buffer, MAX_MSG_SZ);

        /* Determine number of bytes to send */
        if (bytes_left_to_send > MAX_MSG_SZ)
        {
            bytes_to_send = MAX_MSG_SZ;
        }
        else
        {
            bytes_to_send = bytes_left_to_send;
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
        if (i != n_buffers_to_send-1 )
        {
            send_flags = 0;
        }
        else /*else this is not the last message to send */
        {
            send_flags = send_flags | MSG_MORE; /* signifies there is more data to send. */
        }
        
        /* Send the message */
        bytes_sent = send ( socket_fd, buffer, bytes_to_send, send_flags );
        
        /* Verify bytes were sent */
        if ( bytes_sent < 1 )
        {
            perror("ERROR: Failed to send data.");
            // exit(EXIT_FAILURE);
            return -1;
        }

        all_sent_bytes += bytes_sent;
        bytes_left_to_send -= bytes_sent;
        
    } /* For loop */

    /*----------------------------------
    |        VERIFY DATA WAS SENT       |
    ------------------------------------*/
    if ( all_sent_bytes != total_bytes_to_send )
    {
        printf("ERROR: sent %u of %u bytes\n", all_sent_bytes, total_bytes_to_send);
        perror("ERROR");
        exit(EXIT_FAILURE);
        return -1;
    }

    /* Info print */
    printf ("Sent data: \"%s\" \n", data);
    
    return all_sent_bytes;
} /* send_data */

#endif