#ifndef PUB_DBUS_TCP_CLNT_INTERFACE_H
#define PUB_DBUS_TCP_CLNT_INTERFACE_H

/*-------------------------------------
|               INCLUDES               |
--------------------------------------*/

#include <stdlib.h>
#include <stdint.h>


/*-------------------------------------
|            PUBLIC DEFINES            |
--------------------------------------*/

#define MAX_NUM_CLIENTS             (__UINT8_MAX__) /* Max number of dbus clients (instances) a process can have */
#define NUM_SIGNALS                 (3)             /* This should match the signals defined in pub_dbus.h and with the server */
#define DBUS_SRV_NOT_AVAILABLE      (-1)            /* This is returned by tcp_dbus_client_init() if dbus server is not active */

/*-------------------------------------
|           PUBLIC TYPEDEFS            |
--------------------------------------*/

typedef uint8_t dbus_clnt_id;


/*-------------------------------------
|    FUNCTION POINTER DECLARATIONS     |
--------------------------------------*/

/* https://isocpp.org/wiki/faq/mixing-c-and-cpp */

/**
 * This is the universal signal received callback used when a signal is received from dbus server.
 * Use this function prototype when subscribing to signals using tcp_dbus_client_Subscribe2Recv().
 * 
 * When a socket message is received, the dbus server emits signal DBUS_TCP_RECV_SIGNAL. We catch 
 * that signal and call this callback with the socket message sender's uuid, data array, and the
 * size of the data array. tcp_clnt_uuid is a null terminated string, data contains the data received
 * from the tcp client, and data_sz is the size of data.
 * 
 * When a tcp server client connects or disconects, the dbus server emits signal DBUS_TCP_CONNECT_SIGNAL,
 * or DBUS_TCP_DISCONNECT_SIGNAL. We catch these signals after having subscribed to them and when one
 * is caught, this callback is called with the uuid (tcp_clnt_uuid) of the client that
 * connected/disconnected. tcp_clnt_uuid is a null terminated string, data == NULL, and data_sz == 0.
 */
 typedef void (*tcp_rx_signal_callback)(const char* tcp_clnt_uuid, const char* data, unsigned int data_sz);

// /**
//  * When a tcp server client connects or disconects, the dbus server emits signal DBUS_TCP_CONNECT_SIGNAL,
//  * or DBUS_TCP_DISCONNECT_SIGNAL. We catch these signals after having subscribed to them and when one
//  * is caught, this callback is called with the uuid (tcp_clnt_uuid) of the client that
//  * connected/disconnected. tcp_clnt_uuid is a null terminated string.
//  */
//  typedef void (*tcp_connection_signal_callback)(const char* tcp_clnt_uuid);


/*-------------------------------------
|    COMMAND FUNCTION DECLARATIONS     |
-------------------------------------*/

/**
 * This function may be called only after tcp_dbus_client_create() and
 * tcp_dbus_client_init(). Calling this function with the client ID, the
 * uuid of the socket receiver that this message is be to sent to (tcp_clnt_uuid),
 * a data array (data), and the size of that array (data_sz, includes any termination
 * characters) will send the data to the tcp DBUS server and therefore send 
 * the data array over tcp to specified tcp clients. If tcp_clnt_uuid == NULL,
 * then data will be sent to all connected clients.
 * 
 * @Returns true if sending data was successful, else false.
 */
int tcp_dbus_send_msg(dbus_clnt_id clnt_id, const char* tcp_clnt_uuid, char* data, uint data_sz);


/*-------------------------------------
|     PUBLIC FUNCTION DECLARATIONS     |
--------------------------------------*/

/**
 * This function allocates a configuration struct for a unique client
 * and returns that clients unique ID. tcp_dbus_client_init() must be called 
 * with the return unique ID to initialize the client. This function
 * will assert if attempting to create more than MAX_NUM_CLIENTS.
 * Non-blocking
 */
dbus_clnt_id tcp_dbus_client_create();

/** 
 * Given a dbus_clnt_id, will initialize the
 * tcp dbus client by connecting to the server.
 * srv_version is an optionally NULL paramter that
 * returns a pointer to the dbus server's version.
 * 
 * Non-blocking call
 */
int tcp_dbus_client_init(dbus_clnt_id clnt_id, char const ** srv_version);

/**
 * Given a dbus_clnt_id, the signal name, and a callback, 
 * this function will subscribe to tcp recv signal
 * resuling in the callback activating when a tcp message is 
 * received from the server, the callback function will be 
 * called from separate thread. clnt_id must be initialized 
 * and valid.
 */
int tcp_dbus_client_Subscribe2Recv(dbus_clnt_id clnt_id, char* signal, tcp_rx_signal_callback callback);

/** 
 * Given a dbus_clnt_id that was passed to tcp_dbus_client_Subscribe2Recv()
 * to setup a tcp receive subscription (successfully),
 * this function will unsubscribe so that callbacks will no
 * longer happen/no longer receive 
 */
int tcp_dbus_client_UnsubscribeRecv(dbus_clnt_id clnt_id, char* signal);

/** 
 * Given a dbus_clnt_id that has be passed to tcp_dbus_client_init(), 
 * this function will close the dbus connection to the server.
 */
void tcp_dbus_client_disconnect(dbus_clnt_id clnt_id);

/**
 * After calling tcp_dbus_client_create() and obtaining a unique 
 * ID, this function may be called to no longer allocate any 
 * resources for that client. Deallocates client's config struct.
 * If tcp_dbus_client_init() was called for that clnt_id, then
 * tcp_dbus_client_disconnect() must be called prior to this function.
 * 
 * Non-blocking
 */
void tcp_dbus_client_delete(dbus_clnt_id clnt_id);

#endif /* PUB_DBUS_TCP_CLNT_INTERFACE_H */
