/* This file is compiled and run as to test receiving data from the test_send_data_pi0 program. This program runs
    on the main dashcam unit (RPi3) and the test_send_data_pi0 runs on the aux unit (RPi Zero) */


/*-------------------------------------
|           PRIVATE INCLUDES           |
--------------------------------------*/

#include "test_recv_data.h"
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

/* struct is used for message processing and queueing of received messages */
struct msg_struct
{
    std::string UUID;
    char* data;
    uint data_sz;
};

/*-------------------------------------
|           STATIC VARIABLES           |
--------------------------------------*/
static dbus_clnt_id id;
static std::ofstream testFile;

static std::mutex mutex_queue;
static std::queue<msg_struct*> data_queue;

static std::mutex mutex_specific_clnt;
static char* _specific_UUID = NULL;

static std::mutex mutex_connected_clnt;
static std::vector<char*> _connected_clients;

static char msg_from_client__give_data = 0xFE;
static char msg_to_client__request_data = 0xFF;
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
 * in this callback for simplicity. The connect callback is not used by tcp clients, only the disconnect.
 */
//  typedef void (*tcp_rx_signal_callback)(const char* tcp_clnt_uuid, const char* data, unsigned int data_sz);

/** Note that data is freed after this callback is called. As such, if 
 * the data in data needs to be saved, a copy of the data must be made.
 * Refer to this guide on mixing C and C++ callbacks due to ldashcam_tcp_dbus_clnt
 *  being written in C: https://isocpp.org/wiki/faq/mixing-c-and-cpp */
void tcp_clnt_connect_callback(const char* tcp_clnt_uuid, const char* data, unsigned int data_sz)
{
    printf("\n****************client_connect: callback activated.****************\n\n");
    printf("Client connected -> UUID: %s\n", tcp_clnt_uuid);

    assert(tcp_clnt_uuid != NULL);
    assert(data == NULL);
    assert(data_sz == 0);

    char* uuid;

    /* copy uuid */
    uuid = (char*)malloc( strlen(tcp_clnt_uuid)+1 );
    memcpy(uuid, tcp_clnt_uuid, strlen(tcp_clnt_uuid)+1);

    /* Add client UUID to our list of connected clients */
    mutex_connected_clnt.lock();
    _connected_clients.push_back( uuid );
    mutex_connected_clnt.unlock();

    /* Set this client as our specific client to request data from if no specific client has been selected */
    mutex_specific_clnt.lock();
    if ( _specific_UUID == NULL ) 
    {
        _specific_UUID = _connected_clients[0];

        /* Ask specific client for data by sending request for data message to that specific client. Ignore return value/if failed */
        printf("Ask specific %s client for data\n",(char*)_specific_UUID);
        tcp_dbus_send_msg( id, _specific_UUID, &msg_to_client__request_data, 1 );
    }
    mutex_specific_clnt.unlock();

    /* Say hi to all clients by sending message to all clients. Ignore return value/if failed */
    tcp_dbus_send_msg( id, NULL, msg_hi, strlen(msg_hi)+1 );

  	printf("\n****************END---client_connect---END****************\n\n");
}
/** Note that data is freed after this callback is called. As such, if 
 * the data in data needs to be saved, a copy of the data must be made.
 * Refer to this guide on mixing C and C++ callbacks due to ldashcam_tcp_dbus_clnt
 *  being written in C: https://isocpp.org/wiki/faq/mixing-c-and-cpp */
void tcp_clnt_disconnect_callback(const char* tcp_clnt_uuid, const char* data, unsigned int data_sz)
{
    printf("\n****************client_disconnect: callback activated.****************\n\n");

    assert(tcp_clnt_uuid != NULL);
    assert(data == NULL);
    assert(data_sz == 0);

    printf("Client disconnected -> UUID: %s\n", tcp_clnt_uuid);


    /* remove client UUID from our list if exists */
    mutex_connected_clnt.lock();
    auto i = _connected_clients.cbegin();
    for ( ; ( i != _connected_clients.cend() && strcmp(*i, tcp_clnt_uuid) != 0); ++i);

    if (i != _connected_clients.cend())
    {
        free(*i);
        _connected_clients.erase(i);
    }
    mutex_connected_clnt.unlock();

    /* If our specific client to request data from is no longer connected, set it null */
    mutex_specific_clnt.lock();
    if ( _specific_UUID != NULL && strcmp(_specific_UUID, tcp_clnt_uuid) == 0 ) 
    {
        _specific_UUID = NULL;
    }
    mutex_specific_clnt.unlock();

  	printf("\n****************END---client_disconnect---END****************\n\n");
}
/** Note that data is freed after this callback is called. As such, if 
 * the data in data needs to be saved, a copy of the data must be made.
 * Refer to this guide on mixing C and C++ callbacks due to ldashcam_tcp_dbus_clnt
 *  being written in C: https://isocpp.org/wiki/faq/mixing-c-and-cpp */
