#ifndef PUB_DBUS_INTERFACE_H
#define PUB_DBUS_INTERFACE_H

/* Upon changing these defines, the server_introspection_xml and the
    com.dashcam.conf file must be updated to match */
#define DBUS_SERVER_NAME    "com.dashcam.Server"
#define DBUS_IFACE          "com.dashcam.TCP_Interface"
#define DBUS_OPATH          "/com/dashcam/TCP_Object"
// #define DBUS_SERVER_NAME    "org.example.TestServer"
// #define DBUS_IFACE          "org.example.TestInterface"
// #define DBUS_OPATH          "/org/example/TestObject"

#define DBUS_TCP_RECV_SIGNAL_NAME "TCP_Recv_Signal"

#endif /* PUB_DBUS_INTERFACE_H */
