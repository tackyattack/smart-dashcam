/*-------------------------------------
|               INCLUDES               |
--------------------------------------*/

#include "pub_socket_server.h"
#include "prv_socket_server.h"


/*----------------------------------
|         STATIC VARIABLES          |
------------------------------------*/
static int server_socket_fd             = -1;       /* The socket file descriptor for the server (the local machine) */      
static struct client_info *client_infos = NULL;     /* struct pointer to struct that is a linked list of structs that contain info on the clients */
static fd_set active_fd_set             = {0};      /*  */
static bool isRunning                   = false;    /* Set true to execute the client execute thread */

static socket_lib_srv_rx_msg _rx_callback               = NULL; /* Callback called when a message is received from a client */
static socket_lib_srv_connected _conn_callback          = NULL; /* Callback called when a client has connected */
static socket_lib_srv_disconnected _disconn_callback    = NULL; /* Callback called when a client has disconnected */


/*-------------------------------------
|           PRIVATE MUTEXES            |
--------------------------------------*/

pthread_mutex_t mutex_isExecuting_thread = PTHREAD_MUTEX_INITIALIZER;


/*-------------------------------------
|         FUNCTION DEFINITIONS         |
--------------------------------------*/

void  INThandler(int sig)
{
    signal(sig, SIG_IGN);
    printf("\nCtrl-C detected. Exiting....\n");
    close(server_socket_fd);
    exit(EXIT_SUCCESS);
}

void  PIPEhandler(int sig)
{
    signal(sig, SIG_IGN);
    printf("\nPIPE Error detected. A client must have disconnected....\n");
    signal(SIGPIPE, PIPEhandler);
}

struct client_info* find_client_by_fd(const int socket)
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    struct client_info *client;

    /*----------------------------------
    |            INITIALIZE             |
    ------------------------------------*/
    client = NULL;


    /*----------------------------------
    |          SEARCH FOR UUID          |
    ------------------------------------*/

    for(client=client_infos; client!=NULL; client=client->next)
    {
        if( client->fd == socket )
        {
            break;
        }

    } /* for */

    return client;
} /* find_client */

struct client_info* find_client_by_uuid(const char* uuid)
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    struct client_info *client;

    /*----------------------------------
    |            INITIALIZE             |
    ------------------------------------*/
    client = NULL;

    /*----------------------------------
    |          SEARCH FOR UUID          |
    ------------------------------------*/

    for(client=client_infos; client!=NULL; client=client->next)
    {
        if( memcmp(client->uuid,uuid,UUID_SZ) == 0 )
        {
            break;
        }
        
    } /* for */

    return client;
} /* find_client */

int socket_server_init( char* port, socket_lib_srv_rx_msg rx_callback, socket_lib_srv_connected connect_callback, socket_lib_srv_disconnected discon_callback )
{
    /*----------------------------------
    |       SETUP SIGNAL HANDLERS       |
    ------------------------------------*/
    struct sigaction act_sigint,act_sigpipe;
    act_sigint.sa_handler = INThandler;
    sigaction(SIGINT, &act_sigint, NULL);
    act_sigpipe.sa_handler = PIPEhandler;
    sigaction(SIGPIPE, &act_sigpipe, NULL);

    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    int socket_fd;      /* generic socket variable */

    /*-------------------------------------
    |             VERIFICATION             |
    --------------------------------------*/
    if ( atoi(port) < 0 || atoi(port) > 65535 )
    {
        printf("ERROR: invalid port number %s!",port);
        return RETURN_SUCCESS;
    }

    if( server_socket_fd >= 0 )
    {
        return -1;
    }

    /*----------------------------------
    |           SET CALLBACKS           |
    ------------------------------------*/
    _rx_callback = rx_callback;
    _conn_callback = connect_callback;
    _disconn_callback = discon_callback;

    /*----------------------------------
    |       CREATE SERVER SOCKET        |
    ------------------------------------*/

    /* Info print */
    printf("Creating server on port %s\n", port);
    socket_fd = socket_create_socket(port, DEFAULT_SOCKET_TYPE, (const char*)SERVER_ADDR, SOCKET_OWNER_IS_SERVER);

    /*----------------------------------
    |       VERIFY SERVER CREATED       |
    ------------------------------------*/
    assert(socket_fd >= 0);

    /*-----------------------------------------
    |        SET GLOBAL SERVER_SOCKET_FD       |
    ------------------------------------------*/
    server_socket_fd = socket_fd;

    /*-----------------------------------------
    |  SET SERVER TO LISTEN FOR CONN REQUESTS  |
    -------------------------------------------*/

    if ( listen(server_socket_fd, MAX_PENDING_CONNECTIONS) < 0 )
    {
        fprintf (stderr, "errno = %d ", errno);
        perror("Set server to listen for incoming connections");
        exit(EXIT_FAILURE);
    }

    /* Info print */
    printf("Created server on port %s\n", port);

    return server_socket_fd;
}

