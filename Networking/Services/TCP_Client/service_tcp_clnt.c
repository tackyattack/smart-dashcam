/* This file is compiled and run as the DBUS server system service for the dashcam project.
    The DBUS server handles all debus connections for the project including TCP/IP access, GPS sensor access,
    and accelerometer access. For information on the different interfaces, see the Networking/DBUS/pub_dbus_srv.h
    file in the server_introspection_xml static variable. Use the pub_dbus_*_clnt.h files for access dbus interfaces */


/*-------------------------------------
|           PRIVATE INCLUDES           |
--------------------------------------*/

#include "service_tcp_clnt.h"


/*-------------------------------------
|     MAIN FUNCTION OF THE SERVICE     |
--------------------------------------*/


/*-------------------------------------
|              CALLBACKS               |
--------------------------------------*/

// void SubscriberCallback(GDBusConnection *conn, const gchar *sender_name, const gchar *object_path, const gchar *interface_name, const gchar *signal_name, GVariant *parameters,gpointer callback_data)
// {
// 	g_printf("\n****************signal handler: OnEmitSignal received.****************\n\n");
// } /* SubscribeCallback */

 /* main(), */

/* This is for testing */

int main(void)
{
    /*-------------------------------------
    |              VARIABLES               |
    -------------------------------------*/

    
    /*-------------------------------------
    |           INITIALIZATIONS            |
    -------------------------------------*/


    if ( EXIT_FAILURE == init_client() )
    {
        printf("Failed to initialize client!\nExiting.....\n");
        exit(EXIT_FAILURE);
    }


    /*-------------------------------------
    |          CLIENT OPERATIONS           |
    -------------------------------------*/

	/* Test server methods */
	printf("Testing server interface v%s\n", server_version);
	test_Ping();
	test_Echo();
	test_CommandEmitSignal();

    printf("Subscribe to server\n");
    if ( EXIT_FAILURE == Subscribe2Server() )
    {
        printf("Failed to subscribe to server!\nExiting.....\n");
        exit(EXIT_FAILURE);
    }

	test_CommandEmitSignal( );
    sleep(2);

    printf("Unsubscribe from server\n");
    UnsubscribeFromServer();
    
    sleep(2);

    printf("Unsubscribe from server\n");
    UnsubscribeFromServer();
    
    printf("Subscribe to server\n");
    if ( EXIT_FAILURE == Subscribe2Server() )
    {
        printf("Failed to subscribe to server!\nExiting.....\n");
        exit(EXIT_FAILURE);
    }

    printf("Subscribe to server\n");
    if ( EXIT_FAILURE == Subscribe2Server() )
    {
        printf("Failed to subscribe to server!\nExiting.....\n");
    }

    for (size_t i = 0; i < 5; i++)
    {
        sleep(1);
        /* Test server methods */
	    test_Ping( );
	    test_Echo( );
        test_CommandEmitSignal( );
    }
    
    printf("Unsubscribe from server\n");
    UnsubscribeFromServer();

    printf("Press enter to quit....\n");
    getchar(); /* Block until any stdin is received */

    /* Signal to server that it needs to quit as it's no longer needed */
	// test_Quit( );


    /*-------------------------------------
    |               SHUTDOWN               |
    -------------------------------------*/
    disconnect_client();

	return 0;
}
