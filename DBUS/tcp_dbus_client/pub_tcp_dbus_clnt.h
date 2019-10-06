#ifndef PUB_DBUS_TCP_CLNT_INTERFACE_H
#define PUB_DBUS_TCP_CLNT_INTERFACE_H


/*-------------------------------------
|           STATIC VARIABLES           |
-------------------------------------*/

static const char *server_version;


/*-------------------------------------
|            PUBLIC STRUCTS            |
--------------------------------------*/

// struct dbus_subscriber
// {
//     void* Callback;
//     // void* Callback_Data;
// };

/*-------------------------------------
|    FUNCTION POINTER DECLARATIONS     |
--------------------------------------*/
/* https://isocpp.org/wiki/faq/mixing-c-and-cpp */
typedef void (*tcp_rx_signal_callback)(char* data, unsigned int data_sz);

/*-------------------------------------
|    COMMAND FUNCTION DECLARATIONS     |
-------------------------------------*/

void test_Ping(void);

void test_Echo(void);

void test_CommandEmitSignal(void);

void test_Quit(void);


/*-------------------------------------
|     PUBLIC FUNCTION DECLARATIONS     |
--------------------------------------*/

/** 
 * Given a pointer to an allocated dbus_clnt_config
 * struct with valid parameter values, will initialize
 * the dbus client.
 * 
 * Non-blocking call
 */
int init_client(void);

/**
 * Given a pointer to an allocated dbus_subscriber struct 
 * with valid parameter values, will subscribe to specified
 * dbus_subscriber->SignalName. When a signal/message is received
 * from the server, the callback function will be called from
 * separate thread. 
 */
int Subscribe2Server(tcp_rx_signal_callback callback);

/** Given a pointer to a dbus_subscriber struct that has been
 * passed to Subscribe2Server(), which has successfully run,
 * this function will unsubscribe so that callbacks will no
 * longer happen and any signals received for the signal 
 * subscribed to will be ignored.
 */
int UnsubscribeFromServer(void);

/** Given a pointer to a dbus_clnt_config struct that has been
 * passed to init_client(), which successfully run, 
 * this function will close the dbus connection to the server.
 * */
void disconnect_client(void);

#endif /* PUB_DBUS_TCP_CLNT_INTERFACE_H */
