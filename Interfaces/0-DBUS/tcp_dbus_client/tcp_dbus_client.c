/*
 * This uses the recommended GLib API for D-Bus: GDBus,
 * which has been distributed with GLib since version 2.26.
 *
 * For details of how to use GDBus, see:
 * https://developer.gnome.org/gio/stable/gdbus-convenience.html
 *
 * dbus-glib also exists but is deprecated.
 */

/*-------------------------------------
|           PRIVATE INCLUDES           |
--------------------------------------*/

#include "../pub_dbus.h"
#include "pub_tcp_dbus_clnt.h"
#include "prv_tcp_dbus_clnt.h"
#include "../../debug_print_defines.h"


/*-------------------------------------
|    DEFINITIONS OF DBUS FUNCTIONS     |
|      IMPLEMENTED BY THE SERVER       |
--------------------------------------*/

int tcp_dbus_send_msg(dbus_clnt_id clnt_id, const char* tcp_clnt_uuid, char* data, uint data_sz)
{
    /*-------------------------------------
    |              VARIABLES               |
    --------------------------------------*/

    bool send_status;
    GVariant *gvar;
    GError *error = NULL;
    GVariantBuilder *builder_data;
    char* temp;


    /*-------------------------------------
    |             VERIFICATION             |
    --------------------------------------*/

    g_assert(data != NULL);
    g_assert(data_sz != 0);


    /*-------------------------------------
    |           CREATE G_VARIANT           |
    --------------------------------------*/

    builder_data = NULL;

    builder_data = g_variant_builder_new(G_VARIANT_TYPE_ARRAY);

    for (size_t i = 0; i < data_sz; i++)
    {
        g_variant_builder_add(builder_data, "y", data[i]); /* Added uuid string to builder */
    }

    if(tcp_clnt_uuid == NULL)
    {
        temp = malloc(1);
        temp[0] = '\0';
        gvar = g_variant_new("(say)", temp, builder_data);  /* Generate final g_variant to send. G_Variant contains a string (the uuid), and an array of bytes (the data) */
    }
    else
    {
        gvar = g_variant_new("(say)", tcp_clnt_uuid, builder_data);  /* Generate final g_variant to send. G_Variant contains a string (the uuid), and an array of bytes (the data) */
    }


    g_variant_builder_unref(builder_data);   /* cleanup */


    /*-------------------------------------
    |         SEND DATA OVER DBUS          |
    --------------------------------------*/

    info_print("DBUS CLIENT: tcp_dbus_send_msg(): %s\n\n",g_variant_get_type_string(gvar));
    gvar = g_dbus_proxy_call_sync(dbus_config[clnt_id]->proxy, DBUS_TCP_SEND_MSG, gvar, G_DBUS_CALL_FLAGS_NONE, 1000, NULL, &error);


    /*-------------------------------------
    |            VERIFY RESULTS            |
    --------------------------------------*/

    g_assert_no_error(error);


    /*--------------------------------------------
    |  RECEIVE RESPONSE INDICATING IF SUCCESSFUL  |
    ---------------------------------------------*/

    g_variant_get(gvar, "(b)", &send_status);
    g_variant_unref(gvar);

    if(tcp_clnt_uuid == NULL)
    {
        free(temp);
        temp = NULL;
    }

    return send_status;
} /* tcp_dbus_send_msg() */

