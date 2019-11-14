/* This file is compiled and run as the DBUS server system service for the dashcam project.
    The DBUS server handles all debus connections for the project including TCP/IP access, GPS sensor access,
    and accelerometer access. For information on the different interfaces, see the Networking/DBUS/pub_dbus_srv.h
    file in the server_introspection_xml static variable. Use the pub_dbus_*_clnt.h files for access dbus interfaces */


/*-------------------------------------
|           PRIVATE INCLUDES           |
--------------------------------------*/

#include "test_dbus__tcp_clnt.h"


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

/** Note that data is freed after this callback is called. As such, if 
 * the data in data needs to be saved, a copy of the data must be made.
 * Refer to this guide on mixing C and C++ callbacks due to ldashcam_tcp_dbus_clnt
 *  being written in C: https://isocpp.org/wiki/faq/mixing-c-and-cpp */
void tcp_clnt_connect_callback(const char* tcp_clnt_uuid, const char* data, unsigned int data_sz)
{
  	printf("\n****************tcp_clnt_connect_callback: callback activated.****************\n\n");

    if(data != NULL || data_sz != 0)
    {
        printf("ERROR: data is not null or data_sz is not 0!");
    }

    printf("Client uuid is \"%s\"\n",tcp_clnt_uuid);
    
  	printf("\n****************END---tcp_clnt_connect_callback---END****************\n\n");
} /* tcp_clnt_connect_callback() */

/** Note that data is freed after this callback is called. As such, if 
 * the data in data needs to be saved, a copy of the data must be made.
 * Refer to this guide on mixing C and C++ callbacks due to ldashcam_tcp_dbus_clnt
 *  being written in C: https://isocpp.org/wiki/faq/mixing-c-and-cpp */
void tcp_clnt_disconnect_callback(const char* tcp_clnt_uuid, const char* data, unsigned int data_sz)
{
  	printf("\n****************tcp_clnt_disconnect_callback: callback activated.****************\n\n");

    if(data != NULL || data_sz != 0)
    {
        printf("ERROR: data is not null or data_sz is not 0!");
    }

    printf("Client uuid is \"%s\"\n",tcp_clnt_uuid);
    
  	printf("\n****************END---tcp_clnt_disconnect_callback---END****************\n\n");
} /* tcp_clnt_disconnect_callback() */


/*-------------------------------------
|                 MAIN                 |
--------------------------------------*/

