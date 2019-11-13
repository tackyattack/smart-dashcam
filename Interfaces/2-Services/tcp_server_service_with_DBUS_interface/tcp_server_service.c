/*-------------------------------------
|           PRIVATE INCLUDES           |
--------------------------------------*/

#include "tcp_server_service.h"

/*-------------------------------------
|           PRIVATE STRUCTS            |
--------------------------------------*/
struct clients_struct
{
    char* uuid;
    char* ip_addr;
    struct clients_struct* next;
};


/*-------------------------------------
|           STATIC VARIABLES           |
--------------------------------------*/
static dbus_srv_id srv_id;     /* dbus server instance id */
static pthread_mutex_t mutex_tcp_recv_msg = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_connect = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_disconnect = PTHREAD_MUTEX_INITIALIZER;

static struct clients_struct* clients = NULL; /* Linked list of all connected clients updated by connect and disconnect callbacks */
static pthread_mutex_t mutex_clients_struct = PTHREAD_MUTEX_INITIALIZER; /* Mutex lock for thread safety for clients */

/*-------------------------------------
|       CLIENTS_STRUCT FUNCTIONS       |
--------------------------------------*/
void add_client( const char* uuid, const char* ip_addr )
{
    /*-------------------------------------
    |              VARIABLES               |
    --------------------------------------*/
    struct clients_struct* new, *temp;
    
    /*-------------------------------------
    |            VERIFICATIONS             |
    --------------------------------------*/
    if( uuid == NULL || ip_addr == NULL )
    {
        printf("SERVICE: WARNING: Client that connected has no uuid or ip address");
        exit(EXIT_FAILURE);
    }

    /*-------------------------------------
    |       ALLOCATE AND COPY MEMORY       |
    --------------------------------------*/
    new = malloc(sizeof(struct clients_struct));
    new->uuid = malloc(strlen(uuid)+1);
    new->ip_addr = malloc(strlen(ip_addr)+1);
    new->next = NULL;

    memcpy(new->uuid,uuid,strlen(uuid)+1);
    memcpy(new->ip_addr,ip_addr,strlen(ip_addr)+1);

    /*-------------------------------------
    |    ADD NEW CLIENT TO LINKED LIST     |
    --------------------------------------*/
    pthread_mutex_lock(&mutex_clients_struct);

    if(clients == NULL) /* If there are no clients yet */
    {
        clients = new;
        pthread_mutex_unlock(&mutex_clients_struct);
        return;
    }

    /* Iterate to the end of the linked list */
    for ( temp = clients; temp->next != NULL; temp = temp->next ); /* Find last element in linked list of clients */

    /* Add client to end of linked list */
    temp->next = new;
    
    pthread_mutex_unlock(&mutex_clients_struct);
} /* add_client() */

/* Returns NULL if not found. Must lock and unlock mutex_clients_struct prior/after calling this function */
struct clients_struct* get_client(const char* uuid)
{
    struct clients_struct* temp;

    for ( temp = clients; (temp != NULL && strcmp(uuid,temp->uuid) != 0); temp = temp->next ); /* Find element in linked list of clients with matching uuid */

    return temp;
}

