/*
 * This uses the recommended GLib API for D-Bus: GDBus,
 * which has been distributed with GLib since version 2.26.
 *
 * For details of how to use GDBus, see:
 * https://developer.gnome.org/gio/stable/gdbus-convenience.html
 *
 * dbus-glib also exists but is deprecated.
 */
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <glib/gprintf.h>
#include <dbus/dbus-glib-lowlevel.h> /* for glib main loop */
#include <gio/gio.h>
#include <pthread.h>

#include "prv_dbus.h"

static const char *server_version;
//TODO make GMainLoop static (aka config->loop should probably be static as I believe the only time it would be different is if implementing different sources)
 

void test_Ping(GDBusProxy *proxy)
{
	GVariant *result;
	GError *error = NULL;
	const gchar *str;

	g_printf("Calling Ping()...\n");
	result = g_dbus_proxy_call_sync(proxy, "Ping",NULL, G_DBUS_CALL_FLAGS_NONE,	-1,	NULL, &error);
	g_assert_no_error(error);
	g_variant_get(result, "(&s)", &str);
	g_printf("The server answered: '%s'\n", str);
	g_variant_unref(result);
}


void test_Echo(GDBusProxy *proxy)
{
	GVariant *result;
	GError *error = NULL;
	const gchar *str;

	g_printf("Calling Echo('1234')...\n");
	result = g_dbus_proxy_call_sync(proxy,"Echo",g_variant_new ("(s)", "1234"),G_DBUS_CALL_FLAGS_NONE,-1,NULL,&error);
	g_assert_no_error(error);
	g_variant_get(result, "(&s)", &str);
	g_printf("The server answered: '%s'\n", str);
	g_variant_unref(result);
}

void test_CommandEmitSignal(GDBusProxy *proxy)
{
	GVariant *result;
	GError *error = NULL;

	g_printf("Calling method EmitSignal()...\n");
	result = g_dbus_proxy_call_sync(proxy,"EmitSignal",NULL,/* no arguments */G_DBUS_CALL_FLAGS_NONE,-1, NULL,&error);
	g_assert_no_error(error);
	g_variant_unref(result);
}

void test_Quit(GDBusProxy *proxy)
{
	GVariant *result;
	GError *error = NULL;

	g_printf("Calling method Quit()...\n");
	result = g_dbus_proxy_call_sync(proxy,"Quit",NULL,G_DBUS_CALL_FLAGS_NONE,-1,NULL,&error);
	g_assert_no_error(error);
	g_variant_unref(result);
}







void on_emit_signal_callback(GDBusConnection *conn, const gchar *sender_name, const gchar *object_path, const gchar *interface_name, const gchar *signal_name, GVariant *parameters,gpointer data)
{
	g_printf("signal handler: OnEmitSignal received.\n");
}




/*
 * Make the server emit a signal and catch it. In order to detect that
 * the callback has been run (and thus the signal emitted by the
 * server), we use a glib event loop to:
 *
 *    a) allow the signal callback to be called;
 *
 *    b) make the callback to break the local glib loop so exiting the
 *       loop indicates that the signal has been received.
 */
