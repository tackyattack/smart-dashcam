/*-------------------------------------
|               INCLUDES               |
--------------------------------------*/

#include "pub_socket_commons.h"
#include "prv_socket_commons.h"




/*-------------------------------------
|         FUNCTION DEFINITIONS         |
--------------------------------------*/

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
        return RETURN_FAILED;
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

    return RETURN_FAILED;
} /* hostname_to_ip() */

void socket_print_addrinfo(const struct addrinfo *addr)
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

} /* socket_print_addrinfo() */

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
    int returnval;


    /*----------------------------------
    |          INITIALIZATIONS          |
    ------------------------------------*/
    returnval = 0;


    /*----------------------------------
    |       SET NOT-BLOCKING MODE       |
    ------------------------------------*/
    if ((flags = fcntl(sock, F_GETFL, NULL)) < 0)
    {
        fprintf(stderr, "Error fcntl(..., F_GETFL) (%s)\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    flags |= O_NONBLOCK;

    if (fcntl(sock, F_SETFL, flags) < 0)
    {
        fprintf(stderr, "Error fcntl(..., F_SETFL) (%s)\n", strerror(errno));
        exit(EXIT_FAILURE);
    }


    /*----------------------------------
    |  ATTEMPT TO CONNECT WITH TIMEOUT  |
    ------------------------------------*/
    res = connect(sock, addr, addrlen);
    if (res < 0)
    {
        if (errno == EINPROGRESS)
        {
            // fprintf(stderr, "EINPROGRESS in connect() - selecting\n");
            do
            {
                /*----------------------------------
                |       LOOP INITIALIZATIONS        |
                ------------------------------------*/
                tv.tv_sec = timeout;
                tv.tv_usec = 0;
                FD_ZERO(&myset);
                FD_SET(sock, &myset);


                /*----------------------------------
                |       GET CONNECTION STATUS       |
                ------------------------------------*/
                res = select(sock + 1, NULL, &myset, NULL, &tv);
                if (res < 0 && errno != EINTR)
                {
                    fprintf(stderr, "Error connecting %d - %s\n", errno, strerror(errno));
                    exit(EXIT_FAILURE);
                }
                else if (res > 0)
                {
                    // Socket selected for write
                    lon = sizeof(int);
                    if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (void *)(&valopt), &lon) < 0)
                    {
                        fprintf(stderr, "Error in getsockopt() %d - %s\n", errno, strerror(errno));
                        exit(EXIT_FAILURE);
                    }

                    // Check the value returned...
                    if (valopt)
                    {
                        fprintf(stderr, "Error in delayed connection() %d - %s\n", valopt, strerror(valopt));
                        // exit(EXIT_FAILURE);
                        returnval = RETURN_FAILED;
                        break;
                    }
                    break;
                }
                else
                {
                    fprintf(stderr, "Timed out while attempting to connect to server!\n");
                    returnval = RETURN_FAILED;
                    break;
                    // exit(EXIT_FAILURE);
                }
            } while (1);
        }
        else
        {
            fprintf(stderr, "Error connecting %d - %s\n", errno, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }


    /*----------------------------------
    |         SET BLOCKING MODE         |
    ------------------------------------*/
    if ((flags = fcntl(sock, F_GETFL, NULL)) < 0)
    {
        fprintf(stderr, "Error fcntl(..., F_GETFL) (%s)\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    flags &= (~O_NONBLOCK);
    
    if (fcntl(sock, F_SETFL, flags) < 0)
    {
        fprintf(stderr, "Error fcntl(..., F_SETFL) (%s)\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    return returnval;
} /* connect_timeout() */

int server_bind(struct addrinfo *address_info_set)
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    int server_fd;
    int opt = 1;
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

        /* set server socket to allow multiple connections */  
        if( setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (void *)&opt, sizeof(opt)) < 0 )   
        {   
            perror("server_bind(): setsockopt error");   
            exit(EXIT_FAILURE);   
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
    if (i == NULL)
    {
        fprintf(stderr, "Could not bind\n");
        return RETURN_FAILED;
    }

    return server_fd;
} /* server_bind() */

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
        if (connect_timeout(client_fd, i->ai_addr, i->ai_addrlen, CONNECT_TIMEOUT) != RETURN_FAILED)
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
        fprintf(stderr, "Could not open socket\n");
        return RETURN_FAILED;
    }

    /* Print info */
    socket_print_addrinfo(i);

    return client_fd;
} /* client_connect() */

int socket_create_socket(char* port, enum SOCKET_TYPES socket_type,  const char *addr, enum SOCKET_OWNER type_serv_client)
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

    hints.ai_family = DEFAULT_CONN_TYPE;        /* ipv4/ipv6/any/local */
    hints.ai_socktype = socket_type;            /* TCP/UDP/local.... */
    hints.ai_protocol = 0;                      /* Any Port number. Don't worry, we set the desired port later */
    hints.ai_canonname = NULL;                  /* This would be a hostname but we use a different method */
    hints.ai_addr = NULL;                       /* This would be ip address but we dont use this */
    hints.ai_next = NULL;                       /* Next addrinfo struct pointer */
    
    if ( addr != NULL && hostname_to_ip(addr,ip) != RETURN_FAILED )
    {
        addr = ip;
    } 

    if ( type_serv_client == SOCKET_OWNER_IS_SERVER )
    {
        /* Set passive flag on server to signify we want to use this machine's IP/addr */
        hints.ai_flags = AI_PASSIVE;
        
        /* If we are a server, addr should be NULL */
        addr = NULL;
    }
    else /* We are a client */
    {
        hints.ai_flags = 0;
    }


    /*----------------------------------
    |       GET NETWORK ADDR INFO       |
    ------------------------------------*/
    int err = getaddrinfo(addr,port,&hints, &name);
    if ( err != 0 )
    {
        fprintf(stderr, "%s: %s\n", addr, gai_strerror(err));
        perror("ERROR: host/client address not valid");
        return RETURN_FAILED;
    }

   
    /*----------------------------------
    |   GET SOCKET_FD AND BIND/CONNECT  |
    ------------------------------------*/
    if ( type_serv_client == SOCKET_OWNER_IS_SERVER )
    {
        socket_fd = server_bind( name );
    } 
    else
    {
        socket_fd = client_connect( name );
    }


    /*----------------------------------
    |            FREE MEMORY            |
    ------------------------------------*/
    freeaddrinfo(name);

    return socket_fd;
} /* socket_create_socket() */

int remove_msg_header(char *buffer, int buffer_sz)
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    char *temp;


    /*----------------------------------
    |           VERIFICATION            |
    ------------------------------------*/
    if (buffer_sz <= 0)
    {
        return RETURN_FAILED;
    }
    assert(buffer != NULL);


    /*----------------------------------
    |       CHECK FOR MSG HEADERS       |
    ------------------------------------*/
    if (buffer[0] != MSG_START || buffer[buffer_sz-1] != MSG_END)
    {
        /* Info print */
        printf("Received invalid message!");
        return RETURN_FAILED;
    }
    //TODO should implement ability to search for/find the msg start/end


    /*----------------------------------
    |        REMOVE MSG HEADERS         |
    ------------------------------------*/
    /* Copy message (data minus header) to a temp string. Then erase buffer and copy message to buffer */
    temp = malloc(buffer_sz-MSG_HEADER_SZ);
    memcpy(temp, (const void *)&buffer[1], (buffer_sz-MSG_HEADER_SZ));
    bzero(buffer,buffer_sz);
    memcpy(buffer, temp, (buffer_sz-MSG_HEADER_SZ));


    /*----------------------------------
    |            FREE MEMORY            |
    ------------------------------------*/
    free(temp);

    return (int)(buffer_sz-MSG_HEADER_SZ);
} /* remove_msg_header */

