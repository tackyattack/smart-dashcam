#include <stdio.h>
#include <stdlib.h>

#include <errno.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <assert.h>
#include <time.h>
#include  <signal.h>

#include "pub_socket_commons.h" /* From static library */

#define MAX_PENDING_CONNECTIONS  (5)        /* 5 is a standard value for the max backlogged connection requests */
#define SERVER_ADDR              NULL       /* Set this value to a string that is the IP address, hostname or the server you're creating or set to NULL (0) to use this machines address (NULL recommended) */
#define BUFFER_SZ                (1024)     /* Size of the buffer we use to send/receive data */
#define SELECT_TIMEOUT_TIME      (100)      /* How long Select() will block if no message is received */

struct client_info
{
    int fd;
    char uuid[UUID_SZ+1]; /* +1 for termination char */
    struct in_addr address;
    time_t lastPing;
    struct client_info *next;
};

/*----------------------------------
|         STATIC VARIABLES          |
------------------------------------*/
static int server_socket_fd             = -1;       /* The socket file descriptor for the server (the local machine) */      
static struct client_info *client_infos = NULL;     /* struct pointer to struct that is a linked list of structs that contain info on the clients */
static fd_set active_fd_set             = {0};      /*  */

/* Handle signal interupt (ctrl + c) */
void  INThandler(int sig)
{
    signal(sig, SIG_IGN);
    printf("Ctrl-C detected. Exiting....\n");
    close(server_socket_fd);
    exit(EXIT_SUCCESS);
}

/* Handle signal pipe (pipe error when sending data to 
    client. Error signifies client disconnected) */
void  PIPEhandler(int sig)
{
    signal(sig, SIG_IGN);
    printf("PIPE Error detected. A client must have disconnected....\n");


    signal(SIGPIPE, PIPEhandler);
}

/* Return the pointer to the client_info struct containing
    the desired socket fd number. Returns NULL if not found */
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

/* Return the pointer to the client_info struct containing
    the desired UUID. Returns NULL if not found */
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

        /* No port number provided, use default */
        port = (char*)DEFAULT_PORT;
    }
    else if (argc > 2)
    {
        fprintf(stderr, "ERROR, too many arguments!\n 0 or 1 arguments expected. Expected port number!\n");
        exit(EXIT_FAILURE);
    }
    else /* 1 argument */
    {
        /* Test that argument is valid */
        if ( atoi(argv[1]) < 0 || atoi(argv[1]) > 65535 )
        {
            printf("ERROR: invalid port number %s!",argv[1]);
            exit(EXIT_FAILURE);
        }

        /* Get port number of server from the arguments */
        port = argv[1];
    }

    return port;
} /* check_parameters() */

/* Initialize the server and return the server's server_socket_fd */
int init_server(char* port)
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    int server_socket_fd;      /* generic socket variable */


    /*----------------------------------
    |       CREATE SERVER SOCKET        |
    ------------------------------------*/

    /* Info print */
    printf("Creating server on port %s\n", port);
    server_socket_fd = make_socket(port, DEFAULT_SOCKET_TYPE, (const char*)SERVER_ADDR, IS_SOCKET_SERVER);

    /*----------------------------------
    |       VERIFY SERVER CREATED       |
    ------------------------------------*/
    assert(server_socket_fd >= 0);
    

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

/* Handles accepting incoming connections 
   -returns the new clients file descriptor */
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
        return -1;
    }


    /*----------------------------------
    |        SET/ADD CLIENT INFO        |
    ------------------------------------*/
    new_client = malloc(sizeof(struct client_info));
    assert(new_client!=NULL);
    new_client->fd = new_client_fd;
    new_client->address = new_client_info.sin_addr;
    new_client->lastPing = time(NULL);
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

/* Close client socker_fd and remove client from 
    client_infos struct and active_fd_set */
void close_conn(int client_fd)
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

} /* close_conn() */

/* Receive message from client specificed in client_fd and process the message. 
    -Returns number of bytes received */
