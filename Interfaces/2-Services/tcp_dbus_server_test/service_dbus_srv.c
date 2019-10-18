/* This file is compiled and run as the DBUS server system service for the dashcam project.
    The DBUS server handles all debus connections for the project including TCP/IP access, GPS sensor access,
    and accelerometer access. For information on the different interfaces, see the Networking/DBUS/pub_dbus_srv.h
    file in the server_introspection_xml static variable. Use the pub_dbus_*_clnt.h files for access dbus interfaces */


/*-------------------------------------
|           PRIVATE INCLUDES           |
--------------------------------------*/

#include "service_dbus_srv.h"


/*-------------------------------------
|          CALLBACK FUNCTIONS          |
--------------------------------------*/

/** Note that data is freed after this callback is called. As such, if 
 * the data in data needs to be saved, a copy of the data must be made.
 * Refer to this guide on mixing C and C++ callbacks due to ldashcam_tcp_dbus_srv
 *  being written in C: https://isocpp.org/wiki/faq/mixing-c-and-cpp */
void tcp_msg_to_tx_callback(char *data, unsigned int data_sz)
{
  	printf("\n****************tcp_msg_to_tx_callback: callback activated.****************\n\n");

    printf("Received %d bytes as follows:\n\"",data_sz);
    for (size_t i = 0; i < data_sz; i++)
    {
        printf("%c",data[i]);
    }
    printf("\"\n");
    
  	printf("\n****************END---tcp_msg_to_tx_callback---END****************\n\n");
} /* tcp_msg_to_tx_callback() */


/*-------------------------------------
|     MAIN FUNCTION OF THE SERVICE     |
--------------------------------------*/

int main(void)
{
    /*-------------------------------------
    |              VARIABLES               |
    --------------------------------------*/
    dbus_srv_id srv_id;

    
    /*-------------------------------------
    |            CREATE SERVER             |
    --------------------------------------*/

    srv_id = tcp_dbus_srv_create();

    printf("DBUS Server service v%s\n", srv_sftw_version);

    /*-------------------------------------
    |           START THE SERVER           |
    --------------------------------------*/

    if ( tcp_dbus_srv_init(srv_id, tcp_msg_to_tx_callback) == EXIT_FAILURE )
    {
        printf("Failed to initialize server!\nExiting.....\n");
        exit(EXIT_FAILURE);
    }

    if ( tcp_dbus_srv_execute(srv_id) == EXIT_FAILURE )
    {
        printf("Failed to execute server!\nExiting.....\n");
        exit(EXIT_FAILURE);
    }


    /*-------------------------------------
    |            INFINITE LOOP             |
    --------------------------------------*/
    
    do{
        for (size_t i = 0; i < 5; i++)
        // for ( ; ; )
        {
            printf("Emit signals\n");
            if ( 0 != tcp_dbus_srv_emit_msg_recv_signal(srv_id, "TCP\0 has received a message and emitted a signal.\n", 51) )
            {
                printf("ERROR: raising signal tcp_dbus_srv_emit_msg_recv_signal() FAILED!\n");
            }
            if ( 0 != tcp_dbus_srv_emit_connect_signal(srv_id, "TCP\0 client has connected.\n", 28) )
            {
                printf("ERROR: raising signal tcp_dbus_srv_emit_connect_signal() FAILED!\n");
            }
            if ( 0 != tcp_dbus_srv_emit_disconnect_signal(srv_id, "TCP\0 client has disconnected.\n", 31) )
            {
                printf("ERROR: raising signal tcp_dbus_srv_emit_disconnect_signal() FAILED!\n");
            }
            // sleep(1);
        }
        printf("\n\nEnter 'c' to run loop again, else press enter to quit server.\n\n");
    }while( getchar() == 'c' && getchar() == '\n' );

    /* Block until input is received. 
        Once input has been received, quit the program */
    // printf("Press enter to quit DBUS server.");
    // getchar();


    /*-------------------------------------
    |           STOP THE SERVER            |
    --------------------------------------*/

    tcp_dbus_srv_kill(srv_id);


    /*-------------------------------------
    |          DELETE THE SERVER           |
    --------------------------------------*/

    tcp_dbus_srv_delete(srv_id);

    return EXIT_SUCCESS;
} 
