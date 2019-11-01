/*-------------------------------------
|           PRIVATE INCLUDES           |
--------------------------------------*/

#include "tcp_server_service.h"


/*-------------------------------------
|           STATIC VARIABLES           |
--------------------------------------*/
static dbus_srv_id srv_id;     /* dbus server instance id */
static pthread_mutex_t mutex_tcp_recv_msg = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_connect = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_disconnect = PTHREAD_MUTEX_INITIALIZER;


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
        printf("ERROR: raising signal tcp_dbus_srv_emit_msg_recv_signal() FAILED!\n");
        exit(EXIT_FAILURE);
    }
    pthread_mutex_unlock(&mutex_tcp_recv_msg);
    
  	// printf("\n****************END---recv_msg---END****************\n\n");
} /* recv_msg() */

/* This callback notifies when a tcp client connects */
void tcp_client_connect(const char* uuid)
{
  	// printf("\n****************client_connect: callback activated.****************\n\n");

    // printf("Client connected -> UUID: %s\n", uuid);
    pthread_mutex_lock(&mutex_connect);
    if ( 0 != tcp_dbus_srv_emit_connect_signal(srv_id, uuid) )
    {
        pthread_mutex_unlock(&mutex_connect);
        printf("ERROR: raising signal tcp_dbus_srv_emit_connect_signal() FAILED!\n");
        exit(EXIT_FAILURE);
    }
    pthread_mutex_unlock(&mutex_connect);

  	// printf("\n****************END---client_connect---END****************\n\n");
} /* client_connect() */

/* This callback notifies when a tcp client disconnects */
void tcp_client_disconnect(const char* uuid)
{
  	// printf("\n****************client_disconnect: callback activated.****************\n\n");

    // printf("Client disconnected -> UUID: %s\n", uuid);
    pthread_mutex_lock(&mutex_disconnect);
    if ( 0 != tcp_dbus_srv_emit_connect_signal(srv_id, uuid) )
    {
        pthread_mutex_unlock(&mutex_disconnect);
        printf("ERROR: raising signal tcp_dbus_srv_emit_connect_signal() FAILED!\n");
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
        if( 0 >= socket_server_send_data_all(data, data_sz) )
        {
            return false;
        }
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
