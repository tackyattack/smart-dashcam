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
-------------------------------------*/

#include "../pub_dbus.h"
#include "pub_tcp_dbus_clnt.h"
#include "prv_tcp_dbus_clnt.h"


 /*-------------------------------------
 |    DEFINITIONS OF DBUS FUNCTIONS     |
 |      IMPLEMENTED BY THE SERVER       |
 -------------------------------------*/

// void test_Ping(dbus_clnt_id clnt_id)
// {
// 	GVariant *result;
// 	GError *error = NULL;
// 	const gchar *str;

// 	g_printf("Calling Ping()...\n");
// 	result = g_dbus_proxy_call_sync(dbus_config[clnt_id]->proxy, "Ping",NULL, G_DBUS_CALL_FLAGS_NONE,	-1,	NULL, &error);
// 	g_assert_no_error(error);
// 	g_variant_get(result, "(&s)", &str);
// 	g_printf("The server answered: '%s'\n", str);
// 	g_variant_unref(result);
// } /* test_Ping() */


void tcp_dbus_send_msg(dbus_clnt_id clnt_id)/*, char* data, uint data_sz)*/
{
    test_CommandEmitSignal(clnt_id); // FIXME  REMOVE
	// GVariant *result;
	// GError *error = NULL;
	// const gchar *str;

	// g_printf("Calling tcp_dbus_send_msg('1234')...\n");
	// result = g_dbus_proxy_call_sync(dbus_config[clnt_id]->proxy,DBUS_TCP_SEND_MSG,g_variant_new ("(s)", "1234"),G_DBUS_CALL_FLAGS_NONE,-1,NULL,&error);
	// g_assert_no_error(error);
	// g_variant_get(result, "(&s)", &str);
	// g_printf("The server answered: '%s'\n", str);
	// g_variant_unref(result);
} /* tcp_dbus_send_msg() */

// FIXME  REMOVE FUNCTION
void test_CommandEmitSignal(dbus_clnt_id clnt_id)
{
	GVariant *result;
	GError *error = NULL;

	g_printf("Calling method EmitSignal() for DBUS_TCP_RECV_SIGNAL...\n");
	result = g_dbus_proxy_call_sync(dbus_config[clnt_id]->proxy,"EmitSignal",NULL,/* no arguments */G_DBUS_CALL_FLAGS_NONE,-1, NULL,&error);
	g_assert_no_error(error);
	g_variant_unref(result);
} /* test_CommandEmitSignal() */
// FIXME  REMOVE FUNCTION
void test_CommandEmitSignal2(dbus_clnt_id clnt_id)
{
	GVariant *result;
	GError *error = NULL;

	g_printf("Calling method EmitSignal2() for DBUS_TCP_CONNECT_SIGNAL...\n");
	result = g_dbus_proxy_call_sync(dbus_config[clnt_id]->proxy,"EmitSignal2",NULL,/* no arguments */G_DBUS_CALL_FLAGS_NONE,-1, NULL,&error);
	g_assert_no_error(error);
	g_variant_unref(result);
} /* test_CommandEmitSignal() */
// FIXME  REMOVE FUNCTION
void test_CommandEmitSignal3(dbus_clnt_id clnt_id)
{
	GVariant *result;
	GError *error = NULL;

	g_printf("Calling method EmitSignal3() for DBUS_TCP_DISCONNECT_SIGNAL...\n");
	result = g_dbus_proxy_call_sync(dbus_config[clnt_id]->proxy,"EmitSignal3",NULL,/* no arguments */G_DBUS_CALL_FLAGS_NONE,-1, NULL,&error);
	g_assert_no_error(error);
	g_variant_unref(result);
} /* test_CommandEmitSignal() */

// void test_Quit(dbus_clnt_id clnt_id)
// {
// 	GVariant *result;
// 	GError *error = NULL;

// 	g_printf("Calling method Quit()...\n");
// 	result = g_dbus_proxy_call_sync(dbus_config[clnt_id]->proxy,"Quit",NULL,G_DBUS_CALL_FLAGS_NONE,-1,NULL,&error);
// 	g_assert_no_error(error);
// 	g_variant_unref(result);
// } /* test_Quit() */


/*-------------------------------------
|         DBUS CLIENT-SPECIFIC         |
|         FUNCTION DEFINITIONS         |
-------------------------------------*/

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
		printf("ERROR: attemping to create more than MAX_NUM_CLIENTS!!!");
		exit(EXIT_FAILURE);
	}


    /*-------------------------------------
    |           ALLOCATE CLIENT            |
    --------------------------------------*/

	dbus_config[new_clnt_id] = malloc(sizeof(struct dbus_clnt_config));
	bzero(dbus_config[new_clnt_id], sizeof(struct dbus_clnt_config));
    
    // tcp_sbscr[new_clnt_id] = malloc(sizeof(struct dbus_subscriber));
	// bzero(tcp_sbscr[new_clnt_id], sizeof(struct dbus_subscriber));

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
	
    // free(tcp_sbscr[clnt_id]);
	// tcp_sbscr[clnt_id] = NULL;
}

