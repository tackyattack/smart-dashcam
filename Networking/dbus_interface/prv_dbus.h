#ifndef PRV_DBUS_INTERFACE_H
#define PRV_DBUS_INTERFACE_H

#include <gio/gio.h>    /* GDBusProxy and GMainLoop */
#include <dbus/dbus.h> /* DBusConnection */
#include <stdbool.h>

/* Upon changing these defines, the server_introspection_xml must also 
    be updated to match */
#define DBUS_SERVER_NAME    "com.dashcam.Server"
#define DBUS_IFACE          "com.dashcam.TCP_Interface"
#define DBUS_OPATH          "/com/dashcam/TCP_Object"
// #define DBUS_SERVER_NAME    "org.example.TestServer"
// #define DBUS_IFACE          "org.example.TestInterface"
// #define DBUS_OPATH          "/org/example/TestObject"


/* This struct is used to pass multiple 
    args to a dbus function. This struct
    represents a configuration set.*/
    //TODO  this should be public not in prv_dbus
struct dbus_srv_config
{
    DBusConnection *conn; /* Do not set this value */
    GMainLoop *loop;      /* Do not set this value */
};

//TODO  this should be public not in prv_dbus
struct dbus_clnt_config
{
    char* ServerName;      /* Set this before calling init function */
    char* Interface;       /* Set this before calling init function */
    char* ObjectPath;      /* Set this before calling init function */
    GDBusConnection *conn; /* Do not set this value */
    GMainLoop *loop;       /* Do not set this value */
    GDBusProxy *proxy;     /* Do not set this value */
};

/* Each instance of this struct represents subscription to some signal on the server.
    Note that this is also connection specific and as such includes a ptr to a dbus_config */
struct dbus_subscriber
{
    guint id;                     /* Do not set this value */
    bool isSubscribed;            /* Do not set this value */
    char* SignalName;             /* Set this before calling subscribe function */
    GDBusSignalCallback callback; /* Set this before calling subscribe function */
    void *callback_data;          /* Set this before calling subscribe function. Can be NULL */
    /* TODO Probably add callbacks to a separate .c file that users can modify */
    struct dbus_clnt_config *config;   /* Set this before calling subscribe function */
};


#endif /* PRV_DBUS_INTERFACE_H */