int process_recv_msg(int client_fd)
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    int n_recv_bytes;                      /* Number of received bytes */
    char buffer[BUFFER_SZ];                /* Buffer for send/receive of data. Extra byte for termination */
    struct client_info *client;

    /*----------------------------------
    |            INITIALIZE             |
    ------------------------------------*/
    client = NULL;
    bzero(buffer,BUFFER_SZ);

    /* Receive data from client. Returns number of bytes received. */
    n_recv_bytes = receive_data( client_fd, buffer, BUFFER_SZ );
    
    if(n_recv_bytes < 0)
    {
        printf("Failed to receive data from client.");
        return n_recv_bytes;
    }

    /*----------------------------------
    |       HANDLE RECEIVED UUID        |
    ------------------------------------*/
    if (buffer[0] == COMMAND_UUID && n_recv_bytes >= (int)sizeof(COMMAND_PING)+UUID_SZ)
    {
        client = find_client_by_fd(client_fd);
        assert(client != NULL);
        /* client wasn't found in our list. There is a discrepancy. */
        memcpy(client->uuid,&buffer[1],UUID_SZ);
    }
    

    /* Info prints. Data read. */    
    fprintf(stderr, "Server: received %d bytes with message: \"%s\"\n", n_recv_bytes, buffer);

    putchar('0');
    putchar('x');
    for (int i = 0; i < n_recv_bytes; i++) {
        printf("%02x ", buffer[i]);
    }
    putchar('\n');
    putchar('\n');

    return n_recv_bytes;
} /* process_recv_msg */

int process_dbus()
{

    return 0;
} /* process_dbus */

/* Sends a message to all clients */
int send_to_all(const char* buffer, const int buffer_sz)
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    struct client_info *client;
    int returnval, n;

    /*----------------------------------
    |            INITIALIZE             |
    ------------------------------------*/
    client = NULL;
    returnval = 0;


    /*----------------------------------
    |            SEND PINGS             |
    ------------------------------------*/
    for(client=client_infos; client!=NULL; client=client->next)
    {
        n = send_data(client->fd,buffer,buffer_sz);
        if( n < 1 )
        {
            /* Disconnect client if failed to send message */
            close_conn(client->fd);
        }
        else
        {
            /* Tally total bytes sent */
            returnval += n;
        }
        
    } /* for */

    return returnval;
} /* send_to_all */

/* Check if message has been received from any clients and
    handles receiving/processing any messages */
void service_sockets(const fd_set *read_fd_set)
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    int i;                                      /* generic i used in for loop */


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
                if (process_recv_msg(i) < 1)
                {
                    close_conn(i);
                }
            } /* else */

        } /* if (FD_ISSET) */

    } /* For loop */

} /* service_sockets */

void execute_server()
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    fd_set active_fd_set, read_fd_set;          /* generic file descriptor set for sockets */
    struct timeval timeout;                     /* Used to set select() timeout time */
    int select_return;                          /* Return value of select() */
    time_t lastPing;                            /* Last time we sent pings */
    char pingCommand[sizeof(COMMAND_PING)];   /* Create string containing ping command */


    /*----------------------------------
    |            INITIALIZE             |
    ------------------------------------*/
    /* Initialize the set of active sockets. */
    FD_ZERO(&active_fd_set);
    FD_SET(server_socket_fd, &active_fd_set);
    lastPing = 0;


    /*----------------------------------
    |           INFINITE LOOP           |
    ------------------------------------*/
    while (1)
    {
        /*----------------------------------
        |     ITERATION INITIALIZATION      |
        ------------------------------------*/
        timeout.tv_sec  = 0;// SERVER_PING_TIMEOUT; /*  */
        timeout.tv_usec = SELECT_TIMEOUT_TIME*10000; /* Set select() to block for SELECT_TIMEOUT_TIME ms */
        read_fd_set = active_fd_set;

        /*----------------------------------
        |     BLOCK UNTIL RECV OR TIMEOUT   |
        ------------------------------------*/
        select_return = select(FD_SETSIZE, &read_fd_set, NULL, NULL, &timeout);
        if ( select_return < 0)
        {
            fprintf (stderr, "errno = %d ", errno);
            perror("select");
            continue;
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
            printf("Sending Ping to clients\n");

            bzero(pingCommand,sizeof(COMMAND_PING));
            pingCommand[0] = COMMAND_PING;

            send_to_all(pingCommand, sizeof(COMMAND_PING));
            lastPing = time(NULL);
        }

    } /* while (1) */

    free(pingCommand);

} /* execute_server() */

int main(int argc, char *argv[])
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    char* port;                              /* Port number for socket server */
    
    /*----------------------------------
    |       SETUP SIGNAL HANDLERS       |
    ------------------------------------*/
    struct sigaction act_sigint,act_sigpipe;
    act_sigint.sa_handler = INThandler;
    sigaction(SIGINT, &act_sigint, NULL);
    act_sigpipe.sa_handler = PIPEhandler;
    sigaction(SIGPIPE, &act_sigpipe, NULL);


    /*----------------------------------
    |            CHECK INPUT            |
    ------------------------------------*/
    port = check_parameters(argc, argv);


    /*----------------------------------
    |            START SERVER           |
    ------------------------------------*/
    server_socket_fd = init_server(port);

    /* Run the server.  Accept connection requests, 
        and receive messages. Run forever. */
    execute_server();

    /* If we ever return from execute_server, there was a serious error */
    return 1;
} /* main() */
