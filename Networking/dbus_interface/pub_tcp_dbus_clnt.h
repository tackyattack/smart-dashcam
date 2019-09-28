/* Include this file in your project for access to the TCP DBUS interface. */

#ifndef PUB_DBUS_TCP_CLNT_INTERFACE_H
#define PUB_DBUS_TCP_CLNT_INTERFACE_H

/*-------------------------------------
|               INCLUDES               |
-------------------------------------*/

#include "pub_dbus.h"
#include <gio/gio.h>
#include <glib/gprintf.h>
#include <dbus/dbus-glib-lowlevel.h> /* for glib main loop */


/*-------------------------------------
|            PUBLIC STRUCTS            |
--------------------------------------*/

/**
 * This struct should be allocated and passed to the 
 * public functions as needed. Each instance of this 
 * struct represents a unique client.
 * 
 * Prior to calling the client_init() function, the 
 * parameters of the struct should be set accordingly.
 * 
 * ServerName -> DBUS Server name to connect to
 * Interface  -> DBUS Server Interface to connect to
 * ObjectPath -> DBUS Server Object path to use
 * conn       -> Set NULL
 * loop       -> Set NULL
 * proxy      -> Set NULL
 * 
 * DO NOT CHANGE THESE VALUES AFTER CALLING INIT_CLIENT()!
 * 
 * For more information see the pub_dbus.h file for #defines
 */
struct dbus_clnt_config
{
    char* ServerName;      /* Set this before calling init function */
    char* Interface;       /* Set this before calling init function */
    char* ObjectPath;      /* Set this before calling init function */
    GDBusConnection *conn; /* Set to NULL */
    GMainLoop *loop;       /* Set to NULL */
    GDBusProxy *proxy;     /* Set to NULL */
};


/**
 * This struct should be allocated and passed to the 
 * public functions for subscribing and unsubscribing
 * to the specified SignalName as needed. Each instance 
 * of this struct represents a unique SignalName subscription.
 * 
 * Prior to calling the Subscribe2Server() function, the 
 * parameters of the struct should be set accordingly.
 * 
 * SignalName    -> Set to DBUS Server's signal to subscribe to
 * callback      -> Set to GDBusSignalCallback function pointer to be called when a signal from the server has been received
 * callback_data -> Set to as a void pointer to anything that should be passed to the callback function
 * config        -> Set to a pointer to a struct dbus_clnt_config that has been initialized with init_client() successfully
 * id            -> Set to 0
 * isSubscribed  -> Set to 0
 * 
 * DO NOT CHANGE THESE VALUES AFTER CALLING INIT_CLIENT()!
 * 
 * Note: GDBusSignalCallback is of the following form:
 * void (*GDBusSignalCallback) (GDBusConnection  *connection,
                                     const gchar      *sender_name,
                                     const gchar      *object_path,
                                     const gchar      *interface_name,
                                     const gchar      *signal_name,
                                     GVariant         *parameters,
                                     gpointer          user_data);
 * 
 * For more information see the pub_dbus.h file for #defines
 */
/* Each instance of this struct represents subscription to some signal on the server.
    Note that this is also connection specific and as such includes a ptr to a dbus_config */
struct dbus_subscriber
{
    struct dbus_clnt_config *config;    /* Set this before calling subscribe function */
    char* SignalName;                   /* Set this before calling subscribe function */
    GDBusSignalCallback callback;       /* Set this before calling subscribe function */
    void *callback_data;                /* Set this before calling subscribe function. Can be NULL */
    guint id;                           /* Set to 0 */
    bool isSubscribed;                  /* Set to 0 */
};


/*-------------------------------------
|    COMMAND FUNCTION DECLARATIONS     |
-------------------------------------*/

void test_Ping(GDBusProxy *proxy);

void test_Echo(GDBusProxy *proxy);

void test_CommandEmitSignal(GDBusProxy *proxy);

void test_Quit(GDBusProxy *proxy);


/*-------------------------------------
|     PUBLIC FUNCTION DECLARATIONS     |
--------------------------------------*/

/** 
 * Given a pointer to an allocated dbus_clnt_config
 * struct with valid parameter values, will initialize
 * the dbus client.
 * 
 * Non-blocking call
 */
int init_client(struct dbus_clnt_config *config);

/**
 * Given a pointer to an allocated dbus_subscriber struct 
 * with valid parameter values, will subscribe to specified
 * dbus_subscriber->SignalName. When a signal/message is received
 * from the server, the callback function will be called from
 * separate thread. 
 */
int Subscribe2Server(struct dbus_subscriber *subsc);

/** Given a pointer to a dbus_subscriber struct that has been
 * passed to Subscribe2Server(), which has successfully run,
 * this function will unsubscribe so that callbacks will no
 * longer happen and any signals received for the signal 
 * subscribed to will be ignored.
 */
int UnsubscribeFromServer(struct dbus_subscriber* subsc);

/** Given a pointer to a dbus_clnt_config struct that has been
 * passed to init_client(), which successfully run, 
 * this function will close the dbus connection to the server.
 * */
void disconnect_client(struct dbus_clnt_config *config);

#endif /* PUB_DBUS_TCP_CLNT_INTERFACE_H */
