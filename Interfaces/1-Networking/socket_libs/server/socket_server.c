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
    printf("Creating server on port %s\n", port); /* Info print */
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

    printf("Created server on port %s\n", port); /* Info print */

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
    bzero(new_client->uuid , UUID_SZ); /* Client will send this later */
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
    fprintf(stderr, "Server: connect from host %s.\n", inet_ntoa(new_client_info.sin_addr));

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
    client = find_client_by_fd(client_fd);
    if( _disconn_callback != NULL && client->recv_uuid == true ) /* if recv_uuid == false, then the connect callback wasn't called and therefore the disconnect callback shouldn't be called */
    {
        (*_disconn_callback)( client->uuid );
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

void process_recv_msg(int client_fd)
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    struct client_info *client;            /* client_infos client for given client_fd */
    int recv_flag;                         /* Used to receive message flags from the socket_receive_data function call */
    struct socket_msg_struct* msg;         /* Message struct pointer that will contain received message */   
    struct socket_msg_struct* msg_previous;/* Used to free previous msg struct while looping */

    /*----------------------------------
    |            INITIALIZE             |
    ------------------------------------*/
    msg            = NULL;
    msg_previous   = NULL;
    client         = find_client_by_fd(client_fd);
    assert(client != NULL);                 /* if client wasn't found in our list, assert. There is a discrepancy. */

    /*-----------------------------------
    |       RECEIVE SOCKET MESSAGE       |
    ------------------------------------*/
    recv_flag = socket_receive_data( client_fd, &msg );

    /*-------------------------------------
    |             VERIFICATION             |
    --------------------------------------*/
    if( recv_flag == FLAG_DISCONNECT )
    {
        printf("Socket Server: Received client disconnect/socket error. Disconnecting client...\n");
        close_client_conn(client_fd);
        return;
    }
    else if( recv_flag == FLAG_HEADER_ERROR )
    {
        printf("Socket Client: Received invalid msg header...ignoring\n");
        return;
    }

    /*-----------------------------------
    |    HANDLE ANY RECEIVED COMMANDS    |
    ------------------------------------*/
    while ( msg != NULL )
    {
        /* Print header received */
        printf("SERVER: msg command received: 0x%02x with flag %d\n", msg->command, msg->recv_flag);
        /* Print data received */
        // fprintf(stderr, "SERVER: received %d bytes\n", msg->data_sz);
        // for (int i = 0; i < msg->data_sz; i++)
        // {
        //     printf("%c", msg->data[i]);
        // }
        // printf("\n\n");

        /* Handle msg command */
        switch (msg->command)
        {
        case COMMAND_UUID:
            if ( msg->data_sz != (ssize_t)(UUID_SZ) )
            {
                printf("Socket Server: Received invalid UUID: %s!\n",client->uuid);
                break;
            }

            /*-----------------------------------
            |     SET CLIENT UUID IF NOT SET     |
            ------------------------------------*/
            if(client->recv_uuid == false)
            {
                /* New client sent us it's UUID for the first time. */
                memcpy(client->uuid,msg->data,UUID_SZ);
                printf("Socket Server: Setting client UUID for the first time. UUID is %s\n",client->uuid);

                /*----------------------------------
                |           CALL CALLBACK           |
                ------------------------------------*/
                if(_conn_callback != NULL)
                {
                    (*_conn_callback)( client->uuid );
                }

                /* set bool indicating we've received this client's uuid */
                client->recv_uuid = true;
            }
            break;
        case COMMAND_NONE:
            /*----------------------------------
            |           CALL CALLBACK           |
            ------------------------------------*/
            if(_rx_callback != NULL)
            {
                (*_rx_callback)(client->uuid, msg->data, msg->data_sz);
            }
            break;
        default:
            printf("ERROR: socket client received message without valid COMMAND\n");
            break;
        } /* end switch case on recv command */

        /* Loop inits and cleanup */
        msg_previous = msg;
        msg = msg->next;
        if(msg_previous->data != NULL)
        {
            free(msg_previous->data);
        }
        free(msg_previous);
    } /* While loop ... loop through all messages received */

} /* process_recv_msg() */

int socket_server_send_data( const char* uuid, const char* data, const uint data_sz )
{
    return send_data(uuid, COMMAND_NONE, data, data_sz);
} /* socket_server_send_data() */

void socket_server_send_data_all(const char* data, const uint data_sz)
{
    send_data_all(COMMAND_NONE, data, data_sz);
} /* socket_server_send_data_all() */

