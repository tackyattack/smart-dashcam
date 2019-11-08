/*-------------------------------------
|               INCLUDES               |
--------------------------------------*/

#include "pub_socket_commons.h"
#include "prv_socket_commons.h"

/*-------------------------------------
|           STATIC VARIABLES           |
--------------------------------------*/
static pthread_mutex_t mutex_sendData = PTHREAD_MUTEX_INITIALIZER;

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
        /* Return the first ip addr found */
        strcpy(ip , inet_ntoa(*addr_list[i]) );
        printf("socket_commons: Found IP address \"%s\" for hostname \"%s\"\n",ip,hostname);

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
        printf("socket_commons: host=%s, serv=%s\n", hbuf, sbuf);
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
        fprintf(stderr, "ERROR: socket_commons: fcntl(..., F_GETFL) (%s)\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    flags |= O_NONBLOCK;

    if (fcntl(sock, F_SETFL, flags) < 0)
    {
        fprintf(stderr, "ERROR: socket_commons: fcntl(..., F_SETFL) (%s)\n", strerror(errno));
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
            /* fprintf(stderr, "EINPROGRESS in connect() - selecting\n"); */
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
                    fprintf(stderr, "ERROR: socket_commons: failed connecting %d - %s\n", errno, strerror(errno));
                    exit(EXIT_FAILURE);
                }
                else if (res > 0)
                {
                    /* Socket selected for write */
                    lon = sizeof(int);
                    if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (void *)(&valopt), &lon) < 0)
                    {
                        fprintf(stderr, "ERROR: socket_commons: failed in getsockopt() %d - %s\n", errno, strerror(errno));
                        exit(EXIT_FAILURE);
                    }

                    /* Check the value returned... */
                    if (valopt)
                    {
                        fprintf(stderr, "ERROR: socket_commons: failed in delayed connection() %d - %s\n", valopt, strerror(valopt));
                        returnval = RETURN_FAILED;
                        break;
                    }
                    break;
                }
                else
                {
                    fprintf(stderr, "WARNING: socket_commons: Timed out while attempting to connect to server!\n");
                    returnval = RETURN_FAILED;
                    break;
                }
            } while (1);
        }
        else
        {
            fprintf(stderr, "ERROR: socket_commons: failed connecting %d - %s\n", errno, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }


    /*----------------------------------
    |         SET BLOCKING MODE         |
    ------------------------------------*/
    if ((flags = fcntl(sock, F_GETFL, NULL)) < 0)
    {
        fprintf(stderr, "ERROR: socket_commons: failed fcntl(..., F_GETFL) (%s)\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    flags &= (~O_NONBLOCK);
    
    if (fcntl(sock, F_SETFL, flags) < 0)
    {
        fprintf(stderr, "ERROR: socket_commons: failed fcntl(..., F_SETFL) (%s)\n", strerror(errno));
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
            perror("ERROR: socket_commons: server_bind(): setsockopt error");   
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
        fprintf(stderr, "ERROR: socket_commons: Could not bind\n");
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
        fprintf(stderr, "ERROR: socket_commons: Could not open socket\n");
        return RETURN_FAILED;
    }

    /* Print info */
    socket_print_addrinfo(i);

    return client_fd;
} /* client_connect() */

int socket_create_socket( char* port, enum SOCKET_TYPES socket_type,  const char *addr, enum SOCKET_OWNER type_serv_client )
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
        perror("WARNING: socket_commons: host/client address not valid");
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

uint16_t crc16(const unsigned char* data_p, uint16_t length)
{
    /*-------------------------------------
    |              VARIABLES               |
    --------------------------------------*/
    uint16_t x;
    uint16_t crc = 0xFFFF;

    /*-------------------------------------
    |            GENERATE CRC16            |
    --------------------------------------*/
    while ( length-- )
    {
        x = crc >> 8 ^ *data_p++;
        x ^= x>>4;
        crc = (crc << 8) ^ ((uint16_t)(x << 12)) ^ ((uint16_t)(x <<5)) ^ ((uint16_t)x);
    }

    return crc;
} /* crc16() */

enum SOCKET_TX_RX_FLAGS \
socket_receive_data( const int socket_fd, struct socket_msg_struct** msg )
{
    /*-------------------------------------
    |       PARAMETER VERIFICATIONS        |
    --------------------------------------*/
    assert( *msg == NULL );
    assert( socket_fd >= 0 );

    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    ssize_t bytes_read;
    uint16_t header_checksum;
    uint16_t bytes_left_to_rx;
    uint16_t offset;
    struct MSG_HEADER header;
    struct socket_msg_struct* temp_msg;

    /*-------------------------------------
    |           INITIALIZATIONS            |
    --------------------------------------*/
    temp_msg = NULL;
    header_checksum = 0;
    bytes_read = 0;
    bytes_left_to_rx = 0;

    /*-------------------------------------
    |  READ MSG HEADER FROM FILE DESCRIPTOR  |
    --------------------------------------*/
    bytes_read = read(socket_fd, (void*)&header, MSG_HEADER_SZ);

    /*-------------------------------------
    |            VERIFICATIONS             |
    --------------------------------------*/
    if ( bytes_read < 0 )
    {
        /* Read error. */
        fprintf(stderr, "errno = %d ", errno);
        perror("ERROR: socket_commons: failed to recv data");
        return FLAG_DISCONNECT;
    }
    else if ( bytes_read == 0 )
    {
        /* Received disconect */
        return FLAG_DISCONNECT;
    }
    else if ( bytes_read < (ssize_t)MSG_HEADER_SZ ) /* Check that received enough bytes to comprise the msg header */
    {
        /* Soft error: didn't receive minumum number of bytes expected */
        return FLAG_HEADER_ERROR;
    }

    /*-------------------------------------
    |        VERIFY HEADER CHECKSUM        |
    --------------------------------------*/
    header_checksum = header.crc16_checksum;
    header.crc16_checksum = 0; /* Set this to zero because header checksum is generated with this set to 0 */

    if( header_checksum != crc16( (const unsigned char*)&header, MSG_HEADER_SZ ) )
    {
        return FLAG_HEADER_ERROR;
    } /* end if msg header checksum is invalid */

    /*-------------------------------------
    |           SETUP MSG STRUCT           |
    --------------------------------------*/
    temp_msg = malloc( sizeof(struct socket_msg_struct) );
    temp_msg->data = malloc( header.msg_num_bytes );
    temp_msg->data_sz = header.msg_num_bytes;
    temp_msg->recv_flag = FLAG_SUCCESS;
    temp_msg->next = NULL;

    /*-------------------------------------
    |       PRE-LOOP INITIALIZATIONS       |
    --------------------------------------*/
    bytes_read = 0;
    bytes_left_to_rx = temp_msg->data_sz;

    /*-------------------------------------
    |   LOOP UNTIL ALL MSG DATA RECEIVED   |
    --------------------------------------*/
    while( bytes_left_to_rx != 0 )
    {
        /* Update received msg array offset */
        offset = temp_msg->data_sz - bytes_left_to_rx;

        /*----------------------------------
        |   READ DATA FROM FILE DESCRIPTOR  |
        ------------------------------------*/
        bytes_read = read(socket_fd, (temp_msg->data + offset), bytes_left_to_rx);

        /*-------------------------------------
        |            VERIFICATIONS             |
        --------------------------------------*/
        if ( bytes_read < 0 )
        {
            free(temp_msg->data);
            free(temp_msg);
            temp_msg = NULL;
            /* Read error. */
            fprintf(stderr, "errno = %d ", errno);
            perror("ERROR: socket_commons: failed to recv data");
            return FLAG_DISCONNECT;
        }
        else if ( bytes_read == 0 )
        {
            free(temp_msg->data);
            free(temp_msg);
            temp_msg = NULL;
            /* Received disconect */
            return FLAG_DISCONNECT;
        }

        /*-------------------------------------
        |          UPDATE BYTES TO RX          |
        --------------------------------------*/
        bytes_left_to_rx -= bytes_read;
    } /* Receive msg loop */

    /*-------------------------------------
    |          SET RETURN POINTER          |
    --------------------------------------*/
    *msg = temp_msg;

    /* Info print */
    printf("socket_commons: Received %ud bytes of data: ", temp_msg->data_sz + (uint16_t)MSG_HEADER_SZ);
    printf("0x");
    for (int i = 0; i < (int)MSG_HEADER_SZ; i++)
    {
        printf("%02x ", ((char*)&header)[i]);
    } /* print header */
    printf("0x");
    for (int i = 0; i < temp_msg->data_sz; i++)
    {
        printf("%02x ", temp_msg->data[i]);
    } /* Print data */
    printf("\n\n");

    return FLAG_SUCCESS;

} /* socket_receive_data() */

int socket_send_data ( const int socket_fd, const char * data, const uint16_t data_sz )
{
    /*-------------------------------------
    |       PARAMETER VERIFICATIONS        |
    --------------------------------------*/
    /* Verify there are bytes to send */
    assert ( data_sz > 0 );
    assert ( data != NULL );
    assert( socket_fd >= 0 );

    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    static struct MSG_HEADER header;    /* Msg header inserted at the begining of every socket msg to send */
    uint16_t total_bytes_to_send;       /* Bytes to send including msg header */
    int bytes_sent;                     /* Number of bytes sent */
    int total_bytes_sent;               /* Number of bytes sent including msg header */
    uint8_t send_flags;                 /* Send Flags */

    /*----------------------------------
    |          INITIALIZATIONS          |
    ------------------------------------*/
    send_flags = 0;
    bytes_sent = 0;
    total_bytes_sent = 0;
    total_bytes_to_send = data_sz + MSG_HEADER_SZ;

    /* Set message header values */
    header.msg_type = 0x00;
    header.msg_num_bytes = data_sz; /* Number of bytes of the data msg of the msg */
    header.crc16_checksum = 0;      /* Set to 0 for checksum generation */
    header.crc16_checksum = crc16( (const unsigned char*)&header, MSG_HEADER_SZ );    /* Set header checksum */

    send_flags = 0; /* No flags */

    /*-------------------------------------------------
    |       FORCE THREADS TO ENTER SYNCHRONOUSLY       |
    --------------------------------------------------*/
    pthread_mutex_lock(&mutex_sendData);

    /*----------------------------------
    |             SEND DATA             |
    ------------------------------------*/
    /* Send msg header */
    bytes_sent = send ( socket_fd, (void*)&header, MSG_HEADER_SZ, send_flags );

    /* Verify bytes were sent */
    if( bytes_sent < 1 )
    {
        pthread_mutex_unlock(&mutex_sendData);
        perror("ERROR: socket_commons: Failed to send header. Disconnect...");
        return FLAG_DISCONNECT;
    }
    else if ( bytes_sent != MSG_HEADER_SZ )
    {
        pthread_mutex_unlock(&mutex_sendData);
        perror("ERROR: socket_commons: Failed to send data.");
        return RETURN_FAILED;
    }

    /* Tally bytes sent */
    total_bytes_sent += bytes_sent;

    /* Send data */
    bytes_sent = send ( socket_fd, data, data_sz, send_flags );

    /*------------------------------------------
    |  ALLOW NEW THREAD TO ENTER SYNCHRONOUSLY  |
    -------------------------------------------*/
    pthread_mutex_unlock(&mutex_sendData);

    /* Verify bytes were sent */
    if( bytes_sent < 1 )
    {
        perror("ERROR: socket_commons: Failed to header. Disconnect...");
        return FLAG_DISCONNECT;
    }
    else if ( bytes_sent != data_sz )
    {
        perror("ERROR: socket_commons: Failed to send all data.");
        return RETURN_FAILED;
    }

    /* Tally bytes sent */
    total_bytes_sent += bytes_sent;

    /* Info print */
    printf("socket_commons: Sent %d bytes of data: ", total_bytes_sent);
    printf("0x");
    for (int i = 0; i < (int)MSG_HEADER_SZ; i++)
    {
        printf("%02x ", ((char*)&header)[i]);
    } /* print header */
    printf("0x");
    for (int i = 0; i < data_sz; i++)
    {
        printf("%02x ", data[i]);
    } /* Print data */
    printf("\n\n");

    /*----------------------------------
    |        VERIFY DATA WAS SENT       |
    ------------------------------------*/
    if ( total_bytes_to_send != total_bytes_sent )
    {
        printf("ERROR: socket_commons: sent %u of %u bytes\n", total_bytes_sent, total_bytes_to_send);
        perror("ERROR: socket_commons");
        return RETURN_FAILED;
    }

    return data_sz;
} /* socket_send_data() */
