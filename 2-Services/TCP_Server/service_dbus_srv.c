/* This file is compiled and run as the DBUS server system service for the dashcam project.
    The DBUS server handles all debus connections for the project including TCP/IP access, GPS sensor access,
    and accelerometer access. For information on the different interfaces, see the Networking/DBUS/pub_dbus_srv.h
    file in the server_introspection_xml static variable. Use the pub_dbus_*_clnt.h files for access dbus interfaces */


/*-------------------------------------
|           PRIVATE INCLUDES           |
--------------------------------------*/

#include "service_dbus_srv.h"


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

    srv_id = create_server();

    printf("DBUS Server service v%s\n", srv_sftw_version);

    /*-------------------------------------
    |           START THE SERVER           |
    --------------------------------------*/

    if ( init_server(srv_id) == EXIT_FAILURE )
    {
        printf("Failed to initialize server!\nExiting.....\n");
        exit(EXIT_FAILURE);
    }

    if ( execute_server(srv_id) == EXIT_FAILURE )
    {
        printf("Failed to execute server!\nExiting.....\n");
        exit(EXIT_FAILURE);
    }


    /*-------------------------------------
    |            INFINITE LOOP             |
    --------------------------------------*/
    
    /* Block until input is received. 
        Once input has been received, quit the program */
    printf("Press enter to quit DBUS server.");
    getchar();


    /*-------------------------------------
    |           STOP THE SERVER            |
    --------------------------------------*/

    kill_server(srv_id);


    /*-------------------------------------
    |          DELETE THE SERVER           |
    --------------------------------------*/

    delete_server(srv_id);

    return EXIT_SUCCESS;
} 