void send_data_all(const uint8_t command, const char* data, const uint data_sz)
{
    assert( (data == NULL && data_sz == 0) || (data != NULL && data_sz != 0) );

    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    struct client_info *client;
    int n;

    /*----------------------------------
    |            INITIALIZE             |
    ------------------------------------*/
    client = NULL;

    /*----------------------------------
    |             SEND MSGS             |
    ------------------------------------*/
    for(client=client_infos; client!=NULL; client=client->next)
    {
        n = socket_send_data(client->fd, data, data_sz, command);

        if( n < 0 ) /* Disconnect client if failed to send message */
        {
            close_client_conn(client->fd);
        }
    } /* for each client ... send msg */

} /* send_all_data() */

int send_data ( const char* uuid, const uint8_t command, const char * data, uint data_sz )
{
    assert( (data == NULL && data_sz == 0) || (data != NULL && data_sz != 0) );

    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    struct client_info *client;
    int returnval, n;

    /*----------------------------------
    |            INITIALIZE             |
    ------------------------------------*/
    client = find_client_by_uuid(uuid);
    returnval = RETURN_SUCCESS;

    /*-------------------------------------
    |            VERIFICATIONS             |
    --------------------------------------*/
    if(client == NULL)
    {
        return RETURN_FAILED;
    }

    /*-------------------------------------
    |             SEND MESSAGE             |
    --------------------------------------*/
    n = socket_send_data(client->fd, data, data_sz, command);

    /*-------------------------------------
    |             VERIFICATION             |
    --------------------------------------*/
    if( n < 0 ) /* Disconnect client if failed to send message */
    {
        close_client_conn(client->fd);
        returnval = RETURN_FAILED;
    }
    else /* Tally total bytes sent */
    {
        returnval = n;
    }

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
        if ( !FD_ISSET(i, read_fd_set) ) /* Check if client's fd is not set */
        {
            continue;
        }

        if (i == server_socket_fd) /* Server FD set, handle conn request */
        {
            /*----------------------------------
            |          SETUP NEW CLIENT         |
            ------------------------------------*/
            handle_conn_request();
            assert(client_infos != NULL);
        }
        else /* A client FD is set, client has sent us a message */
        {
            process_recv_msg(i);
        } /* else */

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
        read_fd_set = active_fd_set;

        /* Set select() timeout values to block for SELECT_TIMEOUT_TIME ms */
        timeout.tv_sec  = timeout_s_reload;
        timeout.tv_usec = timeout_us_reload;

        /*----------------------------------
        |     BLOCK UNTIL RECV OR TIMEOUT   |
        ------------------------------------*/
        select_return = select(FD_SETSIZE, &read_fd_set, NULL, NULL, &timeout);

        if ( select_return < 0) /* select() returned flag indicating error/problem */
        {
            fprintf (stderr, "errno = %d ", errno);
            perror("select");
            break;
        }

        if( select_return != 0 ) /* Received a message */
        {
            service_sockets(&read_fd_set);
        }

        /*----------------------------------
        |            PING CLIENTS           |
        ------------------------------------*/
        if ( TIME_BETWEEN_PINGS <= (time(NULL)-lastPing) && client_infos != NULL)
        {
            printf("Server: Sending Ping to clients\n"); /* Info print */

            send_data_all(COMMAND_PING, NULL, 0);
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
    /*-------------------------------------
    |              VARIABLES               |
    --------------------------------------*/
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

    /*-------------------------------------
    |              VARIABLES               |
    --------------------------------------*/
    struct client_info *client, *temp;

    /*-------------------------------------
    |           INITIALIZATIONS            |
    --------------------------------------*/
    client = client_infos;

    /*-------------------------------------
    |     CLOSE ALL CLIENT SOCKET FD'S     |
    --------------------------------------*/
    while( client != NULL )
    {
        /* Loop initializations */
        temp = client;
        client = client->next;

        /*----------------------------------
        |    CLOSE CONNS + REMOVE CLIENTS   |
        ------------------------------------*/
        close(temp->fd);

        /* Remove client from file descriptor set */
        FD_CLR(temp->fd, (fd_set*)&active_fd_set);

        /* Loop cleanup */
        free(temp);
    } /* for loop */

    // FIXME the server thread may not exit before setting this variable. Therefore, seg fault possible
        // This line was added only as a safe guard indicator in the init function. It checks if this value is -1 to determine if it should fail init
    server_socket_fd = -1;

} /* socket_server_quit() */

