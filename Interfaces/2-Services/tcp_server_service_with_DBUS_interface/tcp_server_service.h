#ifndef TCP_SERVER_SERVICE_H
#define TCP_SERVER_SERVICE_H

/*-------------------------------------
|               INCLUDES               |
--------------------------------------*/

#include <stdlib.h>
#include <unistd.h> /* for sleep */
#include <stdio.h>
#include <string.h> /* for strlen */
#include <stdbool.h>
#include <pthread.h>

#include <dashcam_sockets/pub_socket_server.h>
/* includes needed for server (This is found in the library's Makefile in the SYS_INC_DIR). Note that the header file includes are the pub headerfiles in the library source directory in DBUS/library_name */
#include <dashcam_dbus/pub_tcp_dbus_srv.h>
#include <dashcam_dbus/pub_dbus.h>

/*-------------------------------------
|    PRIVATE FUNCTION DECLARATIONS     |
--------------------------------------*/


#endif /* TCP_SERVER_SERVICE_H */