int tcp_dbus_client_init(dbus_clnt_id clnt_id)
{
    /*-------------------------------------
    |              VARIABLES               |
    -------------------------------------*/

    GError *error;
	GVariant *variant;


    /*-------------------------------------
    |           INITIALIZATIONS            |
    -------------------------------------*/

     error = NULL;

     dbus_config[clnt_id]->ServerName = DBUS_TCP_SERVER_NAME;
     dbus_config[clnt_id]->Interface  = DBUS_TCP_IFACE;
     dbus_config[clnt_id]->ObjectPath = DBUS_TCP_OPATH;
     dbus_config[clnt_id]->conn       = NULL;
     dbus_config[clnt_id]->loop       = NULL;
     dbus_config[clnt_id]->proxy      = NULL;


     /*-------------------------------------
     |            VERIFICATIONS             |
     -------------------------------------*/

    if ( dbus_config[clnt_id]->conn != NULL )
    {
        printf("WARNING: an attempt to to initialize the client was made but client has already been intialized.\n");
        return EXIT_FAILURE;
    }


    /*-------------------------------------
    |           CONNECT TO DBUS            |
    -------------------------------------*/
    
    /* Note, the 2 main buses are the system (G_BUS_TYPE_SYSTEM) and session/user (G_BUS_TYPE_SESSION) buses. */
	dbus_config[clnt_id]->conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
	g_assert_no_error(error);


	dbus_config[clnt_id]->proxy = g_dbus_proxy_new_sync(dbus_config[clnt_id]->conn,
				                    G_DBUS_PROXY_FLAGS_NONE,
				                    NULL,				            /* GDBusInterfaceInfo */
				                    dbus_config[clnt_id]->ServerName,		/* name */
				                    dbus_config[clnt_id]->ObjectPath,	    /* object path */
				                    dbus_config[clnt_id]->Interface,	        /* interface */
				                    NULL,				            /* GCancellable */
				                    &error);
	g_assert_no_error(error);


    /*-------------------------------------
    |          CREATE G_MAIN_LOOP          |
    -------------------------------------*/

    // FIXME Do we need a different loop for every client? Or just one global loop
    dbus_config[clnt_id]->loop = g_main_loop_new(NULL, false);
	g_assert_no_error(error);


    /*-------------------------------------
    |     GET SERVER SOFTWARE VERSION      |
    -------------------------------------*/

	/* read the version property of the interface */
	variant = g_dbus_proxy_get_cached_property(dbus_config[clnt_id]->proxy, "Version");
	g_assert(variant != NULL);
	g_variant_get(variant, "s", &server_version);
	g_variant_unref(variant);

    return EXIT_SUCCESS;
} /* tcp_dbus_client_init() */


void* GMainLoop_Thread(void *loop)
{
    printf("GMainLoop_Thread is executing...\n");


    /*-------------------------------------
    |   EXECUTE G_MAIN_LOOP INDEFINITELY   |
    -------------------------------------*/
 
    /*
	 * The only way to break the loop is to call
     * g_main_loop_quit(dbus_config[clnt_id]->loop) from any thread
	 */
	g_main_loop_run( (GMainLoop*) loop );

    printf("GMainLoop_Thread is exiting...\n");

    return NULL;
} /* GMainLoop_Thread() */