void remove_client(const char* uuid)
{
    /*-------------------------------------
    |              VARIABLES               |
    --------------------------------------*/
    struct clients_struct* crnt, *previous;

    /*-------------------------------------
    |            VERIFICATIONS             |
    --------------------------------------*/
    pthread_mutex_lock(&mutex_clients_struct);
    if( clients == NULL || uuid == NULL )
    {
        pthread_mutex_unlock(&mutex_clients_struct);
        return;
    }

    /*-------------------------------------
    |           INITIALIZATIONS            |
    --------------------------------------*/
    previous = NULL;

    /*-------------------------------------
    |             FIND CLIENT              |
    --------------------------------------*/
    for ( crnt = clients; (crnt != NULL && strcmp(uuid,crnt->uuid) != 0); crnt = crnt->next ) /* Find element in linked list of clients with matching uuid */
    {
        printf("SERVICE: remove_client(): search for uuid %s. crnt uuid: %s\n",uuid, crnt->uuid);
        previous = crnt;
    }

    /*-------------------------------------
    |            VERIFICATIONS             |
    --------------------------------------*/
    if (crnt == NULL) /* client not found in list */
    {
        pthread_mutex_unlock(&mutex_clients_struct);
        return;
    }

    /*-------------------------------------
    |         REMOVE CLIENT STRUCT         |
    --------------------------------------*/
    if (previous == NULL) /* element is first item in list */
    {
        clients = clients->next;
    }
    else if (previous != NULL)
    {
        previous->next = crnt->next;
    }

    pthread_mutex_unlock(&mutex_clients_struct);

    /*-------------------------------------
    |               CLEANUP                |
    --------------------------------------*/
    free(crnt->uuid);
    free(crnt->ip_addr);
    free(crnt);
    crnt = NULL;

} /* remove_client() */

/*-------------------------------------
|          CALLBACK FUNCTIONS          |
--------------------------------------*/

// From the pub_socket_server.h
// /* https://isocpp.org/wiki/faq/mixing-c-and-cpp */
// typedef void (*socket_lib_srv_rx_msg)(const char* uuid, const char* data, const unsigned int data_sz);
// typedef void (*socket_lib_srv_connected)(const char* uuid);
// typedef void (*socket_lib_srv_disconnected)(const char* uuid);

// /**
//  * When a dbus client of this dbus server calls the DBUS_TCP_SEND_MSG method on this dbus server, 
//  * we extract the method parameters and call the dbus_srv__tcp_send_msg_callback. This callback is
//  * implemented by whomever is setting up this dbus server and will call the appropiate tcp/socket server
//  * function to send the message over the socket */
// typedef bool (*dbus_srv__tcp_send_msg_callback)(const char* tcp_clnt_uuid, const char* data, unsigned int data_sz);

/** Note that data is freed after this callback is called. As such, if 
 * the data in data needs to be saved, a copy of the data must be made.
 * Refer to this guide on mixing C and C++ callbacks due to library
 *  being written in C: https://isocpp.org/wiki/faq/mixing-c-and-cpp */
 /* This callback is data received over TCP and needs to be sent to DBUS clients */
void tcp_recv_msg(const char* uuid, const char *data, const unsigned int data_sz)
{
  	// printf("\n****************recv_msg: callback activated.****************\n\n");

    // printf("Message from the following UUID: %s\n", uuid);
    // printf("Received %d data bytes as follows:\n\"",data_sz);
    // for (size_t i = 0; i < data_sz; i++)
    // {
    //     printf("%c",data[i]);
    // }
    // printf("\"\n");
    pthread_mutex_lock(&mutex_tcp_recv_msg);
    if ( 0 != tcp_dbus_srv_emit_msg_recv_signal(srv_id, uuid, data, data_sz) )
    {
        pthread_mutex_unlock(&mutex_tcp_recv_msg);
        printf("SERVICE: ERROR: raising signal tcp_dbus_srv_emit_msg_recv_signal() FAILED!\n");
        exit(EXIT_FAILURE);
    }
    pthread_mutex_unlock(&mutex_tcp_recv_msg);
    
  	// printf("\n****************END---SERVICE: recv_msg---END****************\n\n");
} /* tcp_recv_msg() */

/* This callback notifies when a tcp client connects */
void tcp_client_connect(const char* uuid, const char* ip_addr)
{
  	// printf("\n****************SERVICE: client_connect: callback activated.****************\n\n");

    /* UPDATE OUR INTERNAL LIST OF CLIENTS */
    add_client(uuid,ip_addr);

    /* EMIT SIGNAL TO ALL SUBSCRIBERS THAT CLIENT HAS CONNECTED */
    printf("Client connected -> UUID: %s\n\tIP address: %s\n", uuid, ip_addr);
    pthread_mutex_lock(&mutex_connect);
    if ( 0 != tcp_dbus_srv_emit_connect_signal(srv_id, uuid) )
    {
        pthread_mutex_unlock(&mutex_connect);
        printf("SERVICE: ERROR: raising signal tcp_dbus_srv_emit_connect_signal() FAILED!\n");
        exit(EXIT_FAILURE);
    }
    pthread_mutex_unlock(&mutex_connect);

  	// printf("\n****************END---SERVICE: client_connect---END****************\n\n");
} /* tcp_client_connect() */