void* test_EmitSignal(void *args) // GDBusProxy *proxy, GMainLoop *loop)
{
    struct dbus_clnt_config *arg = args; /* I'm lazy and don't want to cast args everywhere */
	// GMainLoop *loop;
	// GError *error = NULL;
	guint id; /* subscription id */
	GDBusConnection *conn = NULL;

	arg->loop = g_main_loop_new(NULL, false);
    
    if(arg->proxy == NULL || arg->loop == NULL)
    {
        printf("Failed to start subscriber: proxy or loop is NULL\n");
        exit(EXIT_FAILURE);
    }

	conn = g_dbus_proxy_get_connection(arg->proxy);
    
    if(conn == NULL)
    {
        printf("Failed to start subscriber: conn is NULL\n");
        exit(EXIT_FAILURE);
    }

	id = g_dbus_connection_signal_subscribe(conn,DBUS_SERVER_NAME,DBUS_IFACE,"OnEmitSignal",DBUS_OPATH,NULL, /* arg0 */G_DBUS_SIGNAL_FLAGS_NONE,on_emit_signal_callback,arg->loop, /* user data */NULL);
	/*
	 * Make the server emit the signal. Normally no races can
	 * happen here since signal events are only processed once the
	 * loop is started so the callback can't be run before.
	 */
	// g_printf("Calling method EmitSignal()...\n");
	// g_dbus_proxy_call_sync(proxy,"EmitSignal",NULL,/* no arguments */G_DBUS_CALL_FLAGS_NONE,-1, NULL,&error);
	// g_assert_no_error(error);
    test_CommandEmitSignal(arg->proxy);

	/*
	 * The only way to break the loop is to receive the signal and
	 * run the signal's callback.
	 */
	g_main_loop_run(arg->loop);
	g_dbus_connection_signal_unsubscribe(conn, id);
	g_printf("g_main_loop_quit was called on subscribed thread. Unsubscribed and quitting....\n");

    return NULL;
}









int init_client(struct dbus_clnt_config *config)
{
    GError *error = NULL;
	GVariant *variant;

    if ( config == NULL )
    {
        return EXIT_FAILURE;
    }

    // bzero(config, sizeof(struct dbus_clnt_config));

	config->conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
	g_assert_no_error(error);


	config->proxy = g_dbus_proxy_new_sync(config->conn,
				                    G_DBUS_PROXY_FLAGS_NONE,
				                    NULL,				            /* GDBusInterfaceInfo */
				                    config->ServerName,		        /* name */
				                    config->ObjectPath,	            /* object path */
				                    config->Interface,	            /* interface */
				                    NULL,				            /* GCancellable */
				                    &error);
	g_assert_no_error(error);

    config->loop = g_main_loop_new(NULL, false);
	g_assert_no_error(error);

	/* read the version property of the interface */
	variant = g_dbus_proxy_get_cached_property(config->proxy, "Version");
	g_assert(variant != NULL);
	g_variant_get(variant, "s", &server_version);
	g_variant_unref(variant);

    return EXIT_SUCCESS;
} /* init_client */

void* GMainLoop_Thread(void *loop)
{
    printf("GMainLoop_Thread is executing...\n");
    /*
	 * The only way to break the loop is to call
     * g_main_loop_quit(config->loop) from any thread
	 */
	g_main_loop_run( (GMainLoop*) loop );

    printf("GMainLoop_Thread is exiting...\n");

    return NULL;
} /* GMainLoop_Thread */

int start_main_loop(struct dbus_clnt_config *config)
{
    pthread_t thread_id;
    if ( EXIT_SUCCESS != pthread_create(&thread_id, NULL, GMainLoop_Thread, (void*)&config->loop) )
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
} /* start_main_loop */

void SubscriberCallback(GDBusConnection *conn, const gchar *sender_name, const gchar *object_path, const gchar *interface_name, const gchar *signal_name, GVariant *parameters,gpointer callback_data)
{
	g_printf("\n****************signal handler: OnEmitSignal received.****************\n\n");
} /* SubscribeCallback */


