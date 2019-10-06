#ifndef PRV_DBUS_SRV_H
#define PRV_DBUS_SRV_H

/*-------------------------------------
|               INCLUDES               |
--------------------------------------*/

#include "pub_dbus_srv.h"
#include "../pub_dbus.h"
#include <stdbool.h>
#include <stdio.h>
#include <dbus/dbus.h>
#include <gio/gio.h>                 /* GDBusProxy and GMainLoop */
#include <dbus/dbus-glib-lowlevel.h> /* for glib main loop */


/*-------------------------------------
|           PRIVATE STRUCTS            |
--------------------------------------*/

/* This struct is used to pass multiple 
    args to a dbus function. This struct
    represents a configuration set.*/
struct dbus_srv_config
{
    DBusConnection *conn; /* Do not set this value */
    GMainLoop *loop;      /* Do not set this value */
};


/*-------------------------------------
|           PRIVATE STATICS            |
--------------------------------------*/

/* Contains the dbus_srv_config struct for every server created with create_server() */
static struct dbus_srv_config *SRV_CONFIGS_ARRY[MAX_NUM_SERVERS] = {0};

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
static const char *server_introspection_xml =
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


/*-------------------------------------
|     PUBLIC FUNCTION DECLARATIONS     |
--------------------------------------*/

/*
 * This implements 'Get' method of DBUS_INTERFACE_PROPERTIES so a
 * client can inspect the properties/attributes of 'TestInterface'.
 */
DBusHandlerResult server_get_properties_handler(const char *property, DBusConnection *conn, DBusMessage *reply);

/*
 * This implements 'GetAll' method of DBUS_INTERFACE_PROPERTIES. This
 * one seems required by g_dbus_proxy_get_cached_property().
 */
DBusHandlerResult server_get_all_properties_handler(DBusConnection *conn, DBusMessage *reply);



/*-------------------------------------
|    PRIVATE FUNCTION DECLARATIONS     |
--------------------------------------*/


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
DBusHandlerResult server_message_handler(DBusConnection *conn, DBusMessage *message, void *config);

/**
 * This function runs in a separate thread and contains the g_main_loop. 
 * The g_main_loop handles receiving and processing all messages via DBUS.
 * This function is essentially an infinite loop that will only exit if 
 * g_main_loop_quit is called, which is what the kill_server function does.
 * 
 * This is a Blocking function
 */
void* server_thread(void *config);


struct dbus_srv_config* get_srv_config(dbus_srv_id srv_id);

int set_srv_config(dbus_srv_id srv_id, struct dbus_srv_config* config);

#endif /* PRV_DBUS_SRV_H */