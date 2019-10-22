/*-------------------------------------
|               INCLUDES               |
--------------------------------------*/

#include "prv_socket_client.h"
#include "pub_socket_client.h"


/*-------------------------------------
|           STATIC VARIABLES           |
--------------------------------------*/

static int  client_fd                                  = -1;    /* Stores the socket_fd for our connection to the server */
static char UUID[UUID_STR_LEN]                         = {0};   /* Stores our UUID including termination char */
static bool isRunning                                  = false; /* Set true to execute the client execute thread */
static socket_lib_clnt_rx_msg _rx_callback             = NULL;  /* Callback called when a message is received from the server */
static socket_lib_clnt_disconnected _discont_callback  = NULL;  /* Callback called when we've disconnected from the server */
static char* partial_rx_msg                            = NULL;  /* Used when a message spans multiple packets. This holds the partial message data until all of the data has been received */
static ssize_t partial_rx_msg_sz                       = 0;     /* Used to keep track of the size of partial_rx_msg */

/*-------------------------------------
|           PRIVATE MUTEXES            |
--------------------------------------*/

pthread_mutex_t mutex_isExecuting_thread = PTHREAD_MUTEX_INITIALIZER;


/*-------------------------------------
|         FUNCTION DEFINITIONS         |
--------------------------------------*/

void uuid_create()
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    uuid_t new_uuid;


    /*----------------------------------
    |       CREATE AND SAVE UUID        |
    ------------------------------------*/
    uuid_generate(new_uuid);
    uuid_unparse_lower(new_uuid, UUID);

} /* uuid_create() */

void load_uuid()
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    FILE *fptr;
    char buffer[UUID_SZ]; /* Defined at compile time */


    /*----------------------------------
    |          INITIALIZATIONS          |
    ------------------------------------*/
    assert(UUID_FILE_NAME != NULL);
    fptr = fopen(UUID_FILE_NAME,"r"); /* UUID_FILE_NAME is defined at compile time */

    if(fptr != NULL)
    {
        assert(fgets(buffer,UUID_SZ,fptr) != NULL); /* get uuid string from file */
    }
    else
    {
        uuid_create();
        fptr = fopen(UUID_FILE_NAME,"w"); /* UUID_FILE_NAME is defined at compile time */
        fputs(UUID, fptr);
    }


    /*----------------------------------
    |         SAVE UUID TO UUID         |
    ------------------------------------*/
    memcpy(UUID,buffer,UUID_SZ); /* Copy data */
    printf("Socket Client: UUID is %s\n",UUID);

    /*----------------------------------
    |             CLEAN-UP              |
    ------------------------------------*/
    fclose(fptr);

} /* load_uuid() */

int send_uuid()
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    int sent_bytes;


    /*----------------------------------
    |             SEND UUID             |
    ------------------------------------*/
    sent_bytes = send_data(COMMAND_UUID, UUID, UUID_STR_LEN);   /* Send UUID with null termination to server to identify ourselves */
    if(sent_bytes != UUID_STR_LEN+COMMAND_SZ)
    {
        printf("\nERROR: failed to send UUID correctly to server. Send %d bytes but expected %d bytes...\n\n", sent_bytes, UUID_STR_LEN);
    }

    return sent_bytes;
} /* send_uuid() */

int socket_client_init(char* server_addr, char *port, socket_lib_clnt_rx_msg rx_callback, socket_lib_clnt_disconnected discnt_callback)
{
    /*-------------------------------------
    |             SET CALLBACKS            |
    --------------------------------------*/
    _rx_callback        = rx_callback;
    _discont_callback   = discnt_callback;


    /*-------------------------------------
    |       LOAD AND OR CREATE UUID        |
    --------------------------------------*/
    load_uuid();


    /*-------------------------------------
    |             VERIFICATION             |
    --------------------------------------*/
    if ( atoi(port) < 0 || atoi(port) > 65535 )
    {
        printf("ERROR: invalid port number %s!",port);
        return RETURN_FAILED;
    }


    /* Info print */
    printf ("\nAttempt to open socket to server....\n");

    client_fd = socket_create_socket(port, DEFAULT_SOCKET_TYPE, server_addr, SOCKET_OWNER_IS_CLIENT);

    if(client_fd < 0)
    {
        return RETURN_FAILED;
    }


    /*-------------------------------------
    |         SEND UUID AND VERIFY         |
    --------------------------------------*/

    if( send_uuid() != UUID_STR_LEN+COMMAND_SZ )
    {
        close_and_notify(client_fd);
        return RETURN_FAILED;
    }

    return RETURN_SUCCESS;
} /* socket_client_init() */

