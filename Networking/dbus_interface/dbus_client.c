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
#include <stdio.h>
#include <glib/gprintf.h>
#include <gio/gio.h>
#include <pthread.h>
#include <unistd.h>

#include "prv_dbus.h"

/* This struct is used to pass multiple 
    args to the subscriber thread function call*/
struct dbus_client_subsc_args
{
    GDBusProxy *proxy;
    GMainLoop *subscriber_loop;
};


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


void on_emit_signal_callback(GDBusConnection *conn, const gchar *sender_name, const gchar *object_path, const gchar *interface_name, const gchar *signal_name, GVariant *parameters,gpointer data)
{
	GMainLoop *loop = data;

	g_printf("signal handler: OnEmitSignal received.\n");
	// g_main_loop_quit(loop);
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
void* test_EmitSignal(void *args) // GDBusProxy *proxy, GMainLoop *subscriber_loop)
{
    struct dbus_client_subsc_args *arg = args; /* I'm lazy and don't want to cast args everywhere */
	// GMainLoop *loop;
	// GError *error = NULL;
	guint id; /* subscription id */
	GDBusConnection *conn = NULL;

	arg->subscriber_loop = g_main_loop_new(NULL, false);
    
    if(arg->proxy == NULL || arg->subscriber_loop == NULL)
    {
        printf("Failed to start subscriber: proxy or subscriber_loop is NULL\n");
        exit(1);
    }

	conn = g_dbus_proxy_get_connection(arg->proxy);
    
    if(conn == NULL)
    {
        printf("Failed to start subscriber: conn is NULL\n");
        exit(1);
    }

	id = g_dbus_connection_signal_subscribe(conn,DBUS_SERVER_NAME,DBUS_IFACE,"OnEmitSignal",DBUS_OPATH,NULL, /* arg0 */G_DBUS_SIGNAL_FLAGS_NONE,on_emit_signal_callback,arg->subscriber_loop, /* user data */NULL);
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
	g_main_loop_run(arg->subscriber_loop);
	g_dbus_connection_signal_unsubscribe(conn, id);
	g_printf("g_main_loop_quit was called on subscribed thread. Unsubscribed and quitting....\n");

    return NULL;
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


int main(void)
{
	GDBusProxy *proxy;
	GDBusConnection *conn;
	GError *error = NULL;
	const char *version;
	GVariant *variant;

	conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
	g_assert_no_error(error);

	proxy = g_dbus_proxy_new_sync(conn,
				                    G_DBUS_PROXY_FLAGS_NONE,
				                    NULL,				            /* GDBusInterfaceInfo */
				                    DBUS_SERVER_NAME,		        /* name */
				                    DBUS_OPATH,	                    /* object path */
				                    DBUS_IFACE,	                    /* interface */
				                    NULL,				            /* GCancellable */
				                    &error);
	g_assert_no_error(error);

	/* read the version property of the interface */
	variant = g_dbus_proxy_get_cached_property(proxy, "Version");
	g_assert(variant != NULL);
	g_variant_get(variant, "s", &version);
	g_variant_unref(variant);
	printf("Testing server interface v%s\n", version);

	/* Test server methods */
	test_Ping(proxy);
	test_Echo(proxy);
	test_CommandEmitSignal(proxy);

    printf("Create subscription thread\n");

    pthread_t thread_id;
    struct dbus_client_subsc_args subsc_args;
    subsc_args.proxy = proxy;
    subsc_args.subscriber_loop = NULL;
    if ( 0 != pthread_create(&thread_id, NULL, test_EmitSignal, (void*)&subsc_args) )
    {
        printf("Failed to create thread!");
        exit(1);
    }
    
    printf("Detach subscription thread\n");
    if ( 0 != pthread_detach(thread_id) )
    {
        printf("Failed to create thread!");
        exit(1);
    }
    
    for (size_t i = 0; i < 5; i++)
    {
        sleep(1);
        /* Test server methods */
	    test_Ping(proxy);
	    test_Echo(proxy);
        test_CommandEmitSignal(proxy);
    }
    

    if ( subsc_args.subscriber_loop != NULL )
    {
        printf("Kill subscription thread\n");
        g_main_loop_quit(subsc_args.subscriber_loop); /* Kill subscription (aka thread) */
    }

    printf("Press enter to quit....\n");
    getchar(); /* Block until any stdin is received */
    
    /* Signal to server that it needs to quit as it's no longer needed */
	test_Quit(proxy);

    printf("Cleanup\n");
	g_object_unref(proxy);
	g_object_unref(conn);
	return 0;
}