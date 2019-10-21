/* This file is compiled and run as the DBUS server system service for the dashcam project.
    The DBUS server handles all debus connections for the project including TCP/IP access, GPS sensor access,
    and accelerometer access. For information on the different interfaces, see the Networking/DBUS/pub_dbus_srv.h
    file in the server_introspection_xml static variable. Use the pub_dbus_*_clnt.h files for access dbus interfaces */


/*-------------------------------------
|           PRIVATE INCLUDES           |
--------------------------------------*/

#include "command_control.h"


/*-------------------------------------
|     MAIN FUNCTION OF THE SERVICE     |
--------------------------------------*/


/*-------------------------------------
|              CALLBACKS               |
--------------------------------------*/

/**
 * This is the universal signal received callback used when a signal is received from dbus server.
 * Use this function prototype when subscribing to signals using tcp_dbus_client_Subscribe2Recv().
 * 
 * When a socket message is received, the dbus server emits signal DBUS_TCP_RECV_SIGNAL. We catch 
 * that signal and call this callback with the socket message sender's uuid, data array, and the
 * size of the data array. tcp_clnt_uuid is a null terminated string, data contains the data received
 * from the tcp client, and data_sz is the size of data.
 * 
 * When a tcp server client connects or disconects, the dbus server emits signal DBUS_TCP_CONNECT_SIGNAL,
 * or DBUS_TCP_DISCONNECT_SIGNAL. We catch these signals after having subscribed to them and when one
 * is caught, this callback is called with the uuid (tcp_clnt_uuid) of the client that
 * connected/disconnected. tcp_clnt_uuid is a null terminated string, data == NULL, and data_sz == 0.
 */
//  typedef void (*tcp_rx_signal_callback)(const char* tcp_clnt_uuid, const char* data, unsigned int data_sz);

/** Note that data is freed after this callback is called. As such, if 
 * the data in data needs to be saved, a copy of the data must be made.
 * Refer to this guide on mixing C and C++ callbacks due to ldashcam_tcp_dbus_clnt
 *  being written in C: https://isocpp.org/wiki/faq/mixing-c-and-cpp */
void tcp_rx_data_callback(const char* tcp_clnt_uuid, const char* data, unsigned int data_sz)
{
  	printf("\n****************tcp_rx_data_callback: callback activated.****************\n\n");

    printf("Received %d bytes from client %s as follows:\n\"",data_sz, tcp_clnt_uuid);
    for (size_t i = 0; i < data_sz; i++)
    {
        printf("%c",data[i]);
    }
    printf("\"\n");
    
  	printf("\n****************END---tcp_rx_data_callback---END****************\n\n");
} /* tcp_rx_data_callback() */

void process_recv_data(char* data, uint data_sz)
{
    //Process commands here to do things
}

/*-------------------------------------
|                 MAIN                 |
--------------------------------------*/

int main(void)
{
    /*-------------------------------------
    |              VARIABLES               |
    -------------------------------------*/
    dbus_clnt_id id;
    int val;
    const char* server_version;

    /*-------------------------------------
    |           INITIALIZATIONS            |
    -------------------------------------*/

    id = tcp_dbus_client_create();

    do
    {
        val = tcp_dbus_client_init(id, &server_version);
        if ( EXIT_FAILURE == val )
        {
            printf("Failed to initialize client!\nExiting.....\n");
            exit(EXIT_FAILURE);
        }
        else if ( val == DBUS_SRV_NOT_AVAILABLE )
        {
            printf("WARNING: DBUS server is unavailable. Trying again in 2 seconds...\n");
            sleep(2);
        }
    }while( EXIT_SUCCESS != val );


    /*-------------------------------------
    |          CLIENT OPERATIONS           |
    -------------------------------------*/

	printf("Testing server interface v%s\n", server_version);
    
    printf("Subscribe to DBUS_TCP_RECV_SIGNAL signal\n");
    if ( EXIT_FAILURE == tcp_dbus_client_Subscribe2Recv( id, (char*)DBUS_TCP_RECV_SIGNAL, &tcp_rx_data_callback) )
    {
        printf("Failed to subscribe to DBUS_TCP_RECV_SIGNAL signal!\nExiting.....\n");
        exit(EXIT_FAILURE);
    }

    // printf("Subscribe to DBUS_TCP_CONNECT_SIGNAL signal\n");
    // if ( EXIT_FAILURE == tcp_dbus_client_Subscribe2Recv(id,DBUS_TCP_CONNECT_SIGNAL,&tcp_clnt_connect_callback) )
    // {
    //     printf("Failed to subscribe to DBUS_TCP_CONNECT_SIGNAL signal!\nExiting.....\n");
    //     EXIT_FAILURE;
    // }
    // printf("Subscribe to DBUS_TCP_DISCONNECT_SIGNAL signal\n");
    // if ( EXIT_FAILURE == tcp_dbus_client_Subscribe2Recv(id,DBUS_TCP_DISCONNECT_SIGNAL,&tcp_clnt_disconnect_callback) )
    // {
    //     printf("Failed to subscribe to DBUS_TCP_DISCONNECT_SIGNAL signal!\nExiting.....\n");
    //     EXIT_FAILURE;
    // }


    /*-------------------------------------
    |            INFINITE LOOP             |
    --------------------------------------*/

    while(1){ sleep(10); }

    printf("Unsubscribe from signals\n");
    tcp_dbus_client_UnsubscribeRecv(id,(char*)DBUS_TCP_RECV_SIGNAL);
    tcp_dbus_client_UnsubscribeRecv(id,(char*)DBUS_TCP_CONNECT_SIGNAL);
    tcp_dbus_client_UnsubscribeRecv(id,(char*)DBUS_TCP_DISCONNECT_SIGNAL);

    // sleep(10);


    /*-------------------------------------
    |               SHUTDOWN               |
    -------------------------------------*/

    tcp_dbus_client_disconnect(id);
    tcp_dbus_client_delete(id);

	return 0;
}