int process_recv_msg(const int socket_fd)
{
    /*-------------------------------------
    |              VARIABLES               |
    --------------------------------------*/
    char *buffer;
    int return_val;
    char* temp;
    int n_recv_bytes;
    enum SOCKET_RECEIVE_DATA_FLAGS recv_flag;


    /*-------------------------------------
    |             VERIFICATION             |
    --------------------------------------*/

    if( socket_bytes_to_recv(socket_fd) <= 0 )
    {
        printf("Socket Client: Received disconnect/socket error. Disconnecting from server...\n");
        close_and_notify();
        return RETURN_DISCONNECT;
    }


    /*-------------------------------------
    |           INITIALIZATIONS            |
    --------------------------------------*/
    buffer = malloc(MAX_TX_MSG_SZ); /* We use this instead of MAX_MSG_SZ because we have to account for the message headers that are included even though they aren't returned */
    bzero(buffer,MAX_TX_MSG_SZ);
    return_val = RETURN_SUCCESS;

    /*----------------------------------
    |     RECEIVE DATA FROM SERVER      |
    ------------------------------------*/
    recv_flag = socket_receive_data(socket_fd,buffer,MAX_TX_MSG_SZ,&n_recv_bytes);

    if ( RECV_ERROR == recv_flag )
    {
        printf("\nClient received invalid msg header.\n");
        return RETURN_SUCCESS;
    }

    /*-----------------------------------
    |          HANDLE MSG FLAGS          |
    ------------------------------------*/
    if( recv_flag == RECV_SEQUENCE_CONTINUE || recv_flag == RECV_SEQUENCE_END )
    {
        temp = malloc(partial_rx_msg_sz + n_recv_bytes);
        memcpy(temp, partial_rx_msg, partial_rx_msg_sz);
        memcpy( (temp + partial_rx_msg_sz), buffer, n_recv_bytes );

        free(partial_rx_msg);
        partial_rx_msg = temp;
        partial_rx_msg_sz += n_recv_bytes;

        if (recv_flag == RECV_SEQUENCE_END)
        {
            /*----------------------------------
            |           CALL CALLBACK           |
            ------------------------------------*/
            /* Received message from server. Call callback with message and UUID */
            if(_rx_callback != NULL)
            {
                (*_rx_callback)(partial_rx_msg, partial_rx_msg_sz);
            }

            /* Continue to message command processing */

        } /* if RECV_SEQUENCE_END */
        else /* Nothing more to do until finished receiving data*/
        {
            return RETURN_SUCCESS;
        }
    } /* handle msg flags */
    else /* Nothing special */
    {
        partial_rx_msg    = buffer;
        partial_rx_msg_sz = n_recv_bytes;
    }


    /*----------------------------------
    |  PROCESS COMMANDS OR RX_CALLBACK  |
    ------------------------------------*/
    switch (partial_rx_msg[0])
    {
    case COMMAND_PING:
    case COMMAND_UUID:
        send_uuid();
        break;
    case COMMAND_NONE:
        /*----------------------------------
        |           CALL CALLBACK           |
        ------------------------------------*/
        /* Received message from client. Call callback with message and UUID */
        if(_rx_callback != NULL)
        {
            (*_rx_callback)(partial_rx_msg, partial_rx_msg_sz);
        }
        break;

    default:
        printf("ERROR: socket client received message without valid COMMAND\n");

        return_val = RETURN_FAILED;
        break;
    }

    /*-------------------------------------
    |               CLEANUP                |
    --------------------------------------*/
    free(partial_rx_msg);
    partial_rx_msg_sz = 0;

    return return_val;

} /* process_recv_msg() */

