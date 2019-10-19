/* This file is compiled and run as the DBUS server system service for the dashcam project.
    The DBUS server handles all debus connections for the project including TCP/IP access, GPS sensor access,
    and accelerometer access. For information on the different interfaces, see the Networking/DBUS/pub_dbus_srv.h
    file in the server_introspection_xml static variable. Use the pub_dbus_*_clnt.h files for access dbus interfaces */


/*-------------------------------------
|           PRIVATE INCLUDES           |
--------------------------------------*/

#include "test_socket_clnt.h"


/*-------------------------------------
|     MAIN FUNCTION OF THE SERVICE     |
--------------------------------------*/


/*-------------------------------------
|              CALLBACKS               |
--------------------------------------*/

// From <dashcam_sockets/pub_socket_client.h>
// /* https://isocpp.org/wiki/faq/mixing-c-and-cpp */
// typedef void (*socket_lib_clnt_rx_msg)(const char* data, const unsigned int data_sz);
// typedef void (*socket_lib_clnt_disconnected)(void);

/** Note that data is freed after this callback is called. As such, if 
 * the data in data needs to be saved, a copy of the data must be made.
 * Refer to this guide on mixing C and C++ callbacks due to ldashcam_tcp_dbus_clnt
 *  being written in C: https://isocpp.org/wiki/faq/mixing-c-and-cpp */
void recv_msg(const char* data, const unsigned int data_sz)
{
  	printf("\n****************recv_msg: callback activated.****************\n\n");

    printf("Received %d bytes as follows:\n\"",data_sz);
    for (size_t i = 0; i < data_sz; i++)
    {
        printf("%c",data[i]);
    }
    printf("\"\n");

  	printf("\n****************END---recv_msg---END****************\n\n");
} /* recv_msg() */


void disconnected()
{
  	printf("\n****************disconnected: callback activated.****************\n\n");

    printf("Lost connection to server\n");

  	printf("\n****************END---disconnected---END****************\n\n");
} /* disconnected() */

/*-------------------------------------
|                 MAIN                 |
--------------------------------------*/
static char* host_ip   = SERVER_ADDR;
static char* host_port = SERVER_PORT;
static char  longStr[] = "Client testing very long message to server\nClient testing very long message to server\nClient testing very long message to server\nClient testing very long message to server\nClient testing very long message to server\nClient testing very long message to server\nClient testing very long message to server\nClient testing very long message to server\nClient testing very long message to server\nClient testing very long message to server\nClient testing very long message to server\nClient testing very long message to server\nClient testing very long message to server\nClient testing very long message to server\nClient testing very long message to server\nClient testing very long message to server\n";

//For testing only
char* check_parameters(int argc, char *argv[])
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/

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
    else /* 2 argument */
    {
        if ( atoi(argv[2]) < 0 || atoi(argv[2]) > 65535 )
        {
            printf("ERROR: invalid port number %s!",argv[2]);
            exit(EXIT_FAILURE);
        }
        host_port = argv[2];
    }

    return host_port;
} /* check_parameters() */


// For testing only
int main(int argc, char *argv[])
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    host_port = check_parameters(argc, argv);
    uint count = 10;

    while(count != 0)
    {
        count--;

        /* This call will attempt to connect with the server infinitely */
        if( socket_client_init(host_ip, host_port, recv_msg, disconnected) == -1 )
        {
            sleep(2);
            continue;
        }

        /* Info print */
        printf ("Successfully opened socket to server \"%s\" on port %s.\n", host_ip, host_port);

        /* Main loop for client to receive/process data */
        socket_client_execute();

        for (size_t i = 0; i < 5; i++)
        {
            if( true == is_client_executing())
            {
                socket_client_send_data("Client message to server", 25);
                sleep(2);
            }
            else
            {
                break;
            }
        }

        socket_client_send_data(longStr,sizeof(longStr));


        /* Kill client */
        if( true == is_client_executing())
        {
            socket_client_quit();
            sleep(2);
            if( false == is_client_executing())
            {
                printf("Client Thread killed.\n");
            }
            break;
        }
    }

    return 0;
} /* main() */
