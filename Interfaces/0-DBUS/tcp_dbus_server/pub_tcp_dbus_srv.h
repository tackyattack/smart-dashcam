#ifndef PUB_DBUS_SRV_INTERFACE_H
#define PUB_DBUS_SRV_INTERFACE_H

/*-------------------------------------
|               INCLUDES               |
--------------------------------------*/

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>


/*-------------------------------------
|            PUBLIC DEFINES            |
--------------------------------------*/

/** Because there can only be one server for a given unique server name, 
 * and because the server_init function doesn't take server name parameters, 
 * we can only have one server at a time */
#define MAX_NUM_SERVERS     (1) /* (__UINT8_MAX__) */


/*-------------------------------------
|           PUBLIC TYPEDEFS            |
--------------------------------------*/

typedef uint8_t dbus_srv_id;


/*-------------------------------------
|    FUNCTION POINTER DECLARATIONS     |
--------------------------------------*/

/* https://isocpp.org/wiki/faq/mixing-c-and-cpp */
typedef void (*tcp_send_msg_callback)(char* data, unsigned int data_sz);


/*-------------------------------------
|           STATIC CONSTANTS           |
--------------------------------------*/

static const char *srv_sftw_version = "0.1";


/*-------------------------------------
|     PUBLIC FUNCTION DECLARATIONS     |
--------------------------------------*/

/**
 * This function allocates a configuration struct for a unique server
 * and returns that servers unique ID. tcp_dbus_srv_init must be called 
 * with the return unique ID to initialize the server. This function
 * will assert if attempting to create more than MAX_NUM_SERVERS.
 * 
 * Non-blocking
 */
dbus_srv_id tcp_dbus_srv_create();

/**
 * Initializes a DBUS server for a given dbus_srv_id 
 * give by tcp_dbus_srv_create() and optionally NULL callback
 * Each dbus_srv_id represents a unique and independent server.
 * callback parameter is a function pointer to the user callback
 * that will be called when the send_msg() method is called by a 
 * tcp dbus client.
 * 
 * Non-blocking
 */
int tcp_dbus_srv_init(dbus_srv_id srv_id, tcp_send_msg_callback callback);

/**
 * This function, given a dbus_srv_id that has 
 * been created by tcp_dbus_srv_create(), will run 
 * the g_main_loop() required for the server to 
 * send/receive dbus messages.
 * 
 * Non-blocking
 */
int tcp_dbus_srv_execute(dbus_srv_id srv_id);

/**
 * This function should be called after tcp_dbus_srv_init()
 * and tcp_dbus_srv_execute() to safely quit the dbus server.
 * tcp_dbus_srv_init() must be called again if desiring to 
 * restart server with same dbus_srv_id after calling
 * tcp_dbus_srv_kill()
 * 
 * Non-blocking
 */
void tcp_dbus_srv_kill(dbus_srv_id srv_id);

/**
 * After calling tcp_dbus_srv_create() and obtaining a unique ID,
 * this function may be called to no longer allocate any 
 * resources for that server. Deallocates server's config 
 * struct. If tcp_dbus_srv_init() was called for that server, 
 * then tcp_dbus_srv_kill() must be called prior to this function.
 * 
 * Non-blocking
 */
void tcp_dbus_srv_delete(dbus_srv_id srv_id);


/*-------------------------------------
|     PUBLIC EMIT SIGNAL FUNCTIONS     |
--------------------------------------*/

/**
 * Calling this function (with a valid created, initialized, and executing 
 * dbus_srv_id from tcp_dbus_srv_create(), tcp_dbus_srv_init(), and 
 * tcp_dbus_srv_execute() functions) will emit a signal to all tcp dbus clients
 * who are subscribed to this signal. The signal emitted contains the msg given 
 * to this function. This signal represents the tcp server receiving a tcp message,
 * and passes that message to all subscribers (subscriber-publisher setup).
 * 
 * Non-blocking
 */
bool tcp_dbus_srv_emit_msg_recv_signal(dbus_srv_id srv_id, const char *msg, uint msg_sz);

/**
 * Calling this function (with a valid created, initialized, and executing 
 * dbus_srv_id from tcp_dbus_srv_create(), tcp_dbus_srv_init(), and 
 * tcp_dbus_srv_execute() functions) will emit a signal to all tcp 
 * dbus clients who are subscribed to this signal. The signal emitted to
 * tcp dbus clients  contains the tcp client id for the client that connected 
 * to the tcp server given to this function. This signal represents a new tcp
 * client connection to the tcp server and passes that new clients id to all
 * subscribers (subscriber-publisher setup).
 * 
 * Non-blocking
 */
bool tcp_dbus_srv_emit_connect_signal(dbus_srv_id srv_id, const char *tcp_clnt_id, uint size);

/**
 * Calling this function (with a valid created, initialized, and executing 
 * dbus_srv_id from tcp_dbus_srv_create(), tcp_dbus_srv_init(), and 
 * tcp_dbus_srv_execute() functions) will emit a signal to all tcp 
 * dbus clients who are subscribed to this signal. The signal emitted to
 * tcp dbus clients  contains the tcp client id for the client that disconnected 
 * to the tcp server given to this function. This signal represents a new tcp
 * client disconnection to the tcp server and passes that new clients id to all
 * subscribers (subscriber-publisher setup).
 * 
 * Non-blocking
 */
bool tcp_dbus_srv_emit_disconnect_signal(dbus_srv_id srv_id, const char *tcp_clnt_id, uint size);


#endif /* PUB_DBUS_SRV_INTERFACE_H */
