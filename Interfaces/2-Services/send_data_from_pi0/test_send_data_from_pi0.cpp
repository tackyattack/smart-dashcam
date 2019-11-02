/* This file is compiled and run as to test receiving data from the test_send_data_pi0 program. This program runs
    on the main dashcam unit (RPi3) and the test_send_data_pi0 runs on the aux unit (RPi Zero) */


/*-------------------------------------
|           PRIVATE INCLUDES           |
--------------------------------------*/

#include "test_send_data_from_pi0.h"
#include <iostream>
#include <fstream>
#include <mutex>
#include <queue>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include <vector>
#include <assert.h>

using namespace std;

/*-------------------------------------
|               DEFINES                |
--------------------------------------*/
#define DATA_SZ              (1000)
#define STATUS_CONNECTED     (true)
#define STATUS_NOT_CONNECTED (false)

/*-------------------------------------
|            MESSAGE STRUCT            |
--------------------------------------*/
/* struct is used for message processing and queueing of received messages */
struct msg_struct
{
    char* data;
    uint data_sz;
};

/*-------------------------------------
|           STATIC VARIABLES           |
--------------------------------------*/
static dbus_clnt_id id;
static char* data = NULL;

static std::mutex mutex_queue;
static std::queue<msg_struct*> data_queue;

static std::mutex mutex_connected_status;
static bool _connected_status = STATUS_NOT_CONNECTED;

static char msg_to_server__give_data      = 0xFE;
static char msg_from_server__request_data = 0xFF;
static char msg_hi[] = "HI";


/*-------------------------------------
|     MAIN FUNCTION OF THE SERVICE     |
--------------------------------------*/


/*-------------------------------------
|              CALLBACKS               |
--------------------------------------*/

/* https://isocpp.org/wiki/faq/mixing-c-and-cpp */

/**
 * This is the universal signal received callback used when a signal is received from dbus server.
 * Use this function prototype when subscribing to signals using tcp_dbus_client_Subscribe2Recv().
 * 
 * When a socket message is received, the dbus server emits signal DBUS_TCP_RECV_SIGNAL. We catch 
 * that signal and call this callback with the socket message sender's uuid, data array, and the
 * size of the data array. tcp_clnt_uuid is a null terminated string, data contains the data received
 * from the tcp client, and data_sz is the size of data. For socket clients, tcp_clnt_uuid == NULL.
 * 
 * When a tcp server client connects or disconects, the dbus server emits signal DBUS_TCP_CONNECT_SIGNAL,
 * or DBUS_TCP_DISCONNECT_SIGNAL. We catch these signals after having subscribed to them and when one
 * is caught, this callback is called with the uuid (tcp_clnt_uuid) of the client that
 * connected/disconnected. tcp_clnt_uuid is a null terminated string, data == NULL, and data_sz == 0 for
 * all signals received via this callback. data and data_sz are not used for these 2 signals but were left
 * in this callback for simplicity.
 */
//  typedef void (*tcp_rx_signal_callback)(const char* tcp_clnt_uuid, const char* data, unsigned int data_sz);

/** Note that data is freed after this callback is called. As such, if 
 * the data in data needs to be saved, a copy of the data must be made.
 * Refer to this guide on mixing C and C++ callbacks due to ldashcam_tcp_dbus_clnt
 *  being written in C: https://isocpp.org/wiki/faq/mixing-c-and-cpp */
void tcp_disconnected_from_srv_cb(const char* tcp_clnt_uuid, const char* data, unsigned int data_sz)
{
    printf("\n****************disconnected_from_srv: callback activated.****************\n\n");

    printf("Disconnected from server\n");

    assert(tcp_clnt_uuid != NULL);
    assert(data != NULL);
    assert(data_sz != 0);

    /* Update our connection status with server */
    mutex_connected_status.lock();
    _connected_status = STATUS_NOT_CONNECTED;
    mutex_connected_status.unlock();

  	printf("\n****************END---disconnected_from_srv---END****************\n\n");
}
/** Note that data is freed after this callback is called. As such, if 
 * the data in data needs to be saved, a copy of the data must be made.
 * Refer to this guide on mixing C and C++ callbacks due to ldashcam_tcp_dbus_clnt
 *  being written in C: https://isocpp.org/wiki/faq/mixing-c-and-cpp */