void tcp_rx_data_callback(const char* tcp_clnt_uuid, const char* data, unsigned int data_sz)
{
    printf("\n****************tcp_rx_data_callback: callback activated.****************\n\n");

    printf("Message from the following UUID: %s\n", tcp_clnt_uuid);
    printf("Received %d data bytes as follows:\n\"",data_sz);
    for (size_t i = 0; i < data_sz; i++)
    {
        printf("%c",data[i]);
    }
    printf("\"\n");

    assert(tcp_clnt_uuid != NULL);
    assert(data != NULL);
    assert(data_sz != 0);

    msg_struct *msg;
    msg = new msg_struct;

    msg->UUID = std::string(tcp_clnt_uuid);
    msg->data_sz = data_sz;


    /* copy data received to msg struct char* */
    msg->data = (char*)malloc(data_sz);
    memcpy(msg->data, data, data_sz);
    
    /* Add received data msg struct to queue to be processed by main thread */
    mutex_queue.lock();
    data_queue.push(msg);
    mutex_queue.unlock();

  	printf("\n****************END---tcp_rx_data_callback---END****************\n\n");
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

    /*-------------------------------------
    |              VARIABLES               |
    --------------------------------------*/
    struct timeval tv;
    bool is_data_recv = false;

    mutex_specific_clnt.lock();
    if( _specific_UUID != NULL && strcmp( _specific_UUID, msg->UUID.c_str() ) == 0 )
    {
        is_data_recv = true;
    }
    mutex_specific_clnt.unlock();

    if( is_data_recv == true && msg->data[0] == msg_from_client__give_data ) /* check to see if first byte (the command byte) signifies this is the data requested */
    {
        printf("Client %s is the specific UUID...",msg->UUID.c_str() );
        printf("Client gives data!\n" );

        gettimeofday(&tv,0);
        /* Write the data and millisecond timestamp to file */
        testFile << (unsigned long)((unsigned long)tv.tv_usec/1000 + (unsigned long) tv.tv_sec*1000) << "   ";
        testFile.write(msg->data,msg->data_sz);
        testFile << std::endl << std::endl;

        printf("Request more data from client %s!\n", msg->UUID.c_str() );
        /* Send command to client to send more data. Ignore return value/if failed */
        while ( false == tcp_dbus_send_msg(id, msg->UUID.c_str(), &msg_to_client__request_data ,1) )
        {
            mutex_specific_clnt.lock();
            if( _specific_UUID == NULL )
            {
                mutex_specific_clnt.unlock();
                break;
            }
            mutex_specific_clnt.unlock();
        }
    }
    else if( strcmp(msg->data, msg_hi) == 0 )
    {
        printf("Client %s says HI!\n",msg->UUID.c_str() );
        
        /* Check if msg hi is from specific client indicating it is ready to send data */
        if ( is_data_recv ) 
        {
            printf("Request data from client %s!\n", msg->UUID.c_str() );
            /* Send command to client to send more data. Ignore return value/if failed */
            while ( false == tcp_dbus_send_msg(id, msg->UUID.c_str(), &msg_to_client__request_data ,1) )
            {
                mutex_specific_clnt.lock();
                if( _specific_UUID == NULL )
                {
                    mutex_specific_clnt.unlock();
                    break;
                }
                mutex_specific_clnt.unlock();
            }
        }
    }
    else
    {
        printf("Msg from client %s:\n%s\n",msg->UUID.c_str(),msg->data);
    }
  	printf("\n****************END---process_recv_data---END****************\n\n");
}

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
    testFile.close();
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
    testFile.open("recv_data_file.txt");

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

    printf("Subscribe to DBUS_TCP_CONNECT_SIGNAL signal\n");/* Subscribing to CONNECT_SIGNAL will make our connect_callback activate when a tcp client/aux device connects */
    if ( EXIT_FAILURE == tcp_dbus_client_Subscribe2Recv(id,(char*)DBUS_TCP_CONNECT_SIGNAL,&tcp_clnt_connect_callback) )
    {
        printf("Failed to subscribe to DBUS_TCP_CONNECT_SIGNAL signal!\nExiting.....\n");
        exit(EXIT_FAILURE);
    }

    printf("Subscribe to DBUS_TCP_DISCONNECT_SIGNAL signal\n");/* Subscribing to DISCONNECT_SIGNAL will make our connect_callback activate when a tcp client/aux device disconnects */
    if ( EXIT_FAILURE == tcp_dbus_client_Subscribe2Recv(id,(char*)DBUS_TCP_DISCONNECT_SIGNAL,&tcp_clnt_disconnect_callback) )
    {
        printf("Failed to subscribe to DBUS_TCP_DISCONNECT_SIGNAL signal!\nExiting.....\n");
        exit(EXIT_FAILURE);
    }


    /*-------------------------------------
    |      INFINITE LOOP/MAIN PROGRAM      |
    --------------------------------------*/

    while(1)
    {

#if 0 /* for testing */
        sleep(1);
        /* Say hi to the server */
        if( false == tcp_dbus_send_msg(id, NULL, msg_hi, strlen(msg_hi)+1) )
        {
            printf("FAILED to send HI\n");
        }
        static uint8_t count = 0;
        tcp_dbus_send_msg(id, NULL, (char*)std::to_string(count).c_str(), std::to_string(count).length()+1);
        count++;
#else
        /* sleep 1ms */
        usleep(1000);    
#endif

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

        /* cleanup */
        free(msg->data);
        delete msg;
    }


    /*-------------------------------------
    |       SAFE SHUTDOWN/DISCONNECT       |
    --------------------------------------*/
    printf("Unsubscribe from signals\n");
    tcp_dbus_client_UnsubscribeRecv(id,(char*)DBUS_TCP_RECV_SIGNAL);
    tcp_dbus_client_UnsubscribeRecv(id,(char*)DBUS_TCP_CONNECT_SIGNAL);
    tcp_dbus_client_UnsubscribeRecv(id,(char*)DBUS_TCP_DISCONNECT_SIGNAL);

    tcp_dbus_client_disconnect(id);
    tcp_dbus_client_delete(id);

    return 0;
}