uint16_t tcp_dbus_get_connected_clients( dbus_clnt_id clnt_id, char*** clients_arry_ptr )
{
    /*-------------------------------------
    |              VARIABLES               |
    --------------------------------------*/

    GVariantIter *iter;
    char* arry = NULL;
    GVariant *gvar;
    GError *error = NULL;
    uint32_t n_bytes = 0;
    uint16_t n_clients = 0;


    /*-------------------------------------
    |             VERIFICATION             |
    --------------------------------------*/

    g_assert(*clients_arry_ptr == NULL);


    /*-------------------------------------
    |         SEND DATA OVER DBUS          |
    --------------------------------------*/

    info_print("DBUS CLIENT: tcp_dbus_get_connected_clients(): Request array of clients\n\n");
    gvar = g_dbus_proxy_call_sync(dbus_config[clnt_id]->proxy, DBUS_TCP_GET_CLIENTS, NULL, G_DBUS_CALL_FLAGS_NONE, 1000, NULL, &error);


    /*-------------------------------------
    |            VERIFY RESULTS            |
    --------------------------------------*/

    g_assert_no_error(error);


    /*--------------------------------------------
    |  GET SERIALIZED BYTE ARRY OF CLIENT UUID'S  |
    ---------------------------------------------*/

    info_print("DBUS CLIENT: tcp_dbus_get_connected_clients(): Parameter returned types are %s\n",g_variant_get_type_string(gvar)); /* Print types returned in gvariant */

    g_variant_get(gvar, "(ay)", &iter); /* 1st parameter should be type 'ay' (arry of bytes) and is the serialized list of client UUIDs. These are contained in a tuple "()" */
    n_bytes = get_data_arry(&iter, &arry); /* Utility function to get array and number of bytes in the array */


    if(n_bytes == 1 || arry[0] == '\0')
    {
        free(arry);
        *clients_arry_ptr = NULL;
        n_bytes = 0;
        n_clients = 0;
    }
    else
    {
        /* Determine the number of clients */
        for (size_t i = 0; i < n_bytes; i++)
        {
            if(arry[i] == '\0' || arry[i] == ' ')
            {
                n_clients++;
            }
        }

        /* Allocate memory */
        *clients_arry_ptr = malloc(n_clients*sizeof(char*));

        /* assignment of memory */
        uint16_t j = 0;
        char* temp = &arry[0];
        for ( size_t i = 0; i < n_bytes; i++ )
        {
            if( arry[i] == '\0' || arry[i] == ' ' )
            {
                arry[i] = '\0'; /* Ensure null termination */
                (*clients_arry_ptr)[j] = temp; /* Set pointer to part of the serialized string of client ID's separated by [space] or '\0' */

                if( i+1 < n_bytes )
                {
                    j++;
                    temp = &arry[i+1];
                }
            }
        }/* for */
    }

    g_variant_iter_free(iter);
    g_variant_unref(gvar);

    return (uint16_t)n_clients;
} /* tcp_dbus_send_msg() */

char* tcp_dbus_get_client_ip( dbus_clnt_id clnt_id, const char* clnt_uuid )
{
    /*-------------------------------------
    |              VARIABLES               |
    --------------------------------------*/

    char* clnt_ip;
    GVariant *gvar;
    GError *error = NULL;


    /*-------------------------------------
    |             VERIFICATION             |
    --------------------------------------*/

    g_assert(clnt_uuid != NULL);


    /*-------------------------------------
    |         SEND DATA OVER DBUS          |
    --------------------------------------*/

    info_print("DBUS CLIENT: tcp_dbus_get_client_ip(): Request client %s ip address.\n\n", clnt_uuid);
    gvar = g_dbus_proxy_call_sync(dbus_config[clnt_id]->proxy, DBUS_TCP_GET_IP, g_variant_new ("(s)",clnt_uuid), G_DBUS_CALL_FLAGS_NONE, 1000, NULL, &error);


    /*-------------------------------------
    |            VERIFY RESULTS            |
    --------------------------------------*/

    g_assert_no_error(error);


    /*--------------------------------------------
    |  RECEIVE RESPONSE INDICATING IF SUCCESSFUL  |
    ---------------------------------------------*/

    g_variant_get(gvar, "(s)", &clnt_ip);
    g_variant_unref(gvar);

    /* If no ip found/invalid client UUID was given */
    if ( clnt_ip[0] == '\0' )
    {
        // free(clnt_ip);
        return NULL;
    }

    return clnt_ip;
} /* tcp_dbus_get_client_ip() */

bool tcp_dbus_connected_to_tcp_srv( dbus_clnt_id clnt_id )
{
    /*-------------------------------------
    |              VARIABLES               |
    --------------------------------------*/

    bool return_val;
    GVariant *gvar;
    GError *error = NULL;


    /*-------------------------------------
    |         SEND DATA OVER DBUS          |
    --------------------------------------*/

    info_print("DBUS CLIENT: tcp_dbus_connected_to_tcp_srv(): Sending request to DBUS server\n\n");
    gvar = g_dbus_proxy_call_sync(dbus_config[clnt_id]->proxy, DBUS_TCP_IS_CONNECTED, NULL, G_DBUS_CALL_FLAGS_NONE, 1000, NULL, &error);


    /*-------------------------------------
    |            VERIFY RESULTS            |
    --------------------------------------*/

    g_assert_no_error(error);


    /*--------------------------------------------
    |  RECEIVE RESPONSE INDICATING IF SUCCESSFUL  |
    ---------------------------------------------*/

    g_variant_get(gvar, "(b)", &return_val);
    g_variant_unref(gvar);

    return return_val;
} /* tcp_dbus_connected_to_tcp_srv() */