void* execute_thread(void* args)
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    fd_set client_fd_set,working_fd_set;
    struct timeval timeout;
    int val;


    /*----------------------------------
    |          INITIALIZATIONS          |
    ------------------------------------*/
    FD_ZERO(&client_fd_set);
    FD_ZERO(&working_fd_set);
    FD_SET(client_fd, &client_fd_set);


    /*----------------------------------
    |           INFINITE LOOP           |
    ------------------------------------*/
    while(1)
    {
        /*----------------------------------
        |     ITERATION INITIALIZATIONS     |
        ------------------------------------*/
        timeout.tv_sec  = SERVER_PING_TIMEOUT; /* If nothing received from server for 2 seconds, assume lost connection */
        timeout.tv_usec = 0;
        working_fd_set  = client_fd_set;


        /*----------------------------------
        |      RECEIVE MSG OR TIMEOUT       |
        ------------------------------------*/
        int n = select(FD_SETSIZE, &working_fd_set, NULL, NULL, &timeout); /* This is a blocking call */
        
        if (n == -1)
        {
            /* Select() Error */
            perror("ERROR IN SELECT OPERATION");
            close_and_notify();
            exit(EXIT_FAILURE);
        }
        else if (n == 0)
        {
            /* Timeout */
            printf("\nServer ping timeout! Lost connection to server.....\n");
            close_and_notify();
            break;
        }
        else if (!FD_ISSET(client_fd, &working_fd_set))
        {
            /* No received messages but no timeout or error */
            printf("\nUNKNOWN ERROR\n");
            close_and_notify();
            exit(EXIT_FAILURE);
            break;
        }

        /*-------------------------------------
        |        CHECK IF DISCONNECTED         |
        --------------------------------------*/
        if( socket_bytes_to_recv(client_fd) <= 0 )
        {
            printf("Socket Client: Received disconnect/socket error. Disconnecting from server...\n");
            close_and_notify(client_fd);
            break;
        }


        /*----------------------------------
        |     PROCESS DATA FROM SERVER      |
        ------------------------------------*/
        val = process_recv_msg(client_fd);
        if( RETURN_FAILED == val )
        {
            printf("socket client: received invalid message! ERROR: Failed to process received message!");
        }
        else if( val == RETURN_DISCONNECT )
        {
            break;
        }

    } /* while(1) */


    /*-----------------------------------
    |        SET ISRUNNING FALSE         |
    ------------------------------------*/

    pthread_mutex_lock(&mutex_isExecuting_thread);
    *((bool*)args) = false;
    pthread_mutex_unlock(&mutex_isExecuting_thread);

    return NULL;
} /* execute_thread() */

void socket_client_execute()
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

} /* socket_client_execute() */

int socket_client_send_data ( const char * data, uint data_sz )
{
    return send_data(COMMAND_NONE, data, data_sz);
} /* socket_client_send_data() */

int send_data ( const uint8_t command, const char * data, uint data_sz )
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    int returnval, n;
    char* temp;

    /*----------------------------------
    |            INITIALIZE             |
    ------------------------------------*/
    returnval = 0;

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

    /*----------------------------------
    |             SEND MSGS             |
    ------------------------------------*/
    n = socket_send_data(client_fd,temp,data_sz+COMMAND_SZ);
    if( n < 1 )
    {
        /* Disconnect client if failed to send message */
        close_and_notify();
    }
    else
    {
        /* Tally total bytes sent */
        returnval += n;
    }

    free(temp);

    return returnval;
} /* send_data */

void socket_client_quit()
{
    /*-----------------------------------
    |         CLOSE SERVER SOCKET        |
    ------------------------------------*/
    if ( RETURN_FAILED == close(client_fd) )
    {
        printf("\nFailed: socket_client_quit() failed to close socket!\n");
    }

} /* socket_client_quit() */

bool socket_client_is_executing()
{
    bool return_val;
    /*-----------------------------------
    |           GET ISRUNNING            |
    ------------------------------------*/
    pthread_mutex_lock(&mutex_isExecuting_thread);
    return_val = isRunning;
    pthread_mutex_unlock(&mutex_isExecuting_thread);
    
    return return_val;
} /* socket_client_is_executing() */

void close_and_notify()
{
    close(client_fd);

    client_fd = -1;

    if(_discont_callback != NULL )
    {
        (*_discont_callback)();
    }
}