/* This callback notifies when a tcp client disconnects */
void tcp_client_disconnect(const char* uuid)
{
  	// printf("\n****************SERVICE: client_disconnect: callback activated.****************\n\n");

    /* UPDATE OUR INTERNAL LIST OF CLIENTS */
    remove_client(uuid);

    /* EMIT SIGNAL TO ALL SUBSCRIBERS THAT CLIENT HAS CONNECTED */
    printf("Client disconnected -> UUID: %s\n", uuid);
    pthread_mutex_lock(&mutex_disconnect);
    if ( 0 != tcp_dbus_srv_emit_disconnect_signal(srv_id, uuid) )
    {
        pthread_mutex_unlock(&mutex_disconnect);
        printf("ERROR: raising signal tcp_dbus_srv_emit_disconnect_signal() FAILED!\n");
        exit(EXIT_FAILURE);
    }
    pthread_mutex_unlock(&mutex_disconnect);

  	// printf("\n****************END---client_disconnect---END****************\n\n");
} /* client_disconnect() */

/** Note that data is freed after this callback is called. As such, if 
 * the data in data needs to be saved, a copy of the data must be made.
 * Refer to this guide on mixing C and C++ callbacks due to ldashcam_tcp_dbus_srv
 *  being written in C: https://isocpp.org/wiki/faq/mixing-c-and-cpp */
 /* This is a DBUS callback to us with message data to be sent over tcp */
bool dbus_method_send_tcp_msg_callback(const char* tcp_clnt_uuid, const char* data, unsigned int data_sz)
{
  	// printf("\n****************tcp_msg_to_tx_callback: callback activated.****************\n\n");

    // printf("Received %d bytes as follows:\n\"",data_sz);
    // for (size_t i = 0; i < data_sz; i++)
    // {
    //     printf("%c",data[i]);
    // }
    // printf("\"\n");

    if( tcp_clnt_uuid == NULL || strlen(tcp_clnt_uuid) == 0 )
    {
        socket_server_send_data_all(data, data_sz);
    }
    else
    {
        if( 0 >= socket_server_send_data(tcp_clnt_uuid, data, data_sz) )
        {
            return false;
        }
    }

    return true;

  	// printf("\n****************END---tcp_msg_to_tx_callback---END****************\n\n");
} /* tcp_msg_to_tx_callback() */

/**
 * Given a pointer to a 2D array that is set NULL, will set the pointer to an array of client uuid's
 * the size (or number of elements/clients) is returned. client_list is expected to be NULL and will
 * be NULL if no clients are connected.

 * Returns the number of clients connected (which is the number of clients in the 2D array client_list)
 */
