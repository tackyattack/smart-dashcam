/*-------------------------------------
|               INCLUDES               |
--------------------------------------*/
#include "prv_socket_client.h"
#include "pub_socket_client.h"
#include "../../../debug_print_defines.h"


/*-------------------------------------
|           STATIC VARIABLES           |
--------------------------------------*/
static int  client_fd                                  = -1;    /* Stores the socket_fd for our connection to the server */
static char UUID[UUID_STR_LEN]                         = {0};   /* Stores our UUID including termination char */
static bool isRunning                                  = false; /* Set true to execute the client execute thread */
static socket_lib_clnt_rx_msg _rx_callback             = NULL;  /* Callback called when a message is received from the server */
static socket_lib_clnt_disconnected _discont_callback  = NULL;  /* Callback called when we've disconnected from the server */


/*-------------------------------------
|           PRIVATE MUTEXES            |
--------------------------------------*/
static pthread_mutex_t mutex_isExecuting_thread = PTHREAD_MUTEX_INITIALIZER;


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
    info_print("CLIENT: UUID is %s\n",UUID);

    /*----------------------------------
    |             CLEAN-UP              |
    ------------------------------------*/
    fclose(fptr);

} /* load_uuid() */

int send_uuid()
{
    info_print("CLIENT: Send UUID to server\n");

    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    int sent_bytes;

    /*----------------------------------
    |             SEND UUID             |
    ------------------------------------*/
    sent_bytes = send_data(COMMAND_UUID, UUID, UUID_STR_LEN);   /* Send UUID with null termination to server to identify ourselves */
    if(sent_bytes != UUID_STR_LEN)
    {
        err_print("\nERROR: CLIENT: failed to send UUID correctly to server. Send %d bytes but expected %d bytes...\n\n", sent_bytes, UUID_STR_LEN);
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
    |          LOAD UUID IF NEEDED         |
    --------------------------------------*/
    if( UUID[0] == '\0' )
    {
        load_uuid();
    }

    /*-------------------------------------
    |             VERIFICATION             |
    --------------------------------------*/
    if ( atoi(port) < 0 || atoi(port) > 65535 )
    {
        err_print("ERROR: CLIENT: invalid port number %s!",port);
        return RETURN_FAILED;
    }

    /* Info print */
    info_print ("\nCLIENT: Attempt to open socket to server....\n");

    client_fd = socket_create_socket(port, DEFAULT_SOCKET_TYPE, server_addr, SOCKET_OWNER_IS_CLIENT);

    if(client_fd < 0)
    {
        return RETURN_FAILED;
    }

    /*-------------------------------------
    |         SEND UUID AND VERIFY         |
    --------------------------------------*/
    if( send_uuid() != UUID_STR_LEN )
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
    struct socket_msg_struct* msg;           /* Message struct pointer that will contain received message */   
    struct socket_msg_struct* msg_previous;  /* Used to free previous msg struct while looping */
    enum SOCKET_TX_RX_FLAGS recv_flag;       /* Used to check returned flag from socket_receive_data() */
    int return_val;                          /* Stores this function's return value */

    /*-------------------------------------
    |           INITIALIZATIONS            |
    --------------------------------------*/
    msg = NULL;
    msg_previous = NULL;
    return_val = RETURN_SUCCESS;

    /*----------------------------------
    |     RECEIVE DATA FROM SERVER      |
    ------------------------------------*/
    recv_flag = socket_receive_data(socket_fd, &msg);

    /*-------------------------------------
    |             VERIFICATION             |
    --------------------------------------*/
    if( recv_flag == FLAG_DISCONNECT )
    {
        info_print("CLIENT: Received disconnect/socket error. Disconnecting from server...\n");
        close_and_notify();
        return FLAG_DISCONNECT;
    }
    else if (recv_flag == FLAG_HEADER_ERROR)
    {
        warning_print("CLIENT: Received invalid msg header...ignoring\n");
        return RETURN_FAILED;
    }

    /*-----------------------------------
    |      HANDLE RECEIVED COMMANDS      |
    ------------------------------------*/
    while ( msg != NULL )
    {
        /* Print header received */
        // info_print("CLIENT: msg command received: 0x%02x with flag %d\n", msg->command, msg->recv_flag);
        /* Print data received */
        // info_print("CLIENT: received %d data bytes\n", msg->data_sz);
        // for (int i = 0; i < msg->data_sz; i++)
        // {
        //     info_print("%c", msg->data[i]);
        // }
        // info_print("\n\n");

        /* Handle msg command */
        switch (msg->command)
        {
        case COMMAND_PING:
        case COMMAND_UUID:
            return_val = send_uuid();
            break;
        case COMMAND_NONE:
            /*----------------------------------
            |           CALL CALLBACK           |
            ------------------------------------*/
            /* Received message from client. Call callback with message and UUID */
            // info_print("CLIENT: Call recv msg callback\n");
            if(_rx_callback != NULL && msg->data_sz > 0)
            {
                (*_rx_callback)( msg->data, msg->data_sz );
            }
            break;
        default:
            warning_print("CLIENT: received message without valid COMMAND\n");

            return_val = RETURN_FAILED;
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
            err_print("ERROR: CLIENT: ERROR IN SELECT() OPERATION\n");
            close_and_notify();
            exit(EXIT_FAILURE);
        }
        else if (n == 0)
        {
            /* Timeout */
            warning_print("\nCLIENT: Server ping timeout! Lost connection to server.....\n");
            close_and_notify();
            break;
        }
        else if (!FD_ISSET(client_fd, &working_fd_set))
        {
            /* No received messages but no timeout or error */
            err_print("\nERROR: CLIENT: UNKNOWN ERROR\n");
            close_and_notify();
            exit(EXIT_FAILURE);
            break;
        }

        /*----------------------------------
        |     PROCESS DATA FROM SERVER      |
        ------------------------------------*/
        val = process_recv_msg(client_fd);
        if( val == FLAG_DISCONNECT )
        {
            info_print("CLIENT: Got disconnect flag....stopping thread execution\n");
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
        err_print("\nERROR: CLIENT: Failed to create client execute thread!\n");
        exit(EXIT_FAILURE);
    }

} /* socket_client_execute() */

int socket_client_send_data ( const char * data, uint data_sz )
{
    return send_data(COMMAND_NONE, data, data_sz);
} /* socket_client_send_data() */

int send_data ( const uint8_t command, const char * data, uint data_sz )
{
    assert( (data == NULL && data_sz == 0) || (data != NULL && data_sz != 0) );

    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    int returnval, n;

    /*----------------------------------
    |            INITIALIZE             |
    ------------------------------------*/
    returnval = RETURN_SUCCESS;

    /*-------------------------------------
    |         SEND COMMAND + DATA          |
    --------------------------------------*/
    n = socket_send_data(client_fd, data, data_sz, command);

    /*-------------------------------------
    |             VERIFICATION             |
    --------------------------------------*/
    if( n < 0 )
    {
        info_print("CLIENT: Failed to send data in send_data()...Closing socket\n");
        /* Disconnect client if failed to send message */
        close_and_notify();
        returnval = RETURN_FAILED;
    }
    else
    {
        /* Tally total bytes sent */
        returnval += n;
    }

    return returnval;

} /* send_data() */

void socket_client_quit()
{
    /*-----------------------------------
    |         CLOSE SERVER SOCKET        |
    ------------------------------------*/
    if ( RETURN_FAILED == close(client_fd) )
    {
        err_print("\nERROR: CLIENT: socket_client_quit(): failed to close socket!\n");
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
} /* close_and_notify() */
