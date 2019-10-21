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

/** Note that data is freed after this callback is called. As such, if 
 * the data in data needs to be saved, a copy of the data must be made.
 * Refer to this guide on mixing C and C++ callbacks due to ldashcam_tcp_dbus_clnt
 *  being written in C: https://isocpp.org/wiki/faq/mixing-c-and-cpp */
void tcp_rx_data_callback(char* data, unsigned int data_sz)
{
  	printf("\n****************tcp_rx_data_callback: callback activated.****************\n\n");

    printf("Received %d bytes as follows:\n\"",data_sz);
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
void tcp_clnt_connect_callback(char* clnt_uuid, unsigned int clnt_uuid_sz)
{
  	printf("\n****************tcp_clnt_connect_callback: callback activated.****************\n\n");

    printf("Client uuid is %d bytes with uuid \"",clnt_uuid_sz);
    for (size_t i = 0; i < clnt_uuid_sz; i++)
    {
        printf("%c",clnt_uuid[i]);
    }
    printf("\"\n");
    
  	printf("\n****************END---tcp_clnt_connect_callback---END****************\n\n");
} /* tcp_clnt_connect_callback() */

/** Note that data is freed after this callback is called. As such, if 
 * the data in data needs to be saved, a copy of the data must be made.
 * Refer to this guide on mixing C and C++ callbacks due to ldashcam_tcp_dbus_clnt
 *  being written in C: https://isocpp.org/wiki/faq/mixing-c-and-cpp */
void tcp_clnt_disconnect_callback(char* clnt_uuid, unsigned int clnt_uuid_sz)
{
  	printf("\n****************tcp_clnt_disconnect_callback: callback activated.****************\n\n");

    printf("Client uuid is %d bytes with uuid \"",clnt_uuid_sz);
    for (size_t i = 0; i < clnt_uuid_sz; i++)
    {
        printf("%c",clnt_uuid[i]);
    }
    printf("\"\n");
    
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
        else if ( val == DBUS_SRV_NOT_EXECUTING )
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
	tcp_dbus_send_msg( id,msg, msg_sz );

    printf("Subscribe to DBUS_TCP_RECV_SIGNAL signal\n");
    if ( EXIT_FAILURE == tcp_dbus_client_Subscribe2Recv( id, DBUS_TCP_RECV_SIGNAL, &tcp_rx_data_callback) )
    {
        printf("Failed to subscribe to DBUS_TCP_RECV_SIGNAL signal!\nExiting.....\n");
        exit(EXIT_FAILURE);
    }

    printf("Call send TCP message method (tcp_dbus_send_msg())\n");
	tcp_dbus_send_msg( id,msg, msg_sz );
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

    for (size_t i = 0; i < 5; i++)
    {
        sleep(1);
        /* Test server methods */
        printf("Call send TCP message method (tcp_dbus_send_msg())\n");
        tcp_dbus_send_msg( id,msg, msg_sz );
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
