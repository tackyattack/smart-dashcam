/*-------------------------------------
|           PRIVATE INCLUDES           |
--------------------------------------*/

#include "tcp_client_service.h"


/*-------------------------------------
|           STATIC VARIABLES           |
--------------------------------------*/
static dbus_srv_id srv_id;     /* dbus server instance id */
static char* host_ip   = SERVER_ADDR;
static char* host_port = SERVER_PORT;
static bool ignore_disconnect = false;
static pthread_mutex_t mutex_tcp_recv_msg = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_ignore_disconnect = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_dbus_method_send_tcp_msg_callback = PTHREAD_MUTEX_INITIALIZER;


/*-------------------------------------
|          CALLBACK FUNCTIONS          |
--------------------------------------*/

/* https://isocpp.org/wiki/faq/mixing-c-and-cpp */

// From <dashcam_sockets/pub_socket_client.h>
// /* https://isocpp.org/wiki/faq/mixing-c-and-cpp */
// typedef void (*socket_lib_clnt_rx_msg)(const char* data, const unsigned int data_sz);
// typedef void (*socket_lib_clnt_disconnected)(void);

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
 /* This callback is data received over TCP from the server */
void tcp_recv_msg(const char *data, const unsigned int data_sz)
{
  	printf("\n****************recv_msg: callback activated.****************\n\n");

    printf("Received %d data bytes as follows:\n\"",data_sz);
    for (size_t i = 0; i < data_sz; i++)
    {
        printf("%c",data[i]);
    }
    printf("\"\n");

    pthread_mutex_lock(&mutex_tcp_recv_msg);
    if ( 0 != tcp_dbus_srv_emit_msg_recv_signal(srv_id, "SERVER", data, data_sz) )
    {
        pthread_mutex_unlock(&mutex_tcp_recv_msg);
        printf("ERROR: raising signal tcp_dbus_srv_emit_msg_recv_signal() FAILED!\n");
        exit(EXIT_FAILURE);
    }
    pthread_mutex_unlock(&mutex_tcp_recv_msg);
    
  	printf("\n****************END---recv_msg---END****************\n\n");
} /* tcp_recv_msg() */

/* This callback notifies when we have been disconnected from the server */
void tcp_disconnect()
{
  	printf("\n****************tcp_disconnect: callback activated.****************\n\n");

    pthread_mutex_lock(&mutex_ignore_disconnect);
    if(ignore_disconnect == true)
    {
        pthread_mutex_unlock(&mutex_ignore_disconnect);
        return;
    }
    pthread_mutex_unlock(&mutex_ignore_disconnect);
    
  	printf("\n****************END---tcp_disconnect---END****************\n\n");
} /* tcp_disconnect() */

/** Note that data is freed after this callback is called. As such, if 
 * the data in data needs to be saved, a copy of the data must be made.
 * Refer to this guide on mixing C and C++ callbacks due to ldashcam_tcp_dbus_srv
 *  being written in C: https://isocpp.org/wiki/faq/mixing-c-and-cpp */
 /* This is a DBUS callback to us with message data to be sent over tcp */
bool dbus_method_send_tcp_msg_callback(const char* tcp_clnt_uuid, const char* data, unsigned int data_sz)
{
  	printf("\n****************dbus_method_send_tcp_msg_callback: callback activated.****************\n\n");

    if (tcp_clnt_uuid != NULL && tcp_clnt_uuid[0] != '\0')
    {
        printf("Client ID should be null or == \\0. Current it is %s \n",tcp_clnt_uuid);
    }

    if( socket_client_is_executing() == false )
    {
        return false;
    }

    pthread_mutex_lock(&mutex_dbus_method_send_tcp_msg_callback);
    if( 0 >= socket_client_send_data(data, data_sz) )
    {
        pthread_mutex_unlock(&mutex_dbus_method_send_tcp_msg_callback);
        return false;
    }
    pthread_mutex_unlock(&mutex_dbus_method_send_tcp_msg_callback);

    return true;

  	printf("\n****************END---dbus_method_send_tcp_msg_callback---END****************\n\n");
} /* dbus_method_send_tcp_msg_callback() */


/*--------------------------------------
|     MAIN FUNCTIONS OF THE SERVICE     |
---------------------------------------*/

void check_parameters(int argc, char *argv[])
{
    /*----------------------------------
    |          CHECK ARGUMENTS          |
    ------------------------------------*/
   /* Verify proper number of arguments and set port number for server to listen on
        Note that argc = 1 means no arguments */
    if (argc < 2)
    {
        printf("WARNING, no arguments provided. Defaulting to ip/hostname %s and port number %s!\n", SERVER_ADDR, SERVER_PORT);
        host_port = SERVER_PORT;
        host_ip   = SERVER_ADDR;
    }
    else if (argc > 3)
    {
        fprintf(stderr, "ERROR, too many arguments!\n 0, or 2 arguments expected. Expected ip/hostname and port number!\n");
        exit(EXIT_FAILURE);
    }
    else /* 2 arguments, ip and port */
    {
        if ( atoi(argv[2]) < 0 || atoi(argv[2]) > 65535 )
        {
            printf("ERROR: invalid port number %s!",argv[2]);
            exit(EXIT_FAILURE);
        }
        host_ip   = argv[1];
        host_port = argv[2];
    }
} /* check_parameters() */

// For testing only
int main(int argc, char *argv[])
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/

    /*----------------------------------
    |            CHECK INPUT            |
    ------------------------------------*/
    check_parameters(argc, argv);

    /*-------------------------------------
    |          START DBUS SERVER           |
    --------------------------------------*/
    srv_id = tcp_dbus_srv_create();
    if ( tcp_dbus_srv_init(srv_id, dbus_method_send_tcp_msg_callback) == EXIT_FAILURE )
    {
        printf("Failed to initialize DBUS server!\nExiting.....\n");
        exit(EXIT_FAILURE);
    }
    if ( tcp_dbus_srv_execute(srv_id) == EXIT_FAILURE )
    {
        printf("Failed to execute server!\nExiting.....\n");
        exit(EXIT_FAILURE);
    }

    /*-------------------------------------
    |   START TCP SERVER AND RUN IN LOOP   |
    --------------------------------------*/
    while(1)
    {
        if( socket_client_is_executing() == false && socket_client_init(host_ip, host_port, tcp_recv_msg, tcp_disconnect) == -1 )
        {
            printf("Failed to initialize TCP server!\nRetrying.....\n\n");
            usleep(200000);
            continue;
        }

        /* Run the client. Returns immediately after spawning server. */
        socket_client_execute(); /* Shouldn't return unless lost connection */

        while(socket_client_is_executing() == true)
        {
            usleep(200000);
        }
            
        printf("\nFailed: lost tcp server connection!\n\n");
    }


    printf("--------------------ERROR, TCP SERVER SERVICE IS EXITING UNEXPECTEDLY!!!--------------------");


    /*-------------------------------------
    |           STOP THE SERVERS           |
    --------------------------------------*/
    tcp_dbus_srv_kill(srv_id);

    pthread_mutex_lock(&mutex_ignore_disconnect);
    ignore_disconnect = true;
    pthread_mutex_unlock(&mutex_ignore_disconnect);
    
    socket_client_quit();
    /* Kill client */
    if( true == socket_client_is_executing())
    {
        socket_client_quit();
        sleep(2);
    }


    /*-------------------------------------
    |        DELETE THE DBUS SERVER        |
    --------------------------------------*/
    tcp_dbus_srv_delete(srv_id);

    /* If we ever return from socket_server_execute, there was a serious error */
    return 1;
} /* main() */