int handle_conn_request()
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    struct sockaddr_in new_client_info;         /* socketaddr_in struct containing connecting client info */
    socklen_t new_client_sz;                    /* Size of the new clients socketaddr_in info struct */    
    int new_client_fd;                          /* File descriptor for new client */
    new_client_sz = sizeof(new_client_info);    /* Size of client info struct */
    struct client_info *new_client;             /* Struct containing client information such as UUID */

    /*----------------------------------
    |        ACCEPT CONN REQUEST        |
    ------------------------------------*/
    new_client_fd = accept(server_socket_fd, (struct sockaddr *)&new_client_info, &new_client_sz);

    /*----------------------------------
    |           VERIFY SUCCESS          |
    ------------------------------------*/
    if (new_client_fd < 0)
    {
        fprintf (stderr, "errno = %d ", errno);
        perror("Failed to accept connection request from client");
        return RETURN_FAILED;
    }

    /*----------------------------------
    |        SET/ADD CLIENT INFO        |
    ------------------------------------*/
    new_client = malloc(sizeof(struct client_info));
    assert(new_client!=NULL);
    new_client->fd = new_client_fd;
    new_client->address = new_client_info.sin_addr;
    bzero( new_client->uuid , UUID_SZ); /* Client will send this later */
    new_client->next = NULL;

    /* Add client to fd set and client infos */
    FD_SET(new_client_fd, &active_fd_set);
    if( client_infos == NULL )
    {
        client_infos = new_client;
    }
    else
    {
        struct client_info *client;
        client = NULL;

        /* Iterate until client->next is null */
        for(client=client_infos; client->next!=NULL; client=client->next);

        client->next = new_client;
    }

    /* Info print */
    fprintf(stderr, "Server: connect from host %s, port %u.\n", inet_ntoa(new_client_info.sin_addr), ntohs(new_client_info.sin_port));

    return new_client_fd;
} /* handle_conn_request() */

void close_client_conn(int client_fd)
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    struct client_info *client, *previous;

    /*----------------------------------
    |            INITIALIZE             |
    ------------------------------------*/
    client = NULL;
    previous = NULL;

    /* Info print */
    printf("Closing conenction to client.\n");

    /*----------------------------------
    |           CALL CALLBACK           |
    ------------------------------------*/
    /* Disconnecting client. Call callback with UUID */
    if(_disconn_callback != NULL)
    {
        (*_disconn_callback)( find_client_by_fd(client_fd)->uuid );
    }

    /*----------------------------------
    |     CLOSE CONN + REMOVE CLIENT    |
    ------------------------------------*/
    close(client_fd);

    /* Remove client from file descriptor set */
    FD_CLR(client_fd, (fd_set*)&active_fd_set);

    /* Remove client from list of client infos */
    for(client=client_infos; client!=NULL; client=client->next)
    {
        if(client->fd == client_fd)
        {
            if(previous!=NULL)
            {
                /* De-allocate the memory */
                if(client->next == NULL)
                {
                    free(client);
                    client = NULL;
                    previous->next = NULL;
                    break;
                }
                else
                {
                    struct client_info *temp;
                    temp=client->next;
                    free(client);
                    previous->next=temp;
                }
            }
            else
            {
                /* De-allocate the memory */
                if(client_infos->next == NULL)
                {
                    free(client_infos);
                    client_infos = NULL;
                }
                else
                {
                    struct client_info *temp;
                    temp=client_infos->next;
                    free(client_infos);
                    client_infos=temp;
                }
            }
        }
        previous = client;
    } /* for */

} /* close_client_conn() */

