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

void test_Ping(void)
{
	GVariant *result;
	GError *error = NULL;
	const gchar *str;

	g_printf("Calling Ping()...\n");
	result = g_dbus_proxy_call_sync(dbus_config.proxy, "Ping",NULL, G_DBUS_CALL_FLAGS_NONE,	-1,	NULL, &error);
	g_assert_no_error(error);
	g_variant_get(result, "(&s)", &str);
	g_printf("The server answered: '%s'\n", str);
	g_variant_unref(result);
} /* test_Ping() */


void test_Echo(void)
{
	GVariant *result;
	GError *error = NULL;
	const gchar *str;

	g_printf("Calling Echo('1234')...\n");
	result = g_dbus_proxy_call_sync(dbus_config.proxy,"Echo",g_variant_new ("(s)", "1234"),G_DBUS_CALL_FLAGS_NONE,-1,NULL,&error);
	g_assert_no_error(error);
	g_variant_get(result, "(&s)", &str);
	g_printf("The server answered: '%s'\n", str);
	g_variant_unref(result);
} /* test_Echo() */

void test_CommandEmitSignal(void)
{
	GVariant *result;
	GError *error = NULL;

	g_printf("Calling method EmitSignal()...\n");
	result = g_dbus_proxy_call_sync(dbus_config.proxy,"EmitSignal",NULL,/* no arguments */G_DBUS_CALL_FLAGS_NONE,-1, NULL,&error);
	g_assert_no_error(error);
	g_variant_unref(result);
} /* test_CommandEmitSignal() */

void test_Quit(void)
{
	GVariant *result;
	GError *error = NULL;

	g_printf("Calling method Quit()...\n");
	result = g_dbus_proxy_call_sync(dbus_config.proxy,"Quit",NULL,G_DBUS_CALL_FLAGS_NONE,-1,NULL,&error);
	g_assert_no_error(error);
	g_variant_unref(result);
} /* test_Quit() */


/*-------------------------------------
|         DBUS CLIENT-SPECIFIC         |
|         FUNCTION DEFINITIONS         |
-------------------------------------*/

int init_client()
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

     dbus_config.ServerName = DBUS_SERVER_NAME;
     dbus_config.Interface  = DBUS_IFACE;
     dbus_config.ObjectPath = DBUS_OPATH;
     dbus_config.conn       = NULL;
     dbus_config.loop       = NULL;
     dbus_config.proxy      = NULL;


     /*-------------------------------------
     |            VERIFICATIONS             |
     -------------------------------------*/

    if ( dbus_config.conn != NULL )
    {
        printf("WARNING: an attempt to to initialize the client was made but client has already been intialized.\n");
        return EXIT_FAILURE;
    }


    /*-------------------------------------
    |           CONNECT TO DBUS            |
    -------------------------------------*/
    
    /* Note, the 2 main buses are the system (G_BUS_TYPE_SYSTEM) and session/user (G_BUS_TYPE_SESSION) buses. */
	dbus_config.conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
	g_assert_no_error(error);


	dbus_config.proxy = g_dbus_proxy_new_sync(dbus_config.conn,
				                    G_DBUS_PROXY_FLAGS_NONE,
				                    NULL,				            /* GDBusInterfaceInfo */
				                    dbus_config.ServerName,		/* name */
				                    dbus_config.ObjectPath,	    /* object path */
				                    dbus_config.Interface,	        /* interface */
				                    NULL,				            /* GCancellable */
				                    &error);
	g_assert_no_error(error);


    /*-------------------------------------
    |          CREATE G_MAIN_LOOP          |
    -------------------------------------*/

    dbus_config.loop = g_main_loop_new(NULL, false);
	g_assert_no_error(error);


    /*-------------------------------------
    |     GET SERVER SOFTWARE VERSION      |
    -------------------------------------*/

	/* read the version property of the interface */
	variant = g_dbus_proxy_get_cached_property(dbus_config.proxy, "Version");
	g_assert(variant != NULL);
	g_variant_get(variant, "s", &server_version);
	g_variant_unref(variant);

    return EXIT_SUCCESS;
} /* init_client() */


void* GMainLoop_Thread(void *loop)
{
    printf("GMainLoop_Thread is executing...\n");


    /*-------------------------------------
    |   EXECUTE G_MAIN_LOOP INDEFINITELY   |
    -------------------------------------*/
 
    /*
	 * The only way to break the loop is to call
     * g_main_loop_quit(dbus_config.loop) from any thread
	 */
	g_main_loop_run( (GMainLoop*) loop );

    printf("GMainLoop_Thread is exiting...\n");

    return NULL;
} /* GMainLoop_Thread() */