int main(void)
{
    /*-------------------------------------
    |              VARIABLES               |
    -------------------------------------*/
    dbus_clnt_id id;
    char msg[] = "Message to send over\0TCP\n";
    uint msg_sz = sizeof(msg);
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
    
    printf("Call send TCP message method (tcp_dbus_send_msg())\n");
	tcp_dbus_send_msg( id,"fakeuuid",msg, msg_sz );

    printf("Subscribe to DBUS_TCP_RECV_SIGNAL signal\n");
    if ( EXIT_FAILURE == tcp_dbus_client_Subscribe2Recv( id, DBUS_TCP_RECV_SIGNAL, &tcp_rx_data_callback) )
    {
        printf("Failed to subscribe to DBUS_TCP_RECV_SIGNAL signal!\nExiting.....\n");
        exit(EXIT_FAILURE);
    }

    printf("Call send TCP message method (tcp_dbus_send_msg()) with NULL UUID\n");
	tcp_dbus_send_msg( id, NULL,msg, msg_sz );
    sleep(2);

    printf("Unsubscribe from DBUS_TCP_RECV_SIGNAL signal\n");
    tcp_dbus_client_UnsubscribeRecv(id,DBUS_TCP_RECV_SIGNAL);
    
    sleep(2);

    printf("Unsubscribe from DBUS_TCP_RECV_SIGNAL signal\n");
    tcp_dbus_client_UnsubscribeRecv(id,DBUS_TCP_RECV_SIGNAL);

    printf("Subscribe to DBUS_TCP_RECV_SIGNAL signal\n");
    if ( EXIT_FAILURE == tcp_dbus_client_Subscribe2Recv(id,DBUS_TCP_RECV_SIGNAL,&tcp_rx_data_callback) )
    {
        printf("Failed to subscribe to DBUS_TCP_RECV_SIGNAL signal!\nExiting.....\n");
        exit(EXIT_FAILURE);
    }

    printf("Subscribe to DBUS_TCP_RECV_SIGNAL signal\n");
    if ( EXIT_FAILURE != tcp_dbus_client_Subscribe2Recv(id,DBUS_TCP_RECV_SIGNAL,&tcp_rx_data_callback) )
    {
        printf("Failed to subscribe to DBUS_TCP_RECV_SIGNAL signal!\n \tEXPECTED THIS\n");
        exit(EXIT_FAILURE);
    }
    printf("Subscribe to DBUS_TCP_CONNECT_SIGNAL signal\n");
    if ( EXIT_FAILURE == tcp_dbus_client_Subscribe2Recv(id,DBUS_TCP_CONNECT_SIGNAL,&tcp_clnt_connect_callback) )
    {
        printf("Failed to subscribe to DBUS_TCP_CONNECT_SIGNAL signal!\nExiting.....\n");
        exit(EXIT_FAILURE);
    }
    printf("Subscribe to DBUS_TCP_DISCONNECT_SIGNAL signal\n");
    if ( EXIT_FAILURE == tcp_dbus_client_Subscribe2Recv(id,DBUS_TCP_DISCONNECT_SIGNAL,&tcp_clnt_disconnect_callback) )
    {
        printf("Failed to subscribe to DBUS_TCP_DISCONNECT_SIGNAL signal!\nExiting.....\n");
        exit(EXIT_FAILURE);
    }


    printf("\n\nTest methods\n\n");


    printf("Method 1\n");
    /* Test is_connected functionality for tcp clients */
    if ( true != tcp_dbus_connected_to_tcp_srv(id) )
    {
        printf("tcp_dbus_connected_to_tcp_srv failed: expected true but got false!\nExiting.....\n");
        exit(EXIT_FAILURE);
    }
    printf("Method 2\n");
    if ( false != tcp_dbus_connected_to_tcp_srv(id) )
    {
        printf("tcp_dbus_connected_to_tcp_srv failed: expected false but got true!\nExiting.....\n");
        exit(EXIT_FAILURE);
    }

    printf("Method 3\n");
    /* Test functionality for getting a client's IP address from it's UUID */
    if ( NULL != tcp_dbus_get_client_ip(id, "client_uuid") )
    {
        printf("tcp_dbus_get_client_ip failed: expected NULL!\nExiting.....\n");
        exit(EXIT_FAILURE);
    }
    printf("Method 4\n");
    if ( 0 != strcmp ( "client_ip_addr", tcp_dbus_get_client_ip(id, "client_uuid")) )
    {
        printf("tcp_dbus_get_client_ip failed: expected \"client_ip_addr\"!\nExiting.....\n");
        exit(EXIT_FAILURE);
    }

    printf("Method 5\n");
    char **test = NULL;
    uint16_t num = 0;
    num = tcp_dbus_get_connected_clients(id,&test);
    /* Test functionality for getting a client's IP address from it's UUID */
    if ( test != NULL || num != 0 )
    {
        printf("tcp_dbus_get_connected_clients failed! Expected NULL and 0, but got num = %u.\nExiting.....\n", num);
        exit(EXIT_FAILURE);
    }

    printf("Method 6\n");
    test = NULL;
    num = 0;
    num = tcp_dbus_get_connected_clients(id,&test);
    if ( num != 3 || test == NULL || 0 != strcmp("client_uuid", test[0]) || 0 != strcmp("client_uuid", test[1]) || 0 != strcmp("client_uuid", test[2]) )
    {
        printf("tcp_dbus_get_connected_clients failed!\nExiting.....\n");
        exit(EXIT_FAILURE);
    }

    printf("Loop test send_msg method\n");
    for (size_t i = 0; i < 5; i++)
    {
        sleep(1);
        /* Test server methods */
        printf("Call send TCP message method (tcp_dbus_send_msg())\n");
        tcp_dbus_send_msg( id,"fakeuuid",msg, msg_sz );

    }

    printf("Unsubscribe from signals\n");
    tcp_dbus_client_UnsubscribeRecv(id,DBUS_TCP_RECV_SIGNAL);
    tcp_dbus_client_UnsubscribeRecv(id,DBUS_TCP_CONNECT_SIGNAL);
    tcp_dbus_client_UnsubscribeRecv(id,DBUS_TCP_DISCONNECT_SIGNAL);

    // sleep(10);

    printf("Subscribe to DBUS_TCP_RECV_SIGNAL signal\n");
    if ( EXIT_FAILURE == tcp_dbus_client_Subscribe2Recv(id,DBUS_TCP_RECV_SIGNAL,&tcp_rx_data_callback) )
    {
        printf("Failed to subscribe to DBUS_TCP_RECV_SIGNAL signal!\n \tEXPECTED THIS\n");
    }
    printf("Subscribe to DBUS_TCP_CONNECT_SIGNAL signal\n");
    if ( EXIT_FAILURE == tcp_dbus_client_Subscribe2Recv(id,DBUS_TCP_CONNECT_SIGNAL,&tcp_clnt_connect_callback) )
    {
        printf("Failed to subscribe to DBUS_TCP_CONNECT_SIGNAL signal!\nExiting.....\n");
        EXIT_FAILURE;
    }
    printf("Subscribe to DBUS_TCP_DISCONNECT_SIGNAL signal\n");
    if ( EXIT_FAILURE == tcp_dbus_client_Subscribe2Recv(id,DBUS_TCP_DISCONNECT_SIGNAL,&tcp_clnt_disconnect_callback) )
    {
        printf("Failed to subscribe to DBUS_TCP_DISCONNECT_SIGNAL signal!\nExiting.....\n");
        EXIT_FAILURE;
    }

    printf("Press enter to quit....\n");
    getchar(); /* Block until any stdin is received */

    /* Signal to server that it needs to quit as it's no longer needed */
	// test_Quit( );


    /*-------------------------------------
    |               SHUTDOWN               |
    -------------------------------------*/
    tcp_dbus_client_disconnect(id);
    tcp_dbus_client_delete(id);

	return 0;
}