int process_recv_msg(int client_fd)
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    int n_recv_bytes;                  /* Number of received bytes */
    char* buffer;                          /* Buffer for reception of data */
    struct client_info *client;            /* client_infos client for given client_fd */
    int recv_flag;                         /* Used to receive message flags from the socket_receive_data function call */
    char* temp;                            /* Used for various tasks as needed for specific received commands. */

    /*----------------------------------
    |            INITIALIZE             |
    ------------------------------------*/
    buffer = malloc(MAX_TX_MSG_SZ);
    bzero(buffer,BUFFER_SZ);

    client = find_client_by_fd(client_fd);
    /* if client wasn't found in our list assert. There is a discrepancy. */
    assert(client != NULL);

    printf("\nSERVER: Bytes to read from socket: %d\n", socket_bytes_to_recv(client_fd));

    /*-------------------------------------
    |             VERIFICATION             |
    --------------------------------------*/

    if( socket_bytes_to_recv(client_fd) <= 0 )
    {
        printf("Socket Server: Received client disconnect/socket error. Disconnecting client...\n");
        close_client_conn(client_fd);
        return RETURN_SUCCESS;
    }

    /*-----------------------------------
    |       RECEIVE SOCKET MESSAGE       |
    ------------------------------------*/
    /* Receive data from client. Returns number of bytes received. */
    recv_flag = socket_receive_data( client_fd, buffer, MAX_TX_MSG_SZ, &n_recv_bytes );

    /*-------------------------------------
    |             VERIFICATION             |
    --------------------------------------*/

    if( n_recv_bytes <= 0 )
    {
        printf("Socket Server: Received client disconnect/socket error. Disconnecting client...\n");
        close_client_conn(client_fd);
        return RETURN_SUCCESS;
    }

    if(recv_flag == RECV_ERROR)
    {
        printf("Socket Server: Message header error. Message discarded.\n");
        return RETURN_FAILED; /* Return one to prevent indication of client disconect */
    }

    /* Info prints. Data read. */    
    fprintf(stderr, "SERVER: received %d bytes\n", n_recv_bytes);
    // fprintf(stderr, "Server: received %d bytes with message: \"%s\"\n", n_recv_bytes, buffer);

    // putchar('0');
    // putchar('x');
    // for (int i = 0; i < n_recv_bytes; i++)
    // {
    //     printf("%02x ", buffer[i]);
    // }
    // putchar('\n');
    // putchar('\n');

    /*-----------------------------------
    |          HANDLE MSG FLAGS          |
    ------------------------------------*/
    if( recv_flag == RECV_SEQUENCE_CONTINUE || recv_flag == RECV_SEQUENCE_END )
    {
        temp = malloc(client->partialMSG_sz + n_recv_bytes);

        if (client->partialMSG_sz != 0)
        {
            memcpy(temp, client->partialMSG, client->partialMSG_sz);
        }

        memcpy( (temp + client->partialMSG_sz), buffer, n_recv_bytes );

        if (client->partialMSG_sz != 0)
        {
            free(client->partialMSG);
        }

        client->partialMSG = temp;
        client->partialMSG_sz += n_recv_bytes;

        if (recv_flag != RECV_SEQUENCE_END)
        {
            /* Nothing more to do until finished receiving data */
            return RETURN_SUCCESS;
        } /* if didn't receive RECV_SEQUENCE_END flag */

    } /* handle msg flags */
    else /* Nothing special (no msg flags to handle) */
    {
        client->partialMSG    = buffer;
        client->partialMSG_sz = n_recv_bytes;
    }


    /*-----------------------------------
    |      HANDLE RECEIVED COMMANDS      |
    ------------------------------------*/
    switch (client->partialMSG[0])
    {
    case COMMAND_UUID:
        if ( client->partialMSG_sz < (ssize_t)(COMMAND_SZ+UUID_SZ) )
        {
            return client->partialMSG_sz;
        }

        /*-----------------------------------
        |     SET CLIENT UUID IF NOT SET     |
        ------------------------------------*/
        if(client->uuid[0] == 0x00)
        {
            memcpy(client->uuid,&client->partialMSG[1],UUID_SZ);
            printf("Socket Server: Setting client UUID for the first time. UUID is %s\n",client->uuid);

            /*----------------------------------
            |           CALL CALLBACK           |
            ------------------------------------*/
            /* New client sent us it's UUID for the first time. Call callback new client connected callback  with UUID */
            if(_conn_callback != NULL)
            {
                (*_conn_callback)( client->uuid );
            }
        }
        else
        {
            memcpy(client->uuid,&client->partialMSG[1],UUID_SZ);
        }
        break;
    case COMMAND_NONE:
        /*----------------------------------
        |           CALL CALLBACK           |
        ------------------------------------*/
        /* Received message from client. Call callback with message and UUID */
        if(_rx_callback != NULL)
        {
            (*_rx_callback)(client->uuid, &(client->partialMSG)[COMMAND_SZ], client->partialMSG_sz-COMMAND_SZ);
        }

        break;
    default:
        printf("ERROR: socket client received message without valid COMMAND\n");
        return 0;
        break;
    }

    n_recv_bytes = client->partialMSG_sz;

    /*-------------------------------------
    |               CLEANUP                |
    --------------------------------------*/
    if (client->partialMSG_sz != 0)
    {
        free(client->partialMSG);
    }
    client->partialMSG = NULL;
    client->partialMSG_sz = 0;

    return n_recv_bytes;
} /* process_recv_msg */