int Subscribe2Server(void* args)
{
    struct dbus_subscriber* subsc = args;

    if (subsc == NULL || subsc->callback == NULL || subsc->SignalName == NULL || subsc->config == NULL || subsc->config->ServerName == NULL || subsc->config->Interface == NULL || subsc->config->ObjectPath == NULL || subsc->config->conn == NULL || subsc->config->loop == NULL )
    {
        return EXIT_FAILURE;
    }

    printf("Adding signal subscriber to main loop...\n");
	subsc->id = g_dbus_connection_signal_subscribe   (subsc->config->conn,subsc->config->ServerName,subsc->config->Interface,subsc->SignalName,subsc->config->ObjectPath,NULL, /* arg0 */G_DBUS_SIGNAL_FLAGS_NONE,subsc->callback,subsc->config->loop, /* user data *//*struct signal callback info*/subsc->callback_data);
	// subsc->id = g_dbus_connection_signal_subscribe(subsc->config->conn,      DBUS_SERVER_NAME,          DBUS_IFACE,             "OnEmitSignal",    DBUS_OPATH,              NULL, /* arg0 */G_DBUS_SIGNAL_FLAGS_NONE,SubscriberCallback,subsc->config->loop, /* user data *//*struct signal callback info*/NULL);
    
    g_main_loop_ref(subsc->config->loop);

    if ( !g_main_loop_is_running(subsc->config->loop) )
    {
        /* If the g_main_loop is not running, start it */
        if (EXIT_FAILURE == start_main_loop(subsc->config))
        {
            printf("FAILED: GMainLoop loop failed to start.");
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
} /* Subscribe2Server */

int UnsubscribeFromServer(struct dbus_subscriber* subsc)
{
    g_dbus_connection_signal_unsubscribe(subsc->config->conn,subsc->id);
    g_main_loop_unref(subsc->config->loop);

    return EXIT_SUCCESS;
} /* UnsubscribeFromServer */

void disconnect_client(struct dbus_clnt_config *config)
{
    if ( config == NULL || config->conn == NULL || config->proxy == NULL || config->loop == NULL )
    {
        return;
    }

    GError *err = NULL;

    printf("Disconnect client and kill client thread\n");
    // g_main_loop_quit(config->loop);
    // g_main_loop_unref (config->loop);/* I believe this is mostly just used for keeping track of the number of connections or whatever are using the loop where ref incs the count of current number of uses and and unref is a dec. Note that if dec when count is 1, loop will be freed */
    g_dbus_connection_close_sync(config->conn,NULL,&err);
    g_assert_no_error(err);
    g_object_unref(config->proxy);
    g_object_unref(config->conn);

} /* disconnect_client */


int main(void)
{
    struct dbus_clnt_config dbus_clnt_config = {0}; /* Do not destroy this until server is killed */
    struct dbus_subscriber dbus_sbsc1 = {0}; /* Do not destroy this until server is killed */

    dbus_clnt_config.ServerName = DBUS_SERVER_NAME;
    dbus_clnt_config.Interface = DBUS_IFACE;
    dbus_clnt_config.ObjectPath = DBUS_OPATH;
    
    dbus_sbsc1.config = &dbus_clnt_config;
    dbus_sbsc1.SignalName = "OnEmitSignal";
    dbus_sbsc1.callback = SubscriberCallback;
    dbus_sbsc1.callback_data = NULL;

    if ( EXIT_FAILURE == init_client(&dbus_clnt_config) )
    {
        printf("Failed to initialize client!\nExiting.....\n");
        exit(EXIT_FAILURE);
    }
    // if ( EXIT_FAILURE == start_main_loop(&dbus_clnt_config) )
    // {
    //     printf("Failed to execute client!\nExiting.....\n");
    //     exit(EXIT_FAILURE);
    // }



	printf("Testing server interface v%s\n", server_version);

	/* Test server methods */
	test_Ping(dbus_clnt_config.proxy);
	test_Echo(dbus_clnt_config.proxy);
	test_CommandEmitSignal(dbus_clnt_config.proxy);

    printf("Subscribe to server\n");
    if ( EXIT_FAILURE == Subscribe2Server(&dbus_sbsc1) )
    {
        printf("Failed to subscribe to server!\nExiting.....\n");
        exit(EXIT_FAILURE);
    }

    
    
    for (size_t i = 0; i < 5; i++)
    {
        sleep(1);
        /* Test server methods */
	    test_Ping(dbus_clnt_config.proxy);
	    test_Echo(dbus_clnt_config.proxy);
        test_CommandEmitSignal(dbus_clnt_config.proxy);
    }
    
    UnsubscribeFromServer(&dbus_sbsc1);

    printf("Press enter to quit....\n");
    getchar(); /* Block until any stdin is received */

    /* Signal to server that it needs to quit as it's no longer needed */
	// test_Quit(dbus_clnt_config.proxy);

    disconnect_client(&dbus_clnt_config);

	return 0;
}