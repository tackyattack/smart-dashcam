#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h> /* for glib main loop */

#include "prv_dbus.h"

const char *version = "0.1";

/*
 * This is the XML string describing the interfaces, methods and
 * signals implemented by our 'Server' object. It's used by the
 * 'Introspect' method of 'org.freedesktop.DBus.Introspectable'
 * interface.
 *
 * Currently our tiny server implements only 3 interfaces:
 *
 *    - org.freedesktop.DBus.Introspectable
 *    - org.freedesktop.DBus.Properties
 *    - org.example.TestInterface
 *
 * 'org.example.TestInterface' offers 3 methods:
 *
 *    - Ping(): makes the server answering the string 'Pong'.
 *              It takes no arguments.
 *
 *    - Echo(): replies the passed string argument.
 *
 *    - EmitSignal(): send a signal 'OnEmitSignal'
 *
 *    - Quit(): makes the server exit. It takes no arguments.
 */
const char *server_introspection_xml =
	DBUS_INTROSPECT_1_0_XML_DOCTYPE_DECL_NODE
	"<node>\n"

	"  <interface name='org.freedesktop.DBus.Introspectable'>\n"
	"    <method name='Introspect'>\n"
	"      <arg name='data' type='s' direction='out' />\n"
	"    </method>\n"
	"  </interface>\n"

	"  <interface name='org.freedesktop.DBus.Properties'>\n"
	"    <method name='Get'>\n"
	"      <arg name='interface' type='s' direction='in' />\n"
	"      <arg name='property'  type='s' direction='in' />\n"
	"      <arg name='value'     type='s' direction='out' />\n"
	"    </method>\n"
	"    <method name='GetAll'>\n"
	"      <arg name='interface'  type='s'     direction='in'/>\n"
	"      <arg name='properties' type='a{sv}' direction='out'/>\n"
	"    </method>\n"
	"  </interface>\n"

	/*"  <interface name='org.example.TestInterface'>\n" */
	"  <interface name='" DBUS_IFACE "'>\n"
	"    <property name='Version' type='s' access='read' />\n"
	"    <method name='Ping' >\n"
	"      <arg type='s' direction='out' />\n"
	"    </method>\n"
	"    <method name='Echo'>\n"
	"      <arg name='string' direction='in' type='s'/>\n"
	"      <arg type='s' direction='out' />\n"
	"    </method>\n"
	"    <method name='EmitSignal'>\n"
	"    </method>\n"
	"    <method name='Quit'>\n"
	"    </method>\n"
	"    <signal name='OnEmitSignal'>\n"
	"    </signal>"
	"  </interface>\n"

	"</node>\n";

/*
 * This implements 'Get' method of DBUS_INTERFACE_PROPERTIES so a
 * client can inspect the properties/attributes of 'TestInterface'.
 */
DBusHandlerResult server_get_properties_handler(const char *property, DBusConnection *conn, DBusMessage *reply)
{
	if (!strcmp(property, "Version")) 
    {
		dbus_message_append_args(reply, DBUS_TYPE_STRING, &version, DBUS_TYPE_INVALID);
	} 
    else
    {
		/* Unknown property */
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
	if (!dbus_connection_send(conn, reply, NULL))
	{
        return DBUS_HANDLER_RESULT_NEED_MEMORY;
    }
	return DBUS_HANDLER_RESULT_HANDLED;
} /* server_get_properties_handler */

/*
 * This implements 'GetAll' method of DBUS_INTERFACE_PROPERTIES. This
 * one seems required by g_dbus_proxy_get_cached_property().
 */
DBusHandlerResult server_get_all_properties_handler(DBusConnection *conn, DBusMessage *reply)
{
	DBusHandlerResult result;
	DBusMessageIter array, dict, iter, variant;
	const char *property = "Version";

	/*
	 * All dbus functions used below might fail due to out of
	 * memory error. If one of them fails, we assume that all
	 * following functions will fail too, including
	 * dbus_connection_send().
	 */
	result = DBUS_HANDLER_RESULT_NEED_MEMORY;

	dbus_message_iter_init_append(reply, &iter);
	dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &array);

	/* Append all properties name/value pairs */
	property = "Version";
	dbus_message_iter_open_container(&array, DBUS_TYPE_DICT_ENTRY, NULL, &dict);
	dbus_message_iter_append_basic(&dict, DBUS_TYPE_STRING, &property);
	dbus_message_iter_open_container(&dict, DBUS_TYPE_VARIANT, "s", &variant);
	dbus_message_iter_append_basic(&variant, DBUS_TYPE_STRING, &version);
	dbus_message_iter_close_container(&dict, &variant);
	dbus_message_iter_close_container(&array, &dict);

	dbus_message_iter_close_container(&iter, &array);

	if (dbus_connection_send(conn, reply, NULL))
    {
		result = DBUS_HANDLER_RESULT_HANDLED;
    }

	return result;
} /* server_get_all_properties_handler() */

