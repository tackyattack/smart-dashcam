#ifndef PRV_DBUS_SRV_INTERFACE_H
#define PRV_DBUS_SRV_INTERFACE_H

/*-------------------------------------
|               INCLUDES               |
--------------------------------------*/

#include "pub_tcp_dbus_srv.h"
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
    DBusConnection *conn;                       /* DBUS bus connection */
    GMainLoop *loop;                            /* Loop that executes server */
    dbus_srv__tcp_send_msg_callback callback;   /* Callback for method DBUS_TCP_SEND_MSG */
};


/*-------------------------------------
|           PRIVATE STATICS            |
--------------------------------------*/

/* Contains the dbus_srv_config struct for every server created with tcp_dbus_srv_create() */
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
 *    - com.dashcam.tcp_iface
 *
 * 'com.dashcam.tcp_iface' offers 1 method(s) and 3 signal(s):
 *
 *    	- TCP_SEND_MSG(): 	    Sends array of data given as the second parameter to the client tcp_clnt_uuid specified in the first parameter. Returns true if successful or false if failed to send.
 * 	  	- TCP_RECV_SIGNAL:	    Signal is emitted when data is received over TCP. Emitten signal contains the client's tcp_clnt_uuid we're receiving data from and
 * 							    	the Data array of data. All subscribers will get this notification via callback.
 * 		-TCP_CONNECT_SIGNAL:    A signal is emitted when tcp client connects with clients tcp_clnt_uuid. All subscribers will get this notification via callback.
 * 		-TCP_DISCONNECT_SIGNAL: A signal is emitted when tcp client disconnects with clients tcp_clnt_uuid. All subscribers will get this notification via callback.
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

    "  <interface name='" DBUS_TCP_IFACE "'>\n"
    "    <property name='Version' type='s' access='read' />\n"

    "    <method name='"DBUS_TCP_SEND_MSG"'>\n"
    "      <arg name='string' direction='in' type='s' />\n"
    "      <arg name='string' direction='in' type=\"(ay)\" />\n"
    "      <arg type='b' direction='out' />\n"
    "    </method>\n"
    
    "    <signal name='" DBUS_TCP_RECV_SIGNAL "'>\n"
    "    </signal>"
    
    "    <signal name='" DBUS_TCP_DISCONNECT_SIGNAL "'>\n"
    "    </signal>"
    
    "    <signal name='" DBUS_TCP_CONNECT_SIGNAL "'>\n"
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
 * g_main_loop_quit is called, which is what the tcp_dbus_srv_kill function does.
 * 
 * This is a Blocking function
 */
void* server_thread(void *config);


#endif /* PRV_DBUS_SRV_INTERFACE_H */