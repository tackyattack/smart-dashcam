#ifndef PRV_DBUS_CLNT_INTERFACE_H
#define PRV_DBUS_CLNT_INTERFACE_H

/*-------------------------------------
|           PRIVATE INCLUDES           |
-------------------------------------*/

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <gio/gio.h>
#include <glib/gprintf.h>
#include <dbus/dbus-glib-lowlevel.h>


/*-------------------------------------
|           PRIVATE STRUCTS            |
--------------------------------------*/
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
    struct dbus_clnt_config *dbus_config;    /* Set this before calling subscribe function */
    char* SignalName;                        /* Set this before calling subscribe function */
    GDBusSignalCallback callback;            /* Set this before calling subscribe function */
    void *callback_data;                     /* Set this before calling subscribe function. Can be NULL */
    guint id;                                /* Set to 0 */
    bool isSubscribed;                       /* Set to 0 */
};


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


/*-------------------------------------
|       PRIVATE STATIC VARIABLES       |
--------------------------------------*/

static struct dbus_clnt_config dbus_config = {0};
static struct dbus_subscriber  tcp_sbscr= {0};



/*-------------------------------------
|    PRIVATE FUNCTION DECLARATIONS     |
-------------------------------------*/

/* Run GMainLoop_Thread as a detached thread to execute the g_main_loop
    to provide services for subscribed DBUS client implementations */
void* GMainLoop_Thread(void *loop);

// FIXME remove this?
/* When a subscriber receives data, this callback is called. */
void SubscriberCallback(GDBusConnection *conn, const gchar *sender_name, const gchar *object_path, const gchar *interface_name, const gchar *signal_name, GVariant *parameters,gpointer callback_data);

/* This is the thread that is spawned when a subscription is made to a signal on the dbus server */
int start_main_loop();

#endif /* PRV_DBUS_CLNT_INTERFACE_H */