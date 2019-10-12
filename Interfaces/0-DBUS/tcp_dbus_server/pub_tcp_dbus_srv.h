#ifndef PUB_DBUS_SRV_INTERFACE_H
#define PUB_DBUS_SRV_INTERFACE_H

/*-------------------------------------
|               INCLUDES               |
--------------------------------------*/

#include <stdlib.h>
#include <stdint.h>


/*-------------------------------------
|            PUBLIC DEFINES            |
--------------------------------------*/

#define MAX_NUM_SERVERS __UINT8_MAX__


/*-------------------------------------
|           PUBLIC TYPEDEFS            |
--------------------------------------*/

typedef uint8_t dbus_srv_id;


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
 * Non-blocking
 */
dbus_srv_id tcp_dbus_srv_create();

/**
 * initializes a DBUS server for the config passed in.
 * config should be a pointer to an allocated dbus_srv_config 
 * struct that contains null entries and should exist until 
 * tcp_dbus_srv_kill() is called.
 * Each struct dbus_srv_config represents a unique and 
 * independent server.
 * Non-blocking
 */
int tcp_dbus_srv_init(dbus_srv_id srv_id);

/**
 * This function, given a struct dbus_srv_config that has 
 * been setup by tcp_dbus_srv_init(), will run the g_main_loop()
 * required for the server to send/receive dbus messages.
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

#endif /* PUB_DBUS_SRV_INTERFACE_H */