int socket_server_send_data_all(const char* data, const uint data_sz)
{
    return send_data_all(COMMAND_NONE, data, data_sz);
} /* socket_server_send_data_all() */

int socket_server_send_data( const char* uuid, const char* data, const uint data_sz )
{
    return send_data(uuid, COMMAND_NONE, data, data_sz);
} /* socket_server_send_data() */

int send_data_all(const uint8_t command, const char* data, const uint data_sz)
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    struct client_info *client;
    int returnval, n;
    char* temp;

    /*----------------------------------
    |            INITIALIZE             |
    ------------------------------------*/
    client = NULL;
    returnval = 0;

    /*-------------------------------------
    |        CREATE MESSAGE TO SEND        |
    --------------------------------------*/
    if( data == NULL || data_sz == 0 )
    {
        temp = malloc(COMMAND_SZ);
        memcpy(temp,&command,COMMAND_SZ);
    }
    else
    {
        temp = malloc(data_sz+COMMAND_SZ);
        memcpy(temp,&command,COMMAND_SZ);
        memcpy( (temp+COMMAND_SZ), data, data_sz );        
    }

    /*----------------------------------
    |             SEND MSGS             |
    ------------------------------------*/
    for(client=client_infos; client!=NULL; client=client->next)
    {
        n = socket_send_data(client->fd,temp,data_sz+COMMAND_SZ);
        if( n < 1 )
        {
            /* Disconnect client if failed to send message */
            close_client_conn(client->fd);
        }
        else
        {
            /* Tally total bytes sent */
            returnval += n;
        }

    } /* for */

    free(temp);

    return returnval;
} /* send_all_data() */

int send_data ( const char* uuid, const uint8_t command, const char * data, uint data_sz )
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    struct client_info *client;
    int returnval, n;
    char* temp;

    /*----------------------------------
    |            INITIALIZE             |
    ------------------------------------*/
    client = find_client_by_uuid(uuid);
    returnval = 0;

    /*-------------------------------------
    |            VERIFICATIONS             |
    --------------------------------------*/
    if(client == NULL)
    {
        return RETURN_FAILED;
    }

    /*-------------------------------------
    |        CREATE MESSAGE TO SEND        |
    --------------------------------------*/
    if( data == NULL || data_sz == 0 )
    {
        temp = malloc(COMMAND_SZ);
        memcpy(temp,&command,COMMAND_SZ);
        data_sz = 0;
    }
    else
    {
        temp = malloc(data_sz+COMMAND_SZ);
        memcpy(temp,&command,COMMAND_SZ);
        memcpy( (temp+COMMAND_SZ), data, data_sz );        
    }

    /*-------------------------------------
    |             SEND MESSAGE             |
    --------------------------------------*/
    n = socket_send_data(client->fd,temp,data_sz+COMMAND_SZ);
    if( n < 1 )
    {
        /* Disconnect client if failed to send message */
        close_client_conn(client->fd);
    }
    else
    {
        /* Tally total bytes sent */
        returnval = n;
    }

    free(temp);

    return returnval;

} /* send_data() */

void service_sockets(const fd_set *read_fd_set)
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    int i;

    /*----------------------------------
    |     SERVICE SOCKETS WITH INPUT    |
    ------------------------------------*/
    for (i = 0; i < FD_SETSIZE; ++i)
    {
        /* Check if clients fd is set */
        if (FD_ISSET(i, read_fd_set))
        {
            if (i == server_socket_fd)
            {
                /*----------------------------------
                |          SETUP NEW CLIENT         |
                ------------------------------------*/
                handle_conn_request();
                assert(client_infos != NULL);
            }
            else /* A client has sent us a message */
            {
                process_recv_msg(i);
            } /* else */

        } /* if (FD_ISSET) */

    } /* For loop */

} /* service_sockets */