int start_main_loop()
{
    /*-------------------------------------
    |              VARIABLES               |
    -------------------------------------*/

    pthread_t thread_id;
    
    
    /*------------------------------------
    |       SPAWN G_MAIN_LOOP THREAD      |
    -------------------------------------*/

    if ( EXIT_SUCCESS != pthread_create(&thread_id, NULL, GMainLoop_Thread, (void*)dbus_config.loop) )
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

// FIXME  Remove this?
void SubscriberCallback(GDBusConnection *conn, const gchar *sender_name, const gchar *object_path, const gchar *interface_name, const gchar *signal_name, GVariant *parameters,gpointer callback_data)
{
	g_printf("\n****************signal handler: OnEmitSignal received.****************\n\n");
} /* SubscribeCallback */


int Subscribe2Server(void)
{
    /*-------------------------------------
    |            VERIFICATIONS             |
    -------------------------------------*/

    // if ( /*tcp_sbscr.callback == NULL || */ tcp_sbscr.SignalName == NULL || tcp_sbscr.dbus_config == NULL || tcp_sbscr.dbus_config->ServerName == NULL || tcp_sbscr.dbus_config->Interface == NULL || tcp_sbscr.dbus_config->ObjectPath == NULL || tcp_sbscr.dbus_config->conn == NULL || tcp_sbscr.dbus_config->loop == NULL )
    // {
    //     return EXIT_FAILURE;
    // }

    if (tcp_sbscr.isSubscribed == true )
    {
        printf("WARNING: Attempted to subscribe to server when subscription already exists!\n");
        return EXIT_FAILURE;
    }

    /*-------------------------------------
    |           INITIALIZATIONS            |
    --------------------------------------*/
    
    tcp_sbscr.SignalName = DBUS_TCP_RECV_SIGNAL_NAME;
    tcp_sbscr.callback = SubscriberCallback;
    tcp_sbscr.callback_data = NULL; /* This will be callback given to us as a parameter */
    tcp_sbscr.dbus_config = &dbus_config;

    /*-------------------------------------
    |         SUBSCRIBE TO SERVER          |
    -------------------------------------*/

    printf("Adding signal subscriber to main loop...\n");
	tcp_sbscr.id = g_dbus_connection_signal_subscribe   (  tcp_sbscr.dbus_config->conn,
                                                        tcp_sbscr.dbus_config->ServerName,
                                                        tcp_sbscr.dbus_config->Interface,
                                                        tcp_sbscr.SignalName,
                                                        tcp_sbscr.dbus_config->ObjectPath,
                                                        NULL, 
                                                        /* arg0 */G_DBUS_SIGNAL_FLAGS_NONE,
                                                        tcp_sbscr.callback,
                                                        tcp_sbscr.dbus_config->loop, 
                                                        /* user data *//*struct signal callback info*/tcp_sbscr.callback_data);
    
    tcp_sbscr.isSubscribed = true;


    /*-------------------------------------
    |        CONFIGURE G_MAIN_LOOP         |
    -------------------------------------*/

    g_main_loop_ref(tcp_sbscr.dbus_config->loop);

    if ( !g_main_loop_is_running(tcp_sbscr.dbus_config->loop) )
    {
        /* If the g_main_loop is not running, start it */
        if (EXIT_FAILURE == start_main_loop(tcp_sbscr.dbus_config))
        {
            printf("FAILED: GMainLoop loop failed to start.");
            return EXIT_FAILURE;
        }
    } /* if() */

    return EXIT_SUCCESS;
} /* Subscribe2Server() */

int UnsubscribeFromServer(void)
{
    /*-------------------------------------
    |            VERIFICATIONS             |
    -------------------------------------*/
    if (tcp_sbscr.isSubscribed == false )
    {
        printf("WARNING: Attempted to unsubscribe from the server when subscription doesn't exist!\n");
        return EXIT_FAILURE;
    }


    /*-------------------------------------
    |          CANCEL SUBSCRIBER           |
    -------------------------------------*/

    g_dbus_connection_signal_unsubscribe(tcp_sbscr.dbus_config->conn,tcp_sbscr.id);
    g_main_loop_unref(tcp_sbscr.dbus_config->loop);
    tcp_sbscr.isSubscribed = false;
    
    return EXIT_SUCCESS;
} /* UnsubscribeFromServer() */

void disconnect_client(void)
{
    /*-------------------------------------
    |            VERIFICATIONS             |
    -------------------------------------*/

    if ( dbus_config.conn == NULL || dbus_config.proxy == NULL || dbus_config.loop == NULL )
    {
        return;
    }


    /*-------------------------------------
    |           INITIALIZATIONS            |
    -------------------------------------*/

    GError *err = NULL;


    /*-------------------------------------
    |         DISCONNECT PROCEDURE         |
    -------------------------------------*/

    printf("Disconnect client and kill client thread\n");
    /* Not sure how many/which of these are needed */
    g_main_loop_quit(dbus_config.loop);
    g_main_loop_unref (dbus_config.loop);
    g_dbus_connection_close_sync(dbus_config.conn,NULL,&err);
    g_assert_no_error(err);


    /*-------------------------------------
    |               CLEANUP                |
    -------------------------------------*/

    g_object_unref(dbus_config.proxy);
    g_object_unref(dbus_config.conn);

    bzero(&dbus_config,sizeof(struct dbus_clnt_config));

} /* disconnect_client() */