void tcp_rx_data_callback(const char* tcp_clnt_uuid, const char* data, unsigned int data_sz)
{
    printf("\n****************rx_data: callback activated.****************\n\n");
    assert(tcp_clnt_uuid != NULL);
    assert(data == NULL);
    assert(data_sz == 0);

    /*-------------------------------------
    |        SET CONNECTION STATUS         |
    --------------------------------------*/
    /* Update our connection status with server and say HI if previously weren't connected */
    mutex_connected_status.lock();
    if ( _connected_status == STATUS_NOT_CONNECTED )
    {
        printf("Connected to server\n");
        printf("Send %s to server...", msg_hi);
        /* Say hi to the server */
        if( false == tcp_dbus_send_msg(id, NULL, msg_hi, strlen(msg_hi)) )
        {
            printf("FAILED\n");
            _connected_status = STATUS_NOT_CONNECTED;
        }
        else
        {
            printf("SUCCEEDED\n");
            _connected_status = STATUS_NOT_CONNECTED;
        }
    }
    mutex_connected_status.unlock();

    /*-------------------------------------
    |   SAVE MSG TO QUEUE FOR PROCESSING   |
    --------------------------------------*/
    msg_struct *msg;
    msg = new msg_struct;

    msg->data_sz = data_sz;

    /* copy data received to msg struct char* */
    msg->data = (char*)malloc(data_sz);
    memcpy(msg->data, data, data_sz);
    
    /* Add received data msg struct to queue to be processed by main thread */
    mutex_queue.lock();
    data_queue.push(msg);
    mutex_queue.unlock();

  	printf("\n****************END---rx_data---END****************\n\n");
} /* tcp_rx_data_callback() */

void process_recv_data(msg_struct *msg)
{
    printf("\n****************process_recv_data****************\n\n");
    /*-------------------------------------
    |             VERIFICATION             |
    --------------------------------------*/
    if( msg == NULL )
    {
        return;
    }

    if( msg->data[0] == msg_from_server__request_data ) /* check to see if first byte (the command byte) signifies this is the data requested */
    {
        /* Write the data and millisecond timestamp to file. Ignore if failed to send */
        tcp_dbus_send_msg(id, NULL, data ,DATA_SZ);
    }
    
    if( strcmp(msg->data, msg_hi) )
    {
        printf("Server says HI!\n" );
    }
  	printf("\n****************END---process_recv_data---END****************\n\n");
} /* process_recv_data() */

/*-------------------------------------
|                 MAIN                 |
--------------------------------------*/

/* used to detect cntl + c to exit program safely */
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
    dbus_clnt_id id;
    int val;
    const char* server_version;
    msg_struct* msg;


    /*-------------------------------------
    |           INITIALIZATIONS            |
    --------------------------------------*/

    data = (char*)malloc(DATA_SZ);
    data[0] = msg_to_server__give_data;
    for (size_t i = 1; i < DATA_SZ; i++)
    {
        data[i] = 'a';
    }
    data[DATA_SZ-1] = '\0';

    id = tcp_dbus_client_create();

    /* Attempt to initialize the dbus_client until successful */
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

    printf("TCP Server version is v%s\n", server_version);


    /*-------------------------------------
    |      SUBSCRIBE CALLBACK SIGNALS      |
    --------------------------------------*/
    printf("Subscribe to DBUS_TCP_RECV_SIGNAL signal\n"); /* Subscribing to RECV_SIGNAL will make our rx_data_callback activate on a received tcp message */
    if ( EXIT_FAILURE == tcp_dbus_client_Subscribe2Recv( id, (char*)DBUS_TCP_RECV_SIGNAL, &tcp_rx_data_callback) )
    {
        printf("Failed to subscribe to DBUS_TCP_RECV_SIGNAL signal!\nExiting.....\n");
        exit(EXIT_FAILURE);
    }

    printf("Subscribe to DBUS_TCP_DISCONNECT_SIGNAL signal\n");/* Subscribing to DISCONNECT_SIGNAL will make our connect_callback activate when a tcp client/aux device disconnects */
    if ( EXIT_FAILURE == tcp_dbus_client_Subscribe2Recv(id,(char*)DBUS_TCP_DISCONNECT_SIGNAL,&tcp_disconnected_from_srv_cb) )
    {
        printf("Failed to subscribe to DBUS_TCP_DISCONNECT_SIGNAL signal!\nExiting.....\n");
        exit(EXIT_FAILURE);
    }


    /*-------------------------------------
    |      INFINITE LOOP/MAIN PROGRAM      |
    --------------------------------------*/

    /* Say hi to server. Ignore return value/if failed */
    tcp_dbus_send_msg( id, NULL, msg_hi, strlen(msg_hi) );

    while(1)
    {

        /* sleep 1ms */
        usleep(1000);

        /* Get msg from queue to process if queue isn't empty */
        /* If nothing to process, go to next loop iteration */
        mutex_queue.lock();
        if(data_queue.empty() == false)
        {
            msg = data_queue.front();
            data_queue.pop();
        }
        else
        {
            mutex_queue.unlock();
            continue;
        }
        mutex_queue.unlock();

        /* ensure msg isn't null */
        if(msg == NULL)
        {
            continue;
        }

        process_recv_data(msg);

        delete msg;
    }


    /*-------------------------------------
    |       SAFE SHUTDOWN/DISCONNECT       |
    --------------------------------------*/
    printf("Unsubscribe from signals\n");
    tcp_dbus_client_UnsubscribeRecv(id,(char*)DBUS_TCP_RECV_SIGNAL);
    tcp_dbus_client_UnsubscribeRecv(id,(char*)DBUS_TCP_DISCONNECT_SIGNAL);

    tcp_dbus_client_disconnect(id);
    tcp_dbus_client_delete(id);

    return 0;
}