int start_main_loop(dbus_clnt_id clnt_id)
{
    /*-------------------------------------
    |              VARIABLES               |
    -------------------------------------*/

    pthread_t thread_id;
    
    
    /*------------------------------------
    |       SPAWN G_MAIN_LOOP THREAD      |
    -------------------------------------*/

    if ( EXIT_SUCCESS != pthread_create(&thread_id, NULL, GMainLoop_Thread, (void*)dbus_config[clnt_id]->loop) )
    {
        printf("Failed to create subscriber thread!\n");
        return (EXIT_FAILURE);
    }
    if ( EXIT_SUCCESS != pthread_detach(thread_id) )
    {
        printf("Failed to detach subscriber thread!\n");
        return (EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
} /* start_main_loop() */

void SubscriberCallback(GDBusConnection *conn, const gchar *sender_name, const gchar *object_path, const gchar *interface_name, const gchar *signal_name, GVariant *parameters,gpointer callback_data)
{
	// g_printf("\n****************SubscriberCallback: signal \"%s\" received.****************\n\n", signal_name);
    /* RESOURCES
    https://developer.gnome.org/glib/stable/gvariant-format-strings.html
    https://developer.gnome.org/glib/stable/glib-GVariant.html
    https://people.gnome.org/~ryanl/glib-docs/gvariant-format-strings.html#gvariant-format-strings-arrays
    */

    /*-------------------------------------
    |              VARIABLES               |
    --------------------------------------*/

    GVariantIter *iter;
    gchar c;
    gchar *arry;
    uint i,arry_sz;


    /*-------------------------------------
    |           INITIALIZATIONS            |
    --------------------------------------*/
    i       = 0;
    iter    = NULL;


    /*-------------------------------------
    |        GET ITERATOR OVER DATA        |
    --------------------------------------*/

    g_variant_get(parameters, "(ay)", &iter);


    /*-------------------------------------
    |        COPY DATA TO NEW ARRAY        |
    --------------------------------------*/

    arry_sz = g_variant_iter_n_children(iter);
    arry    = malloc(arry_sz);
    
    while (g_variant_iter_loop(iter, "y", &c))
    {
        // g_print("\t%c\n", c);
        arry[i] = c;
        i++;
    }


    /*-------------------------------------
    |  CALL USER CALLBACK WITH ARRY DATA   |
    --------------------------------------*/

    (*(((struct dbus_subscriber*)callback_data)->user_callback))(arry, arry_sz);


    /*-------------------------------------
    |               CLEANUP                |
    --------------------------------------*/

    g_variant_iter_free(iter);
    free(arry);

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
                    printf("WARNING: Attempted to subscribe to server when subscription already exists!\n");
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
    -------------------------------------*/

    if ( tcp_sbscr->callback == NULL ||  tcp_sbscr->SignalName == NULL || tcp_sbscr->dbus_config == NULL || tcp_sbscr->dbus_config->ServerName == NULL || tcp_sbscr->dbus_config->Interface == NULL || tcp_sbscr->dbus_config->ObjectPath == NULL || tcp_sbscr->dbus_config->conn == NULL || tcp_sbscr->dbus_config->loop == NULL )
    {
        return EXIT_FAILURE;
    }

    if (tcp_sbscr->isSubscribed == true )
    {
        printf("WARNING: Attempted to subscribe to server when subscription already exists!\n");
        return EXIT_FAILURE;
    }


    /*-------------------------------------
    |         SUBSCRIBE TO SERVER          |
    -------------------------------------*/

    printf("Adding signal %s subscription to main loop...\n", tcp_sbscr->SignalName);
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
    -------------------------------------*/

    g_main_loop_ref(tcp_sbscr->dbus_config->loop);

    if ( !g_main_loop_is_running(tcp_sbscr->dbus_config->loop) )
    {
        /* If the g_main_loop is not running, start it */
        if (EXIT_FAILURE == start_main_loop(clnt_id))
        {
            printf("FAILED: GMainLoop loop failed to start.");
            return EXIT_FAILURE;
        }
    } /* if() */

    return EXIT_SUCCESS;
} /* tcp_dbus_client_Subscribe2Recv() */

int tcp_dbus_client_UnsubscribeRecv(dbus_clnt_id clnt_id, char* signal)
{
    /*-------------------------------------
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
    -------------------------------------*/
    
    if ( i >= NUM_SIGNALS || dbus_config[clnt_id]->tcp_sbscr[i].isSubscribed == false )
    {
        printf("WARNING: Attempted to unsubscribe from the server when subscription doesn't exist!\n");
        return EXIT_FAILURE;
    }


    /*-------------------------------------
    |           INITIALIZATIONS            |
    --------------------------------------*/

    tcp_sbscr = &dbus_config[clnt_id]->tcp_sbscr[i];   /* get the appropiate tcp_sbscr instance for clnt_id */


    /*-------------------------------------
    |          CANCEL SUBSCRIBER           |
    -------------------------------------*/

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
    -------------------------------------*/

    if ( dbus_config[clnt_id]->conn == NULL || dbus_config[clnt_id]->proxy == NULL || dbus_config[clnt_id]->loop == NULL )
    {
        return;
    }


    /*-------------------------------------
    |           INITIALIZATIONS            |
    -------------------------------------*/

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
    -------------------------------------*/

    printf("Disconnect client and kill client thread\n");
    /* Not sure how many/which of these are needed */
    g_main_loop_quit(dbus_config[clnt_id]->loop);
    g_main_loop_unref (dbus_config[clnt_id]->loop);
    g_dbus_connection_close_sync(dbus_config[clnt_id]->conn,NULL,&err);
    g_assert_no_error(err);


    /*-------------------------------------
    |               CLEANUP                |
    -------------------------------------*/

    g_object_unref(dbus_config[clnt_id]->proxy);
    g_object_unref(dbus_config[clnt_id]->conn);

    bzero(&dbus_config,sizeof(struct dbus_clnt_config));

} /* tcp_dbus_client_disconnect() */
