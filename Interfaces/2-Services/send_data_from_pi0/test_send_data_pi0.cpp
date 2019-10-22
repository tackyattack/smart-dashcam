/* This file is compiled and run as the DBUS server system service for the dashcam project.
    The DBUS server handles all debus connections for the project including TCP/IP access, GPS sensor access,
    and accelerometer access. For information on the different interfaces, see the Networking/DBUS/pub_dbus_srv.h
    file in the server_introspection_xml static variable. Use the pub_dbus_*_clnt.h files for access dbus interfaces */


/*-------------------------------------
|           PRIVATE INCLUDES           |
--------------------------------------*/

#include "test_send_data_pi0.h"
// #include <iostream>
// #include <fstream>
// #include <mutex>
// #include <queue>
#include <stdio.h>
#include <signal.h>
// #include <string.h>
// #include <sys/time.h>

using namespace std;

/*-------------------------------------
|           STATIC VARIABLES           |
--------------------------------------*/
// static std::ofstream testFile;
// static std::mutex mutex_queue;
// static std::queue<char*> data_queue;
static char* data;
static dbus_clnt_id id;
/*-------------------------------------
|     MAIN FUNCTION OF THE SERVICE     |
--------------------------------------*/


/*-------------------------------------
|              CALLBACKS               |
--------------------------------------*/

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
//  typedef void (*tcp_rx_signal_callback)(const char* tcp_clnt_uuid, const char* data, unsigned int data_sz);

/** Note that data is freed after this callback is called. As such, if 
 * the data in data needs to be saved, a copy of the data must be made.
 * Refer to this guide on mixing C and C++ callbacks due to ldashcam_tcp_dbus_clnt
 *  being written in C: https://isocpp.org/wiki/faq/mixing-c-and-cpp */
void tcp_rx_data_callback(const char* tcp_clnt_uuid, const char* data, unsigned int data_sz)
{
    //   printf("\n****************tcp_rx_data_callback: callback activated.****************\n\n");
    // char *temp = (char*)malloc(data_sz);
    // memcpy(temp,data,data_sz);

    // mutex_queue.lock();
    // data_queue.push(temp);
    // mutex_queue.unlock();

    printf("Received %d bytes from client %s as follows:\n\"",data_sz, tcp_clnt_uuid);
    for (size_t i = 0; i < data_sz; i++)
    {
        printf("%c",data[i]);
    }
    printf("\"\n");
    
    //   printf("\n****************END---tcp_rx_data_callback---END****************\n\n");
} /* tcp_rx_data_callback() */

// void process_recv_data(char* data, uint data_sz)
// {
//     //Process commands here to do things
// }

/*-------------------------------------
|                 MAIN                 |
--------------------------------------*/

void  INThandler(int sig)
{
    signal(sig, SIG_IGN);
    printf("\nCtrl-C detected. Exiting....\n");
    /*-------------------------------------
    |               SHUTDOWN               |
    -------------------------------------*/
    tcp_dbus_client_disconnect(id);
    tcp_dbus_client_delete(id);
    exit(EXIT_SUCCESS);
}

int main(void)
{
    /*----------------------------------
    |       SETUP SIGNAL HANDLERS       |
    ------------------------------------*/
    struct sigaction act_sigint;
    act_sigint.sa_handler = INThandler;
    sigaction(SIGINT, &act_sigint, NULL);

    /*-------------------------------------
    |              VARIABLES               |
    -------------------------------------*/
    int val;
    const char* server_version;
    // char* data;
    // struct timeval tv;

    /*-------------------------------------
    |           INITIALIZATIONS            |
    -------------------------------------*/
    // testFile.open ("command_control.log");

    id = tcp_dbus_client_create();

    do
    {
        val = tcp_dbus_client_init(id, &server_version);
        if ( EXIT_FAILURE == val )
        {
            printf("Failed to initialize client!\nExiting.....\n");
            exit(EXIT_FAILURE);
        }
        else if ( val == DBUS_SRV_NOT_AVAILABLE )
        {
            printf("WARNING: DBUS server is unavailable. Trying again in 2 seconds...\n");
            sleep(2);
        }
    }while( EXIT_SUCCESS != val );


    /*-------------------------------------
    |          CLIENT OPERATIONS           |
    -------------------------------------*/

    printf("Testing server interface v%s\n", server_version);
    
    printf("Subscribe to DBUS_TCP_RECV_SIGNAL signal\n");
    if ( EXIT_FAILURE == tcp_dbus_client_Subscribe2Recv( id, (char*)DBUS_TCP_RECV_SIGNAL, &tcp_rx_data_callback) )
    {
        printf("Failed to subscribe to DBUS_TCP_RECV_SIGNAL signal!\nExiting.....\n");
        exit(EXIT_FAILURE);
    }

    // printf("Subscribe to DBUS_TCP_CONNECT_SIGNAL signal\n");
    // if ( EXIT_FAILURE == tcp_dbus_client_Subscribe2Recv(id,DBUS_TCP_CONNECT_SIGNAL,&tcp_clnt_connect_callback) )
    // {
    //     printf("Failed to subscribe to DBUS_TCP_CONNECT_SIGNAL signal!\nExiting.....\n");
    //     EXIT_FAILURE;
    // }
    // printf("Subscribe to DBUS_TCP_DISCONNECT_SIGNAL signal\n");
    // if ( EXIT_FAILURE == tcp_dbus_client_Subscribe2Recv(id,DBUS_TCP_DISCONNECT_SIGNAL,&tcp_clnt_disconnect_callback) )
    // {
    //     printf("Failed to subscribe to DBUS_TCP_DISCONNECT_SIGNAL signal!\nExiting.....\n");
    //     EXIT_FAILURE;
    // }

    #define DATA_SZ    (1000)

    data = (char*)malloc(DATA_SZ);

    for (size_t i = 0; i < DATA_SZ; i++)
    {
        data[i] = 'a';
    }
    data[DATA_SZ-1] = '\0';


    /*-------------------------------------
    |            INFINITE LOOP             |
    --------------------------------------*/
    while(1)
    {
        usleep(50000);
        // mutex_queue.lock();
        // if(data_queue.empty() == false)
        // {
        //     data = data_queue.front();
        //     data_queue.pop();
        // }
        // mutex_queue.unlock();

        // gettimeofday(&tv, 0);
        // /* Write timestamp to file in milliseconds */
        // testFile << (tv.tv_usec / 1000 + tv.tv_sec * 1000) << endl;
        // /* Write data to file */
        // testFile << *data << endl;
        // free(data);
        if( tcp_dbus_send_msg(id, NULL, data, DATA_SZ) == false )
        {
            printf("Failed to send data");
        }
    }

    free(data);
    data = NULL;

    printf("Unsubscribe from signals\n");
    tcp_dbus_client_UnsubscribeRecv(id,(char*)DBUS_TCP_RECV_SIGNAL);
    tcp_dbus_client_UnsubscribeRecv(id,(char*)DBUS_TCP_CONNECT_SIGNAL);
    tcp_dbus_client_UnsubscribeRecv(id,(char*)DBUS_TCP_DISCONNECT_SIGNAL);

    // sleep(10);


    /*-------------------------------------
    |               SHUTDOWN               |
    -------------------------------------*/

    tcp_dbus_client_disconnect(id);
    tcp_dbus_client_delete(id);

    return 0;
}
