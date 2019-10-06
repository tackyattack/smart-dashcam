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
 * and returns that servers unique ID. init_server must be called 
 * with the return unique ID to initialize the server. This function
 * will assert if attempting to create more than MAX_NUM_SERVERS.
 * Non-blocking
 */
dbus_srv_id create_server();

/**
 * initializes a DBUS server for the config passed in.
 * config should be a pointer to an allocated dbus_srv_config 
 * struct that contains null entries and should exist until 
 * kill_server() is called.
 * Each struct dbus_srv_config represents a unique and 
 * independent server.
 * Non-blocking
 */
int init_server(dbus_srv_id srv_id);

/**
 * This function, given a struct dbus_srv_config that has 
 * been setup by init_server(), will run the g_main_loop()
 * required for the server to send/receive dbus messages.
 * 
 * Non-blocking
 */
int execute_server(dbus_srv_id srv_id);

/**
 * This function should be called after init_server()
 * and execute_server() to safely quit the dbus server.
 * init_server() must be called again if desiring to 
 * restart server with same dbus_srv_id afer calling
 * kill_server()
 * 
 * Non-blocking
 */
void kill_server(dbus_srv_id srv_id);


/**
 * After calling create_server() and obtaining a unique ID,
 * this function may be called to no longer allocate any 
 * resources for that server. Deallocates server's config 
 * struct. If init_server() was called for that server, 
 * then kill_server() must be called prior to this function.
 * 
 * Non-blocking
 */
void delete_server(dbus_srv_id srv_id);


#endif /* PUB_DBUS_SRV_INTERFACE_H */