/*-------------------------------------
|         DBUS CLIENT-SPECIFIC         |
|         FUNCTION DEFINITIONS         |
--------------------------------------*/

dbus_clnt_id tcp_dbus_client_create()
{
    /*-------------------------------------
    |              VARIABLES               |
    --------------------------------------*/

    dbus_clnt_id new_clnt_id;

    /*-------------------------------------
    |       FIND FIRST AVAILABLE ID        |
    --------------------------------------*/

    for(new_clnt_id = 0; new_clnt_id < MAX_NUM_CLIENTS && dbus_config[new_clnt_id] != NULL; new_clnt_id++);

    /*-------------------------------------
    |        VERIFY ID IS AVAILABLE        |
    --------------------------------------*/

    if ( new_clnt_id == MAX_NUM_CLIENTS-1 && dbus_config[new_clnt_id] == NULL )
    {
        err_print("ERROR: DBUS CLIENT: Attempted to create more than MAX_NUM_CLIENTS!!!");
        exit(EXIT_FAILURE);
    }


    /*-------------------------------------
    |           ALLOCATE CLIENT            |
    --------------------------------------*/

    dbus_config[new_clnt_id] = malloc(sizeof(struct dbus_clnt_config));
    bzero(dbus_config[new_clnt_id], sizeof(struct dbus_clnt_config));
    
    return new_clnt_id;
}

void tcp_dbus_client_delete(dbus_clnt_id clnt_id)
{
    /*-------------------------------------
    |        VERIFY CLNT_ID EXISTS         |
    --------------------------------------*/

    if ( dbus_config[clnt_id] == NULL )
    {
        return;
    }


    /*-------------------------------------
    |          DEALLOCATE SERVER           |
    --------------------------------------*/

    free(dbus_config[clnt_id]);
    dbus_config[clnt_id] = NULL;
    
}

int tcp_dbus_client_init(dbus_clnt_id clnt_id, char const ** srv_version)
{
    /*-------------------------------------
    |              VARIABLES               |
    --------------------------------------*/

    GError *error;
    GVariant *variant;


    /*-------------------------------------
    |           INITIALIZATIONS            |
    --------------------------------------*/

     error = NULL;

     dbus_config[clnt_id]->ServerName = DBUS_TCP_SERVER_NAME;
     dbus_config[clnt_id]->Interface  = DBUS_TCP_IFACE;
     dbus_config[clnt_id]->ObjectPath = DBUS_TCP_OPATH;
     dbus_config[clnt_id]->conn       = NULL;
     dbus_config[clnt_id]->loop       = NULL;
     dbus_config[clnt_id]->proxy      = NULL;


     /*-------------------------------------
     |            VERIFICATIONS             |
     --------------------------------------*/

    if ( dbus_config[clnt_id]->conn != NULL )
    {
        warning_print("DBUS CLIENT: WARNING: an attempt to to initialize the client was made but client has already been intialized.\n");
        return EXIT_FAILURE;
    }


    /*-------------------------------------
    |           CONNECT TO DBUS            |
    --------------------------------------*/
    
    /* Note, the 2 main buses are the system (G_BUS_TYPE_SYSTEM) and session/user (G_BUS_TYPE_SESSION) buses. */
    dbus_config[clnt_id]->conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    g_assert_no_error(error);


    dbus_config[clnt_id]->proxy = g_dbus_proxy_new_sync(dbus_config[clnt_id]->conn,
                                    G_DBUS_PROXY_FLAGS_NONE,
                                    NULL,				                    /* GDBusInterfaceInfo */
                                    dbus_config[clnt_id]->ServerName,		/* name */
                                    dbus_config[clnt_id]->ObjectPath,	    /* object path */
                                    dbus_config[clnt_id]->Interface,	    /* interface */
                                    NULL,				                    /* GCancellable */
                                    &error);
    g_assert_no_error(error);


    /*-------------------------------------
    |          CREATE G_MAIN_LOOP          |
    --------------------------------------*/

    // FIXME Do we need a different loop for every client? Or just one global loop
    dbus_config[clnt_id]->loop = g_main_loop_new(NULL, false);
    g_assert_no_error(error);


    /*-------------------------------------
    |     GET SERVER SOFTWARE VERSION      |
    --------------------------------------*/

    /* read the version property of the interface */
    variant = g_dbus_proxy_get_cached_property(dbus_config[clnt_id]->proxy, "Version");

    // TODO setup tcp_service to automatically be started if it's not running rather than us continueally attempting to connect
    if ( variant == NULL )
    {
        err_print("------FAILED: DBUS CLIENT is not running!------\n");
        g_main_loop_unref (dbus_config[clnt_id]->loop);
        g_object_unref(dbus_config[clnt_id]->proxy);
        g_object_unref(dbus_config[clnt_id]->conn);
        return DBUS_SRV_NOT_AVAILABLE;
    }

    g_variant_get(variant, "s", &server_version);
    g_variant_unref(variant);

    if( srv_version != NULL )
    {
        *srv_version = server_version;
    }

    return EXIT_SUCCESS;
} /* tcp_dbus_client_init() */


