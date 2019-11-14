/* This file is compiled and run as the DBUS server system service for the dashcam project.
    The DBUS server handles all debus connections for the project including TCP/IP access, GPS sensor access,
    and accelerometer access. For information on the different interfaces, see the Networking/DBUS/pub_dbus_srv.h
    file in the server_introspection_xml static variable. Use the pub_dbus_*_clnt.h files for access dbus interfaces */


/*-------------------------------------
|           PRIVATE INCLUDES           |
--------------------------------------*/

#include "test_dbus__tcp_srv.h"


/*-------------------------------------
|          CALLBACK FUNCTIONS          |
--------------------------------------*/

/**
 * When a dbus client of this dbus server calls the DBUS_TCP_SEND_MSG method on this dbus server, 
 * we extract the method parameters and call the dbus_srv__tcp_send_msg_callback. This callback is
 * implemented by whomever is setting up this dbus server and will call the appropiate tcp/socket server
 * function to send the message over the socket */
// typedef bool (*dbus_srv__tcp_send_msg_callback)(const char* tcp_clnt_uuid, const char* data, unsigned int data_sz);


/** Note that data is freed after this callback is called. As such, if 
 * the data in data needs to be saved, a copy of the data must be made.
 * Refer to this guide on mixing C and C++ callbacks due to ldashcam_tcp_dbus_srv
 *  being written in C: https://isocpp.org/wiki/faq/mixing-c-and-cpp */
bool tcp_msg_to_tx_callback(const char* tcp_clnt_uuid, const char* data, unsigned int data_sz)
{
  	printf("\n****************tcp_msg_to_tx_callback: callback activated.****************\n\n");

    printf("Received %d bytes from uuid %s as follows:\n\"",data_sz, tcp_clnt_uuid);
    for (size_t i = 0; i < data_sz; i++)
    {
        printf("%c",data[i]);
    }
    printf("\"\n");

    return true;
    
  	printf("\n****************END---tcp_msg_to_tx_callback---END****************\n\n");
} /* tcp_msg_to_tx_callback() */

uint32_t dbus_method__request_clients_callback( char** client_list )
{
    static char clients[] = "client_uuid client_uuid client_uuid"; /* Statics used as workaround to mem leak because we can't free the data given in client_list */
    static uint8_t count = -1;
    count++;

    /* Alternate returning either NULL and 0, or fake 3 clients */
    if(count%2 == 0)
    {
        *client_list = NULL;
        return 0;
    }
    else
    {
        *client_list = &clients[0];
        return strlen(clients)+1;
    }
} /* dbus_method__request_clients_callback() */

char* dbus_method__get_client_ip_callback( const char* clnt_uuid )
{
    static char fake_clnt_ip[] = "client_ip_addr"; /* Statics used as workaround to mem leak because we can't free the data given in clnt_uuid */
    static uint8_t count = -1;
    count++;

    assert(clnt_uuid != NULL);

    /* Alternate returning either NULL and 0, or fake 3 clients */
    if(count%2 == 0)
    {
        return NULL;
    }
    else
    {
        return fake_clnt_ip;
    }
} /* dbus_method__get_client_ip_callback() */

bool dbus_method__is_tcp__connected_to_tcp_srv_callback()
{
    static uint8_t count = -1;
    count++;

    /* Alternate returning either NULL and 0, or fake 3 clients */
    if(count%2 == 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

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

    if ( tcp_dbus_srv_init(srv_id, tcp_msg_to_tx_callback, dbus_method__request_clients_callback, dbus_method__get_client_ip_callback, dbus_method__is_tcp__connected_to_tcp_srv_callback) == EXIT_FAILURE )
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
            if ( 0 != tcp_dbus_srv_emit_msg_recv_signal(srv_id,"fakeuuid", "TCP\0 has received a message and emitted a signal.\n", 51) )
            {
                printf("ERROR: raising signal tcp_dbus_srv_emit_msg_recv_signal() FAILED!\n");
            }
            if ( 0 != tcp_dbus_srv_emit_connect_signal(srv_id, "TCP client uuid") )
            {
                printf("ERROR: raising signal tcp_dbus_srv_emit_connect_signal() FAILED!\n");
            }
            if ( 0 != tcp_dbus_srv_emit_disconnect_signal(srv_id, "TCP client uuid") )
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
