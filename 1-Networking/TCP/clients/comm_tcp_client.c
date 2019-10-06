#include "../comm_tcp.h"

#include <uuid/uuid.h>
#include <unistd.h>

#define SERVER_ADDR  "192.168.200.1"                               /* Address of the server we are conencting to. Note, this can be a IP or hostname */
// #define SERVER_ADDR  "raspberrypi"                              /* Address of the server we are conencting to. Note, this can be a IP or hostname */
#define TIME_BETWEEN_CONNECTION_ATTEMPTS (2)                       /* Time in seconds between attempts to connect to server if we fail to connect */
#define SERVER_PING_TIMEOUT              (2*TIME_BETWEEN_PINGS)    /* Max time allowed before we assume the server has disconnected. If we haven't received a 
                                                                        message from the server within this amount of time (s), disconnect and assume failure */

/* Static variables */
static int client_fd = -1;          /* Stores the socket_fd for our connection to the server */
static char UUID[UUID_SZ+1] = {0};  /* Stores our UUID. +1 because uuid_unparse generates a str with UUID_SZ ascii characters plus a termination char */


/* Check for value parameters and return port number */
char* check_parameters(int argc, char *argv[])
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    char* port;


    /*----------------------------------
    |          CHECK ARGUMENTS          |
    ------------------------------------*/
    /* Verify proper number of arguments and set port number for server to listen on
        Note that argc = 1 means no arguments */
    if (argc < 2)
    {
        printf("WARNING, no port provided, defaulting to %s\n", DEFAULT_PORT);

        port = (char*)DEFAULT_PORT;
    }
    else if (argc > 2)
    {
        fprintf(stderr, "ERROR, too many arguments!\n 0 or 1 arguments expected. Expected port number!\n");
        exit(EXIT_FAILURE);
    }
    else /* 1 argument */
    {
        if ( atoi(argv[1]) < 0 || atoi(argv[1]) > 65535 )
        {
            printf("ERROR: invalid port number %s!",argv[1]);
            exit(EXIT_FAILURE);
        }
        port = argv[1];
    }

    return port;
} /* check_parameters() */

/* Generate uuid and set UUID to the new uuid */
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

/* Reads UUID from file. If UUID file doesn't exist, it is created 
    -Returns -1 if failed. */
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


    /*----------------------------------
    |             CLEAN-UP              |
    ------------------------------------*/
    fclose(fptr);

} /* load_uuid() */

/* Send our UUID to server. 
    -Returns -1 if failed else number of bytes sent. */
int send_uuid()
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    char temp[UUID_SZ+1]; /* UUID_SZ + 1 for command byte */
    int sent_bytes;


    /*----------------------------------
    |          INITIALIZATIONS          |
    ------------------------------------*/
    temp[0] = COMMAND_UUID; /* Set command byte */
    memcpy(&temp[sizeof(COMMAND_UUID)],UUID,UUID_SZ);


    /*----------------------------------
    |             SEND UUID             |
    ------------------------------------*/
    sent_bytes = send_data(client_fd, temp, UUID_SZ+sizeof(COMMAND_UUID));   /* Send UUID plus UUID command to server to identify ourselves */
    if(sent_bytes != UUID_SZ+sizeof(COMMAND_UUID))
    {
        printf("ERROR: failed to send UUID correctly to server...\n");
    }

    return sent_bytes;
} /* send_uuid() */

/* Initialize client
    -returns socket or -1 if failed */
int client_init(char *port)
{
    /*----------------------------------
    |        LOOP UNTIL CONNECTED       |
    ------------------------------------*/
    do
    {
        /* Info print */
        printf ("\nAttempt to open socket to server....\n");

        sleep(TIME_BETWEEN_CONNECTION_ATTEMPTS);
        client_fd = make_socket(port, DEFAULT_SOCKET_TYPE, SERVER_ADDR, IS_CLIENT);
    } while (client_fd < 0);
    
    return send_uuid();
} /* client_init() */

/* Given a buffer of a received message from server, 
    performs actions appropiate. */
void process_recv_msg(const char* buffer, const int buffer_sz)
{
    /*----------------------------------
    |          CHECK PARAMETERS         |
    ------------------------------------*/
    if(buffer == NULL)
    {
        return;
    }


    /*----------------------------------
    |          PROCESS COMMANDS         |
    ------------------------------------*/
    if (buffer[0] == COMMAND_PING)
    {
        send_uuid(client_fd);
    }

} /* process_recv_msg() */

void execute_client()
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    char buffer[MAX_MSG_SZ];
    fd_set client_fd_set,working_fd_set;
    struct timeval timeout;
    int n_recv_bytes;


    /*----------------------------------
    |          INITIALIZATIONS          |
    ------------------------------------*/
    bzero(buffer,MAX_MSG_SZ);
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
        timeout.tv_sec  = SERVER_PING_TIMEOUT; /* If nothing received from server for 5 seconds, assume lost connection */
        timeout.tv_usec = 0;
        n_recv_bytes = 0;
        working_fd_set = client_fd_set;


        /*----------------------------------
        |      RECEIVE MSG OR TIMEOUT       |
        ------------------------------------*/
        int n = select(FD_SETSIZE, &working_fd_set, NULL, NULL, &timeout); /* This is a blocking call */
        
        if (n == -1)
        {
            /* Select() Error */
            perror("ERROR IN SELECT OPERATION");
            close(client_fd);
            exit(EXIT_FAILURE);
        }
        else if (n == 0)
        {
            /* Timeout */
            printf("\nServer ping timeout! Lost connection to server.....\n");
            close(client_fd);
            break;
        }
        else if (!FD_ISSET(client_fd, &working_fd_set))
        {
            /* No received messages but no timeout or error */
            printf("\nUNKNOWN ERROR\n");
            close(client_fd);
            break;
        }


        /*----------------------------------
        |     RECEIVE DATA FROM SERVER      |
        ------------------------------------*/
        n_recv_bytes = receive_data(client_fd,buffer,MAX_MSG_SZ);
        if (n_recv_bytes < 0)
        {
            printf("\nClient received invalid msg.....Close connection to server....\n");
            close(client_fd);
            return;
        }
        
        /*----------------------------------
        |     PROCESS DATA FROM SERVER      |
        ------------------------------------*/
        process_recv_msg(buffer,n_recv_bytes);
        
    } /* while(1) */

} /* execute_client() */

int main(int argc, char *argv[])
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    char* port = check_parameters(argc, argv);


    /*----------------------------------
    |          INITIALIZATIONS          |
    ------------------------------------*/
    load_uuid();


    while(1)
    {
        /* This call will attempt to connect with the server infinitely */
        if( client_init(port) != UUID_SZ+sizeof(COMMAND_UUID) )
        {
            close(client_fd);
            break;
        }

        /* Info print */
        printf ("Successfully opened socket to server \"%s\" on port %s.\n", SERVER_ADDR, port);
        
        /* Main loop for client to receive/process data */
        execute_client();
    }

    /* Close socket */
    close(client_fd);
    
    return 0;
} /* main() */
