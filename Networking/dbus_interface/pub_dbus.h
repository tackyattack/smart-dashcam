#ifndef PUB_DBUS_INTERFACE_H
#define PUB_DBUS_INTERFACE_H

// #include <gio/gio.h>    /* GDBusProxy and GMainLoop */
// #include <dbus/dbus.h>  /* DBusConnection */
// #include <stdbool.h>

/* Upon changing these defines, the server_introspection_xml and the
    com.dashcam.conf file must be updated to match */
#define DBUS_SERVER_NAME    "com.dashcam.Server"
#define DBUS_IFACE          "com.dashcam.TCP_Interface"
#define DBUS_OPATH          "/com/dashcam/TCP_Object"
// #define DBUS_SERVER_NAME    "org.example.TestServer"
// #define DBUS_IFACE          "org.example.TestInterface"
// #define DBUS_OPATH          "/org/example/TestObject"


#endif /* PUB_DBUS_INTERFACE_H */