void* GMainLoop_Thread(void *loop)
{
    info_print("DBUS CLIENT: main thread is executing...\n");


    /*-------------------------------------
    |   EXECUTE G_MAIN_LOOP INDEFINITELY   |
    --------------------------------------*/

    /*
     * The only way to break the loop is to call
     * g_main_loop_quit(dbus_config[clnt_id]->loop) from any thread
     */
    g_main_loop_run( (GMainLoop*) loop );

    info_print("DBUS CLIENT: main thread is exiting...\n");

    return NULL;
} /* GMainLoop_Thread() */

int start_main_loop(dbus_clnt_id clnt_id)
{
    /*-------------------------------------
    |              VARIABLES               |
    --------------------------------------*/

    pthread_t thread_id;


    /*------------------------------------
    |       SPAWN G_MAIN_LOOP THREAD      |
    -------------------------------------*/

    if ( EXIT_SUCCESS != pthread_create(&thread_id, NULL, GMainLoop_Thread, (void*)dbus_config[clnt_id]->loop) )
    {
        err_print("ERROR: DBUS CLIENT: Failed to create subscriber thread!\n");
        return (EXIT_FAILURE);
    }
    if ( EXIT_SUCCESS != pthread_detach(thread_id) )
    {
        err_print("ERROR: DBUS CLIENT: Failed to detach subscriber thread!\n");
        return (EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
} /* start_main_loop() */

uint get_data_arry(GVariantIter **iter, char** data)
{
    /*-------------------------------------
    |              VARIABLES               |
    --------------------------------------*/

    uint i,arry_sz;
    gchar c;


    /*-------------------------------------
    |           INITIALIZATIONS            |
    --------------------------------------*/

    i       = 0;
    arry_sz = 0;


    /*-------------------------------------
    |          COPY DATA TO ARRAY          |
    --------------------------------------*/

    arry_sz = g_variant_iter_n_children(*iter);
    *data    = malloc(arry_sz);

    while (g_variant_iter_loop(*iter, "y", &c))
    {
        // g_print("\t%c\n", c);
        (*data)[i] = c;
        i++;
    }

    return arry_sz;
} /* get_data_arry() */

void SubscriberCallback(GDBusConnection *conn, const gchar *sender_name, const gchar *object_path, const gchar *interface_name, const gchar *signal_name, GVariant *parameters,gpointer callback_data)
{
    info_print("\n****************DBUS CLIENT: SubscriberCallback: signal \"%s\" received.****************\n\n", signal_name);
    /* RESOURCES
    https://developer.gnome.org/glib/stable/gvariant-format-strings.html
    https://developer.gnome.org/glib/stable/glib-GVariant.html
    https://people.gnome.org/~ryanl/glib-docs/gvariant-format-strings.html#gvariant-format-strings-arrays
    */

    /*-------------------------------------
    |              VARIABLES               |
    --------------------------------------*/

    GVariantIter *iter;
    const gchar *uuid;
    char* data;
    uint data_sz;


    /*-------------------------------------
    |           INITIALIZATIONS            |
    --------------------------------------*/
    uuid    = NULL;
    data    = NULL;
    data_sz = 0;


    /*-------------------------------------
    |        GET ITERATOR OVER DATA        |
    --------------------------------------*/

    info_print("\n%s\n",g_variant_get_type_string(parameters));

    if( 0 == strcmp(signal_name, DBUS_TCP_RECV_SIGNAL) )
    {
        g_variant_get(parameters, "(say)", &uuid, &iter); /* 1st parameter is type 's' (string) and is the uuid. 2nd parameter is an array of bytes. These are contained in a tuple "()" */
        data_sz = get_data_arry(&iter, &data);
    }
    else
    {
        g_variant_get(parameters, "(s)", &uuid);
    }


    /*-------------------------------------
    |  CALL USER CALLBACK WITH ARRY DATA   |
    --------------------------------------*/

    (*(((struct dbus_subscriber*)callback_data)->user_callback))(uuid, data, data_sz);


    /*-------------------------------------
    |               CLEANUP                |
    --------------------------------------*/

    if( 0 == strcmp(signal_name, DBUS_TCP_RECV_SIGNAL) )
    {
        g_variant_iter_free(iter);
        free(data);
    }

} /* SubscriberCallback */

int tcp_dbus_client_Subscribe2Recv(dbus_clnt_id clnt_id, char* signal, tcp_rx_signal_callback callback)
{
    /*-------------------------------------
    |              VARIABLES               |
    --------------------------------------*/

    struct dbus_subscriber *tcp_sbscr;
    size_t i;


    /*-------------------------------------
    |   CHECK FOR EXISTING SUBSCRIPTION    |
    |     GET NEXT AVAILABLE TCP_SBSCR     |
    --------------------------------------*/

    tcp_sbscr = NULL;

    for ( i = 0; i < NUM_SIGNALS; i++ )
    {
        if( dbus_config[clnt_id]->tcp_sbscr[i].SignalName != NULL )
        {
            if( 0 == strcmp( dbus_config[clnt_id]->tcp_sbscr[i].SignalName, signal ) )
            {
                if ( dbus_config[clnt_id]->tcp_sbscr[i].isSubscribed == true )
                {
                    warning_print("DBUS CLIENT: WARNING: Attempted to subscribe to server when subscription already exists!\n");
                    return EXIT_FAILURE;
                }
            }
        }
        else if ( tcp_sbscr == NULL )
        {
            tcp_sbscr = &dbus_config[clnt_id]->tcp_sbscr[i]; /* get the appropiate tcp_sbscr instance for clnt_id */
        }
    }


    /*-------------------------------------
    |             VERIFICATION             |
    --------------------------------------*/

    if ( tcp_sbscr == NULL )
    {
        return EXIT_FAILURE;
    }


    /*-------------------------------------
    |           INITIALIZATIONS            |
    --------------------------------------*/

    tcp_sbscr->SignalName    = signal;                 /* signal to subscribe to */
    tcp_sbscr->callback      = SubscriberCallback;     /* This callback is our local callback */
    tcp_sbscr->user_callback = callback;               /* This callback is given to us to call from our local callback */
    tcp_sbscr->callback_data = NULL;                   /* This will be callback given to us as a parameter */
    tcp_sbscr->dbus_config   = dbus_config[clnt_id];   /* Give tcp_sbscr pointer to owning dbus_config */


    /*-------------------------------------
    |            VERIFICATIONS             |
    --------------------------------------*/

    if ( tcp_sbscr->callback == NULL ||  tcp_sbscr->SignalName == NULL || tcp_sbscr->dbus_config == NULL || tcp_sbscr->dbus_config->ServerName == NULL || tcp_sbscr->dbus_config->Interface == NULL || tcp_sbscr->dbus_config->ObjectPath == NULL || tcp_sbscr->dbus_config->conn == NULL || tcp_sbscr->dbus_config->loop == NULL )
    {
        return EXIT_FAILURE;
    }

    if (tcp_sbscr->isSubscribed == true )
    {
        warning_print("DBUS CLIENT: WARNING: Attempted to subscribe to server when subscription already exists!\n");
        return EXIT_FAILURE;
    }


    /*-------------------------------------
    |         SUBSCRIBE TO SERVER          |
    --------------------------------------*/

    // info_print("DBUS CLIENT: Adding signal %s subscription to main loop...\n", tcp_sbscr->SignalName);
    tcp_sbscr->subscription_id = g_dbus_connection_signal_subscribe( tcp_sbscr->dbus_config->conn,
                                                                     tcp_sbscr->dbus_config->ServerName,
                                                                     tcp_sbscr->dbus_config->Interface,
                                                                     tcp_sbscr->SignalName,
                                                                     tcp_sbscr->dbus_config->ObjectPath,
                                                                     NULL,
                                                                     /* arg0 */G_DBUS_SIGNAL_FLAGS_NONE,
                                                                     tcp_sbscr->callback /* Callback to be called */,
                                                                     tcp_sbscr/* data passed to callback function */,
                                                                     NULL/* user data free function (called when subscr is removed) */);
    
    tcp_sbscr->isSubscribed = true;


    /*-------------------------------------
    |        CONFIGURE G_MAIN_LOOP         |
    --------------------------------------*/

    g_main_loop_ref(tcp_sbscr->dbus_config->loop);

    if ( !g_main_loop_is_running(tcp_sbscr->dbus_config->loop) )
    {
        /* If the g_main_loop is not running, start it */
        if (EXIT_FAILURE == start_main_loop(clnt_id))
        {
            err_print("ERROR: DBUS CLIENT: GMainLoop loop failed to start.\n");
            return EXIT_FAILURE;
        }
    } /* if() */

    return EXIT_SUCCESS;
} /* tcp_dbus_client_Subscribe2Recv() */

int tcp_dbus_client_UnsubscribeRecv(dbus_clnt_id clnt_id, char* signal)
{
    /*--------------------------------------
    |              VARIABLES               |
    --------------------------------------*/

    struct dbus_subscriber *tcp_sbscr;
    size_t i;


    /*-------------------------------------
    |            VERIFICATIONS             |
    --------------------------------------*/

    if( signal == NULL )
    {
        return EXIT_FAILURE;
    }

    /*-------------------------------------
    |            GET TCP_SBSCR             |
    --------------------------------------*/

    for ( i = 0; i < NUM_SIGNALS; i++ )
    {
        if( dbus_config[clnt_id]->tcp_sbscr[i].SignalName != NULL )
        {
            if( 0 == strcmp( dbus_config[clnt_id]->tcp_sbscr[i].SignalName, signal ) )
            {
                break;
            }
        }
    }


    /*-------------------------------------
    |            VERIFICATIONS             |
    --------------------------------------*/
    
    if ( i >= NUM_SIGNALS || dbus_config[clnt_id]->tcp_sbscr[i].isSubscribed == false )
    {
        warning_print("DBUS CLIENT: WARNING: Attempted to unsubscribe from the server when subscription doesn't exist!\n");
        return EXIT_FAILURE;
    }


    /*-------------------------------------
    |           INITIALIZATIONS            |
    --------------------------------------*/

    tcp_sbscr = &dbus_config[clnt_id]->tcp_sbscr[i];   /* get the appropiate tcp_sbscr instance for clnt_id */


    /*-------------------------------------
    |          CANCEL SUBSCRIBER           |
    --------------------------------------*/

    g_dbus_connection_signal_unsubscribe(tcp_sbscr->dbus_config->conn,tcp_sbscr->subscription_id);
    g_main_loop_unref(tcp_sbscr->dbus_config->loop);
    tcp_sbscr->isSubscribed = false;

    bzero(tcp_sbscr, sizeof(struct dbus_subscriber));

    return EXIT_SUCCESS;
} /* tcp_dbus_client_UnsubscribeRecv() */

void tcp_dbus_client_disconnect(dbus_clnt_id clnt_id)
{
    /*-------------------------------------
    |              VARIABLES               |
    --------------------------------------*/

    size_t i;


    /*-------------------------------------
    |            VERIFICATIONS             |
    --------------------------------------*/

    if ( dbus_config[clnt_id]->conn == NULL || dbus_config[clnt_id]->proxy == NULL || dbus_config[clnt_id]->loop == NULL )
    {
        return;
    }


    /*-------------------------------------
    |           INITIALIZATIONS            |
    --------------------------------------*/

    GError *err = NULL;


    /*-------------------------------------
    |       UNSUBSCRIBE ALL SIGNALS        |
    --------------------------------------*/

    for (i = 0; i < NUM_SIGNALS; i++)
    {
        tcp_dbus_client_UnsubscribeRecv(clnt_id,dbus_config[clnt_id]->tcp_sbscr[i].SignalName);
    }


    /*-------------------------------------
    |         DISCONNECT PROCEDURE         |
    --------------------------------------*/

    info_print("DBUS CLIENT: Disconnect client and kill client thread\n");
    /* Not sure how many/which of these are needed */
    g_main_loop_quit(dbus_config[clnt_id]->loop);
    g_main_loop_unref (dbus_config[clnt_id]->loop);
    g_dbus_connection_close_sync(dbus_config[clnt_id]->conn,NULL,&err);
    g_assert_no_error(err);


    /*-------------------------------------
    |               CLEANUP                |
    --------------------------------------*/

    g_object_unref(dbus_config[clnt_id]->proxy);
    g_object_unref(dbus_config[clnt_id]->conn);

    bzero(&dbus_config,sizeof(struct dbus_clnt_config));

} /* tcp_dbus_client_disconnect() */
