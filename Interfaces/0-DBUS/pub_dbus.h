#ifndef PUB_DBUS_INTERFACE_H
#define PUB_DBUS_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Upon changing these defines, the server_introspection_xml and the
    com.dashcam.conf file must be updated to match */
#define DBUS_TCP_SERVER_NAME        "com.dashcam.tcp"
#define DBUS_TCP_IFACE              "com.dashcam.tcp_iface"
#define DBUS_TCP_OPATH              "/com/dashcam/tcp_obj"

/* Methods */
#define DBUS_TCP_SEND_MSG           "TCP_SEND_MSG"
#define DBUS_TCP_GET_CLIENTS        "TCP_GET_CLIENTS"
#define DBUS_TCP_GET_IP             "TCP_GET_IP"
#define DBUS_TCP_IS_CONNECTED       "IS_TCP_CONNECTED_SRV"

/* Signals */
#define DBUS_TCP_RECV_SIGNAL        "TCP_RECV_SIGNAL"
#define DBUS_TCP_CONNECT_SIGNAL     "TCP_CONNECT_SIGNAL"
#define DBUS_TCP_DISCONNECT_SIGNAL  "TCP_DISCONNECT_SIGNAL"


#ifdef __cplusplus
}
#endif

#endif /* PUB_DBUS_INTERFACE_H */
