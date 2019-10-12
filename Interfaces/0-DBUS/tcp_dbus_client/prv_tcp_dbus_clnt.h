#ifndef PRV_DBUS_CLNT_INTERFACE_H
#define PRV_DBUS_CLNT_INTERFACE_H

/*-------------------------------------
|           PRIVATE INCLUDES           |
-------------------------------------*/

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <gio/gio.h>
#include <glib/gprintf.h>
#include <dbus/dbus-glib-lowlevel.h>


/*-------------------------------------
|         FORWARD DECLARATIONS         |
--------------------------------------*/

struct dbus_clnt_config;


/*-------------------------------------
|           PRIVATE STRUCTS            |
--------------------------------------*/
/**
 * Use by functions for subscribing and unsubscribing
 * to the specified SignalName as needed. Each instance 
 * of this struct represents a unique SignalName subscription 
 * for client clnt_id. Each client can have NUM_SIGNALS number
 * of these scructs (subscriptions to signals)
 * 
 * DO NOT CHANGE THESE VALUES AFTER CALLING tcp_dbus_client_init()!
 * 
 * Note: GDBusSignalCallback is of the following form:
 * void (*GDBusSignalCallback) (GDBusConnection  *connection,
                                     const gchar      *sender_name,
                                     const gchar      *object_path,
                                     const gchar      *interface_name,
                                     const gchar      *signal_name,
                                     GVariant         *parameters,
                                     gpointer          user_data;
 * 
 * For more information see the pub_dbus.h file for #defines
 */
/* Each instance of this struct represents subscription to some signal on the server.
    Note that this is also connection specific and as such includes a ptr to a dbus_config */
struct dbus_subscriber
{
    struct dbus_clnt_config *dbus_config;    /* Pointer to parent dbus_clnt_config struct */
    char* SignalName;                        /* Name of the signal for this subscription */
    GDBusSignalCallback callback;            /* library local callback to call with signal is received */
    tcp_rx_signal_callback user_callback;    /* user callback to call from the library local callback */
    void *callback_data;                     /* Data passed to user callback (not used) */
    guint subscription_id;                   /* Stores the ID given to us by the g_dbus_connection_signal_subscribe() 
                                                function and used in the g_dbus_connection_signal_unsubscribe() function call */
    bool isSubscribed;                       /* Set if that instance is currently in use */
};


/**
 * Each instance of this struct represents a unique client.
 * The system is allowed to have MAX_NUM_CLIENTS clients
 * 
 * DO NOT CHANGE THESE VALUES AFTER CALLING tcp_dbus_client_init()!
 * 
 * For more information see the pub_dbus.h file for #defines
 */
struct dbus_clnt_config
{
    char* ServerName;                               /* Name of the DBUS server */
    char* Interface;                                /* Name of the DBUS Server Interface */
    char* ObjectPath;                               /* Name of the DBUS Server object path */
    GDBusConnection *conn;                          /* Pointer to established connection */
    GMainLoop *loop;                                /* Pointer to MainLoop used to receive signals */
    GDBusProxy *proxy;                              /* Set to NULL */
    struct dbus_subscriber tcp_sbscr[NUM_SIGNALS];  /* Array large enough to contain one dbus_subscriber struct for every signal */
};


/*-------------------------------------
|           PRIVATE STATICS            |
--------------------------------------*/

/* This struct arrays keep track of all clients via dbus_clnt_id tcp_dbus_client_create() */
static struct dbus_clnt_config *dbus_config[MAX_NUM_CLIENTS]     = {0};


/*-------------------------------------
|    PRIVATE FUNCTION DECLARATIONS     |
-------------------------------------*/

/* Run GMainLoop_Thread as a detached thread to execute the g_main_loop
    to provide services for subscribed DBUS client implementations */
void* GMainLoop_Thread(void *loop);

/* When a subscriber receives data, this callback is called. */
void SubscriberCallback(GDBusConnection *conn, const gchar *sender_name, const gchar *object_path, const gchar *interface_name, const gchar *signal_name, GVariant *parameters,gpointer callback_data);

/* This is the thread that is spawned when a subscription is made to a signal on the dbus server */
int start_main_loop(dbus_clnt_id clnt_id);

#endif /* PRV_DBUS_CLNT_INTERFACE_H */