void* execute_thread(void* args)
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    fd_set read_fd_set;                         /* generic file descriptor set for sockets */
    struct timeval timeout;                     /* Used to set select() timeout time */
    int select_return;                          /* Return value of select() */
    time_t lastPing;                            /* Last time we sent pings */
    long timeout_s_reload, timeout_us_reload;

    /*----------------------------------
    |            INITIALIZE             |
    ------------------------------------*/
    /* Initialize the set of active sockets. */
    FD_ZERO(&active_fd_set);
    FD_SET(server_socket_fd, &active_fd_set);
    lastPing          = 0;
    timeout_s_reload  = SELECT_TIMEOUT_TIME/1000;          /* Determine number of seconds */
    timeout_us_reload = (SELECT_TIMEOUT_TIME%1000)*1000;   /* Determine number of microseconds */

    /*----------------------------------
    |           INFINITE LOOP           |
    ------------------------------------*/
    while (1)
    {
        /*----------------------------------
        |     ITERATION INITIALIZATION      |
        ------------------------------------*/
        /* Set select() timeout values to block for SELECT_TIMEOUT_TIME ms */
        timeout.tv_sec  = timeout_s_reload;
        timeout.tv_usec = timeout_us_reload;
        read_fd_set = active_fd_set;

        /*----------------------------------
        |     BLOCK UNTIL RECV OR TIMEOUT   |
        ------------------------------------*/
        select_return = select(FD_SETSIZE, &read_fd_set, NULL, NULL, &timeout);
        if ( select_return < 0)
        {
            fprintf (stderr, "errno = %d ", errno);
            perror("select");
            break;
        }

        if(select_return != 0) /* Received a message */
        {
            service_sockets(&read_fd_set);
        }

        /*----------------------------------
        |            PING CLIENTS           |
        ------------------------------------*/
        if ( TIME_BETWEEN_PINGS <= (time(NULL)-lastPing) && client_infos != NULL)
        {
            /* Info print */
            printf("Server: Sending Ping to clients\n");

            send_data_all(COMMAND_PING, NULL, COMMAND_SZ);
            lastPing = time(NULL);
        }

    } /* while (1) */

    /*-----------------------------------
    |        SET ISRUNNING FALSE         |
    ------------------------------------*/
    pthread_mutex_lock(&mutex_isExecuting_thread);
    *((bool*)args) = false;
    pthread_mutex_unlock(&mutex_isExecuting_thread);

    return NULL;

} /* execute_thread() */

void socket_server_execute()
{
    /*-----------------------------------
    |         SET ISRUNNING TRUE         |
    ------------------------------------*/
    pthread_mutex_lock(&mutex_isExecuting_thread);
    if( isRunning == true )
    {
        pthread_mutex_unlock(&mutex_isExecuting_thread);
        return;
    }
    isRunning = true;
    pthread_mutex_unlock(&mutex_isExecuting_thread);

    /*-----------------------------------
    |        SPAWN EXECUTE THREAD        |
    ------------------------------------*/
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, execute_thread, &isRunning);

    if( 0 != pthread_detach(thread_id) )
    {
        printf("\nFailed to create client execute thread!\n");
        exit(EXIT_FAILURE);
    }
} /* socket_server_execute() */

bool socket_server_is_executing()
{
    bool return_val;
    /*-----------------------------------
    |           GET ISRUNNING            |
    ------------------------------------*/
    pthread_mutex_lock(&mutex_isExecuting_thread);
    return_val = isRunning;
    pthread_mutex_unlock(&mutex_isExecuting_thread);

    return return_val;
} /* socket_server_is_executing() */

void socket_server_quit()
{
    /*-----------------------------------
    |         CLOSE SERVER SOCKET        |
    ------------------------------------*/
    if ( RETURN_FAILED == close(server_socket_fd) )
    {
        printf("\nFailed: socket_server_quit() failed to close socket!\n");
    }

    struct client_info *client, *temp;

    client = client_infos;
    while( client != NULL )
    {
        temp = client;
        client = client->next;

        /*----------------------------------
        |    CLOSE CONNS + REMOVE CLIENTS   |
        ------------------------------------*/
        close(temp->fd);

        /* Remove client from file descriptor set */
        FD_CLR(temp->fd, (fd_set*)&active_fd_set);

        free(temp);
    } /* for loop */
    // FIXME the server thread may not exit before setting this variable. Therefore, seg fault possible
        // This line was added only as a safe guard indicator in the init function. It checks if this value is -1 to determine if it should fail init
    server_socket_fd = -1;

} /* socket_server_quit() */

