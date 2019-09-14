#include "../comm_tcp.h"

#include <unistd.h> /* Needed for close() */

#define SERVER_ADDR  "192.168.200.1"                            /* Address of the server we are conencting to. Note, this can be a IP or hostname */
// #define SERVER_ADDR  "raspberrypi"                              /* Address of the server we are conencting to. Note, this can be a IP or hostname */
#define TIME_BETWEEN_CONNECTION_ATTEMPTS (2)                       /* Time in seconds between attempts to connect to server if we fail to connect */
#define SERVER_PING_TIMEOUT              (2*TIME_BETWEEN_PINGS)    /* Max time allowed before we assume the server has disconnected. If we haven't received a 
                                                                    message from the server within this amount of time (s), disconnect and assume failure */

/* Static variables */
static int client_fd = -1;
static char UUID[UUID_SZ] = {0};


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

    /* Verify proper number of arguments and set port number for Server to listen on
        Note that argc = 1 means no arguments*/
    if (argc < 2)
    {
        printf("WARNING, no port provided, defaulting to %s\n", DEFAULT_PORT);

        //No port number provided, use default
        // strcpy(port, (const char*)DEFAULT_PORT);
        port = (char*)DEFAULT_PORT;
        // port = DEFAULT_PORT;
    }
    else if (argc > 2)
    {
        fprintf(stderr, "ERROR, too many arguments!\n 0 or 1 arguments expected. Expected port number!\n");
        exit(EXIT_FAILURE);
    }
    else //1 argument
    {
        if ( atoi(argv[1]) < 0 || atoi(argv[1]) > 65535 )
        {
            printf("ERROR: invalid port number %s!",argv[1]);
            exit(EXIT_FAILURE);
        }
        //Get port number of server from the arguments
        port = argv[1];
    }

    return port;
} /* check_parameters() */

/* Reads UUID from file. Returns -1 if failed. str should be at least UUID_SZ+1 */
void load_uuid()
{
    FILE *fptr;
    char buffer[UUID_SZ]; /* Defined at compile time */

    fptr = fopen(UUID_FILE_NAME,"r"); /* UUID_FILE_NAME if defined at compile time */
    assert(fptr != NULL);

    assert(fgets(buffer,UUID_SZ,fptr) != NULL); /* get uuid string from file */

    memcpy(UUID,buffer,UUID_SZ); /* Copy data */

    fclose(fptr);
}

int send_uuid()
{
    char temp[UUID_SZ+1]; /* UUID_SZ + 1 for command byte */
    int sent_bytes;

    temp[0] = COMMAND_UUID; /* Set command byte */
    memcpy(&temp[sizeof(COMMAND_UUID)],UUID,UUID_SZ);
    
    sent_bytes = send_data(client_fd, temp, UUID_SZ+sizeof(COMMAND_UUID));   /* Send UUID plus UUID command to server to identify ourselves */
    if(sent_bytes != UUID_SZ+sizeof(COMMAND_UUID))
    {
        printf("ERROR: failed to send UUID correctly to server...\n");
    }
    return sent_bytes;
}

/* returns socket or -1 if failed */
int client_init(char *port)
{
    /* Loop until successful connection with server */
    do
    {
        /* Info print */
        printf ("\nAttempt to open socket to server....\n");

        sleep(TIME_BETWEEN_CONNECTION_ATTEMPTS);
        client_fd = make_socket(port, DEFAULT_SOCKET_TYPE, SERVER_ADDR, IS_CLIENT);
    } while (client_fd < 0);
    

    return send_uuid();
}


void process_recv_msg(const char* buffer, const int buffer_sz)
{
    if(buffer_sz)
    /* if server has pinged us */
    if (buffer[0] == COMMAND_PING)
    {
        send_uuid(client_fd);
    }

    /* Info print */
    printf("Received message from server: \"%s\"\n",buffer);

}

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
        |    WAIT UP TO 5s FOR DATA RECV    |
        ------------------------------------*/
        int n = select(FD_SETSIZE, &working_fd_set, NULL, NULL, &timeout);
        // assert(n==-1);
        
        if (n==-1)
        {
            perror("ERROR IN SELECT OPERATION");
            close(client_fd);
            exit(EXIT_FAILURE);
        }
        else if (n == 0)
        {
            printf("\nServer ping timeout. Lost server connection.....\n");
            close(client_fd);
            break;//timeout
        }
        else if (!FD_ISSET(client_fd, &working_fd_set))
        {
            printf("\nUNKNOWN ERROR\n");
            close(client_fd);
            break;//again something wrong
        }


        /*----------------------------------
        |     RECEIVE DATA FROM SERVER      |
        ------------------------------------*/

        /* receive is a blocking call, but we know from using select that there is data to be read */
        n_recv_bytes = receive_data(client_fd,buffer,MAX_MSG_SZ);
        if (n_recv_bytes < 0)
        {
            printf("\nClient received invalid msg.....Close connection to server....\n");
            close(client_fd);
            return;
        }
        
        process_recv_msg(buffer,n_recv_bytes);
        
        /* Info print TODO remove this */
        printf ("Received message \"%s\" from server.\n", buffer);

    } /* while(1) */

} /* execute_client */

int main(int argc, char *argv[])
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    char* port = check_parameters(argc, argv);
    
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
}