int socket_receive_data(int socket_fd, char* buffer, const size_t buffer_sz)
{
    //TODO see if data received is broken into MAX_MSG_SZ chucks or is received in its entirety
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    int bytes_read;


    /*----------------------------------
    |   READ DATA FROM FILE DESCRIPTOR  |
    ------------------------------------*/
    //TODO see what happens if buffer is too small for received data
    bytes_read = read(socket_fd, buffer, buffer_sz);

    if (bytes_read < 0)
    {
        /* Read error. */
        fprintf (stderr, "errno = %d ", errno);
        perror("read");
    }
    else if (bytes_read < (int)MSG_HEADER_SZ )
    {
        /* Soft error: didn't receive minumum number of bytes expected */
        return RETURN_FAILED;
    }

    /* Info print */
    printf ("Received raw data: ");
    putchar('0');
    putchar('x');
    for (int i = 0; i < bytes_read; i++) {
        printf("%02x ", buffer[i]);
    }
    putchar('\n');
    putchar('\n');
    
    return remove_msg_header(buffer, bytes_read);

} /* socket_receive_data() */

int socket_send_data ( const int socket_fd, const char * data, const uint16_t data_sz )
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    char buffer[MAX_TX_MSG_SZ];     /* Buffer used to send data. Should be size data + MSG_START + MSG_END */
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

    n_buffers_to_send = data_sz/MAX_MSG_SZ;

    if ( data_sz % MAX_MSG_SZ != 0 )
    {
        n_buffers_to_send+=1;
    }

    total_bytes_to_send = data_sz + n_buffers_to_send * MSG_HEADER_SZ;
    bytes_left_to_send = total_bytes_to_send;


    /*----------------------------------
    |             SEND DATA             |
    ------------------------------------*/
    for (uint8_t i = 0; i < n_buffers_to_send; i++)
    {
        bzero(buffer, MAX_TX_MSG_SZ);

        /* Determine number of bytes to send this iteration */
        if (bytes_left_to_send > MAX_TX_MSG_SZ)
        {
            bytes_to_send = MAX_TX_MSG_SZ;
        }
        else
        {
            bytes_to_send = bytes_left_to_send;
        }

        /* Verify there are bytes to send */
        assert ( bytes_to_send > 0 );

        buffer[0] = MSG_START;
        /* Get substring to be sent and copy to buffer */
        memcpy( &buffer[1], &data[all_sent_bytes], bytes_to_send-MSG_HEADER_SZ);
        buffer[bytes_to_send-sizeof(MSG_END)] = MSG_END;
        
        /* if this is the last message to send in a sequence, set appropiate flag */ /* For send flags, see https://linux.die.net/man/2/send */
        if (i == n_buffers_to_send-1 )
        {
            send_flags = 0;
        }
        else /*else this is not the last message to send */
        {
            send_flags = send_flags | MSG_MORE;
        }
        
        /* Send the message */
        bytes_sent = send ( socket_fd, buffer, bytes_to_send, send_flags );
        
        /* Verify bytes were sent */
        if ( bytes_sent != bytes_to_send )
        {
            perror("ERROR: Failed to send data.");
            return RETURN_FAILED;
        }

        /* Info print */
        printf ("Sent data: ");
        putchar('0');
        putchar('x');
        for (int i = 0; i < bytes_to_send; i++) {
            printf("%02x ", buffer[i]);
        }
        putchar('\n');

        /* Tally bytes */
        all_sent_bytes += bytes_sent;
        bytes_left_to_send -= bytes_sent;
        
    } /* For loop */

    /* Info print */
    putchar('\n');

    /*----------------------------------
    |        VERIFY DATA WAS SENT       |
    ------------------------------------*/
    if ( all_sent_bytes != total_bytes_to_send )
    {
        printf("ERROR: sent %u of %u bytes\n", all_sent_bytes, total_bytes_to_send);
        perror("ERROR");
        return RETURN_FAILED;
    }

    return data_sz;
} /* socket_send_data */