uint16_t dbus_method__request_clients_callback( char*** client_list )
{
    /*-------------------------------------
    |              VARIABLES               |
    --------------------------------------*/
    // char** clients;
    struct clients_struct* iter;
    uint16_t num_clients;

    /*-------------------------------------
    |       PARAMETER VERIFICATIONS        |
    --------------------------------------*/
    if( client_list != NULL )
    {
        printf("SERVICE: WARNING: dbus_method_request_clients_callback(): Someone request list of clients but gives a non null pointer parameter!\n");
    }

    /*-------------------------------------
    |           INITIALIZATIONS            |
    --------------------------------------*/
    num_clients = 0;
    client_list = NULL;

    /*-------------------------------------
    |            VERIFICATIONS             |
    --------------------------------------*/
    pthread_mutex_lock(&mutex_clients_struct);

    if(clients == NULL) /* No clients connected */
    {
        pthread_mutex_unlock(&mutex_clients_struct);
        return 0;
    }

    /*-------------------------------------
    |           GET NUM CLIENTS            |
    --------------------------------------*/
    for ( iter = clients; (iter != NULL); iter = iter->next )
    {
        num_clients += 1;
    }

    /*-------------------------------------
    |  ALLOCATE MEMORY AND COPY IP ADDRS   |
    --------------------------------------*/
    *client_list = malloc(num_clients);

    num_clients = 0;
    for ( iter = clients; (iter != NULL); iter = iter->next )
    {
        (*client_list)[num_clients] = malloc( strlen(iter->ip_addr)+1 );

        memcpy( (*client_list)[num_clients], iter->ip_addr, strlen(iter->ip_addr)+1 );

        num_clients += 1;
    }

    pthread_mutex_unlock(&mutex_clients_struct);

    return num_clients;
    // Keep internal list of connected clients and return copy of them
    // implement dbus
}

//TODO implement dbus
char* dbus_method__get_client_ip_callback( const char* clnt_uuid )
{
    /*-------------------------------------
    |              VARIABLES               |
    --------------------------------------*/
    char* ip_addr;
    struct clients_struct* client;

    pthread_mutex_lock(&mutex_clients_struct);

    client = get_client(clnt_uuid);

    ip_addr = malloc( strlen(client->ip_addr) + 1 );
    memcpy(ip_addr, client->ip_addr, strlen(client->ip_addr) + 1);

    pthread_mutex_lock(&mutex_clients_struct);

    return ip_addr;
}

/*--------------------------------------
|     MAIN FUNCTIONS OF THE SERVICE     |
---------------------------------------*/

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
        printf("WARNING, no port provided, defaulting to %s\n", "5555");

        /* No port number provided, use default */
        port = (char*)"5555";
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

// For testing only
int main(int argc, char *argv[])
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    char* port;             /* Port number for socket server */

    /*----------------------------------
    |            CHECK INPUT            |
    ------------------------------------*/
    port = check_parameters(argc, argv);


    /*-----------------------------------
    |         INITIALIZE SERVERS         |
    ------------------------------------*/
    srv_id = tcp_dbus_srv_create();
    if ( tcp_dbus_srv_init(srv_id, dbus_method_send_tcp_msg_callback) == EXIT_FAILURE )
    {
        printf("Failed to initialize DBUS server!\nExiting.....\n");
        exit(EXIT_FAILURE);
    }

    if( 0 > socket_server_init(port, tcp_recv_msg, tcp_client_connect, tcp_client_disconnect) )
    {
        printf("Failed to initialize TCP server!\nExiting.....\n");
        exit(EXIT_FAILURE);
    }

    /*-------------------------------------
    |           EXECUTE SERVERS            |
    --------------------------------------*/
    if ( tcp_dbus_srv_execute(srv_id) == EXIT_FAILURE )
    {
        printf("Failed to execute server!\nExiting.....\n");
        exit(EXIT_FAILURE);
    }

    /* Run the server.  Accept connection requests, 
        and receive messages. Run forever. */
    socket_server_execute();

    if(socket_server_is_executing() == false)
    {
        printf("Failed: server not executing\n");
        exit(EXIT_FAILURE);
    }


    /*-------------------------------------
    |     INFINITE LOOP UNLESS FAILURE     |
    --------------------------------------*/
    while(socket_server_is_executing() == true)
    {
        sleep(1);
    }

    printf("--------------------ERROR, TCP SERVER SERVICE IS EXITING UNEXPECTEDLY!!!--------------------");


    /*-------------------------------------
    |           STOP THE SERVERS           |
    --------------------------------------*/
    socket_server_quit();
    sleep(2);
    tcp_dbus_srv_kill(srv_id);


    /*-------------------------------------
    |        DELETE THE DBUS SERVER        |
    --------------------------------------*/
    tcp_dbus_srv_delete(srv_id);

    /* If we ever return from socket_server_execute, there was a serious error */
    return 1;
} /* main() */