/*
 * This function implements the 'TestInterface' interface for the
 * 'Server' DBus object.
 *
 * It also implements 'Introspect' method of
 * 'org.freedesktop.DBus.Introspectable' interface which returns the
 * XML string describing the interfaces, methods, and signals
 * implemented by 'Server' object. This also can be used by tools such
 * as d-feet(1) and can be queried by:
 *
 * $ gdbus introspect --session --dest org.example.TestServer --object-path /org/example/TestObject
 */
DBusHandlerResult server_message_handler(DBusConnection *conn, DBusMessage *message, void *config)
{
	DBusHandlerResult result;
    DBusMessage *reply = NULL;
	DBusError err;
	bool quit = false;

	fprintf(stderr, "Got D-Bus request: %s.%s on %s\n",
		            dbus_message_get_interface(message),
		            dbus_message_get_member(message),
		            dbus_message_get_path(message));

	/*
	 * Does not allocate any memory; the error only needs to be
	 * freed if it is set at some point.
	 */
	dbus_error_init(&err);

	if (dbus_message_is_method_call(message, DBUS_INTERFACE_INTROSPECTABLE, "Introspect")) {

		if (!(reply = dbus_message_new_method_return(message)))
        {
			goto fail;
        }

		dbus_message_append_args(reply,DBUS_TYPE_STRING, &server_introspection_xml, DBUS_TYPE_INVALID);
	}
    else if (dbus_message_is_method_call(message, DBUS_INTERFACE_PROPERTIES, "Get")) {
		const char *interface, *property;

		if (!dbus_message_get_args(message, &err, DBUS_TYPE_STRING, &interface, DBUS_TYPE_STRING, &property, DBUS_TYPE_INVALID))
        {
			goto fail;
        }
		if (!(reply = dbus_message_new_method_return(message)))
		{	
            goto fail;
        }

		result = server_get_properties_handler(property, conn, reply);
		dbus_message_unref(reply);
		return result;
	}
    else if (dbus_message_is_method_call(message, DBUS_INTERFACE_PROPERTIES, "GetAll")) 
    {
		if (!(reply = dbus_message_new_method_return(message)))
		{
            goto fail;
        }

		result = server_get_all_properties_handler(conn, reply);
		dbus_message_unref(reply);
		return result;
	}
    else if (dbus_message_is_method_call(message, DBUS_IFACE, "Ping"))
    {
		const char *pong = "Pong";

		if (!(reply = dbus_message_new_method_return(message)))
        {
            goto fail;
        }

		dbus_message_append_args(reply,DBUS_TYPE_STRING, &pong,DBUS_TYPE_INVALID);
	}
    else if (dbus_message_is_method_call(message, DBUS_IFACE, "Echo")) 
    {
		const char *msg;

		if (!dbus_message_get_args(message, &err, DBUS_TYPE_STRING, &msg, DBUS_TYPE_INVALID))
		{
            goto fail;
        }

		if (!(reply = dbus_message_new_method_return(message)))
		{
            goto fail;
        }

		dbus_message_append_args(reply, DBUS_TYPE_STRING, &msg, DBUS_TYPE_INVALID);
	} 
    else if (dbus_message_is_method_call(message, DBUS_IFACE, "EmitSignal")) 
    {
		if (!(reply = dbus_message_new_signal(DBUS_OPATH,DBUS_IFACE,"OnEmitSignal")))
		{
            goto fail;
        }

		if (!dbus_connection_send(conn, reply, NULL))
        {
            return DBUS_HANDLER_RESULT_NEED_MEMORY;
        }

		/* Send a METHOD_RETURN reply. */
		reply = dbus_message_new_method_return(message);
	}
    else if (dbus_message_is_method_call(message, DBUS_IFACE, "Quit")) 
    {
		/*
		 * Quit() has no return values but a METHOD_RETURN
		 * reply is required, so the caller will know the
		 * method was successfully processed.
		 */
		reply = dbus_message_new_method_return(message);
		quit  = true; /* unable to do this as the GMainLoop ptr is not known */
	}
    else
	{
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

fail:
	if (dbus_error_is_set(&err))
    {
		if (reply)
		{
            dbus_message_unref(reply);
        }
		reply = dbus_message_new_error(message, err.name, err.message);
		dbus_error_free(&err);
	}

	/*
	 * In any cases we should have allocated a reply otherwise it
	 * means that we failed to allocate one.
	 */
	if (!reply)
    {
		return DBUS_HANDLER_RESULT_NEED_MEMORY;
    }

	/* Send the reply which might be an error one too. */
	result = DBUS_HANDLER_RESULT_HANDLED;
	if (!dbus_connection_send(conn, reply, NULL))
    {
        result = DBUS_HANDLER_RESULT_NEED_MEMORY;
    }

	dbus_message_unref(reply);

	if (quit)
    {
		fprintf(stderr, "Message Handler received quit command!\n");
		g_main_loop_quit( ((struct dbus_srv_config*)config)->loop );
	}

	return result;
} /* server_message_handler() */



void* server_thread(void *config)
{
    printf("Starting dbus tiny server v%s\n", version);
	/* Start the glib event loop */
	g_main_loop_run( ((struct dbus_srv_config *)config)->loop );
    
    printf("Server thread exiting....\n");
    return NULL;
}  /* server_thread() */

int init_server(struct dbus_srv_config *config)
{
    const DBusObjectPathVTable server_vtable = {.message_function = server_message_handler};
    DBusError err;
	int val;

    bzero(config, sizeof(struct dbus_srv_config));
    dbus_error_init(&err);

	/* connect to the daemon bus. Note, the 2 main buses are the system and session (user) buses.
		This is connecting to the system bus. */
	config->conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
	if (config->conn == NULL)
    {
		fprintf(stderr, "Failed to get a system DBus connection: %s\n", err.message);
		dbus_error_free(&err);
	    return EXIT_FAILURE;
	}

	val = dbus_bus_request_name(config->conn, DBUS_SERVER_NAME, DBUS_NAME_FLAG_REPLACE_EXISTING , &err);
	if (val != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
    {
		fprintf(stderr, "-----Note: This service is required to be run as root-----\nFailed to request name on bus: %s. Be sure to execute as root\n", err.message);
		dbus_error_free(&err);
	    return EXIT_FAILURE;
	}

    val = dbus_connection_register_object_path(config->conn, DBUS_OPATH, &server_vtable, (void*)config);
	if (!val)
    {
		fprintf(stderr, "Failed to register object path for '%s'\n", DBUS_OPATH);
		dbus_error_free(&err);
	    return EXIT_FAILURE;
	}

    /*
	 * For the sake of simplicity we're using glib event loop to
	 * handle DBus messages. This is the only place where glib is
	 * used.
	 */
	config->loop = g_main_loop_new(NULL, false);

    /* Set up the DBus connection to work in a GLib event loop */
	dbus_connection_setup_with_g_main(config->conn, NULL);

    return EXIT_SUCCESS;
} /* init_server() */

int execute_server(struct dbus_srv_config *config)
{
	pthread_t thread_id;

    if ( config == NULL )
    {
        /* config ptr points to nothing. */
        return EXIT_FAILURE;
    }

    if ( config->conn == NULL || config->loop == NULL )
    {
        /* config hasn't been initialized, initialize config/server */
        if( EXIT_FAILURE == init_server(config) )
        {
            printf("Failed to initialize server!\nExiting.....\n");
            return (EXIT_FAILURE);
        }
    }
    
    if ( EXIT_SUCCESS != pthread_create(&thread_id, NULL, server_thread, (void*)config) )
    {
        printf("FAILED to create thread!");
        return (EXIT_FAILURE);
    }
    
    printf("Detach server thread!\n");
    if ( EXIT_SUCCESS != pthread_detach(thread_id) )
    {
        printf("FAILED to detach thread!");
        return (EXIT_FAILURE);
    }

	return EXIT_SUCCESS;
} /* execute_server() */

void kill_server(struct dbus_srv_config *config)
{
   if ( config == NULL || config->loop == NULL )
    {
        return;
    }

    printf("Kill server thread\n");
    g_main_loop_quit(config->loop); /* Kill server (aka message handling thread/g_main_loop) */
    g_main_loop_unref (config->loop);

    //TODO addition cleanup needed?
} /* kill_server() */


/* This is a test function. This function represents what would be done 
    in the fuction/program that is using this dbus server code */
int main(void)
{
    struct dbus_srv_config dbus_srv_config = {0}; /* Do not destroy this until server is killed */

    if ( init_server(&dbus_srv_config) == EXIT_FAILURE )
    {
        printf("Failed to initialize server!\nExiting.....\n");
        exit(EXIT_FAILURE);
    }

    if ( execute_server(&dbus_srv_config) == EXIT_FAILURE )
    {
        printf("Failed to execute server!\nExiting.....\n");
        exit(EXIT_FAILURE);
    }
        

    // for (size_t i = 0; i < 10; i++)
    for(;;)
    {
        sleep(1);
        /* Test server methods */
        dbus_message_new_signal(DBUS_OPATH,DBUS_IFACE,"OnEmitSignal");
        //TODO 
    }

    printf("Press enter to quit....\n");
    getchar(); /* Block until any stdin is received */

    kill_server(&dbus_srv_config);

    return EXIT_SUCCESS;
}