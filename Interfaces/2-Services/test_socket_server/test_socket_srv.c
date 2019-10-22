/*-------------------------------------
|           PRIVATE INCLUDES           |
--------------------------------------*/

#include "test_socket_srv.h"


/*-------------------------------------
|          CALLBACK FUNCTIONS          |
--------------------------------------*/

// From the pub_socket_server.h
// /* https://isocpp.org/wiki/faq/mixing-c-and-cpp */
// typedef void (*socket_lib_srv_rx_msg)(const char* uuid, const char* data, const unsigned int data_sz);
// typedef void (*socket_lib_srv_connected)(const char* uuid);
// typedef void (*socket_lib_srv_disconnected)(const char* uuid);

/** Note that data is freed after this callback is called. As such, if 
 * the data in data needs to be saved, a copy of the data must be made.
 * Refer to this guide on mixing C and C++ callbacks due to library
 *  being written in C: https://isocpp.org/wiki/faq/mixing-c-and-cpp */
void recv_msg(const char* uuid, const char *data, const unsigned int data_sz)
{
  	printf("\n****************recv_msg: callback activated.****************\n\n");

    printf("Message from the following UUID: %s\n", uuid);
    printf("Received %d data bytes as follows:\n\"",data_sz);
    for (size_t i = 0; i < data_sz; i++)
    {
        printf("%c",data[i]);
    }
    printf("\"\n");
    
  	printf("\n****************END---recv_msg---END****************\n\n");
} /* recv_msg() */

void client_connect(const char* uuid)
{
  	printf("\n****************client_connect: callback activated.****************\n\n");

    printf("Client connected -> UUID: %s\n", uuid);

  	printf("\n****************END---client_connect---END****************\n\n");
} /* client_connect() */

void client_disconnect(const char* uuid)
{
  	printf("\n****************client_disconnect: callback activated.****************\n\n");

    printf("Client disconnected -> UUID: %s\n", uuid);

  	printf("\n****************END---client_disconnect---END****************\n\n");
} /* client_disconnect() */

/*--------------------------------------
|     MAIN FUNCTIONS OF THE SERVICE     |
---------------------------------------*/

// For testing only
char* check_parameters(int argc, char *argv[])
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/

    char* port;


    /*----------------------------------
    |          CHECK ARGUMENTS          |
    ------------------------------------*/

    /* Verify proper number of arguments and set port number for Server to listen on
        Note that argc = 1 means no arguments*/
    if (argc < 2)
    {
        printf("WARNING, no port provided, defaulting to %s\n", "5555");

        /* No port number provided, use default */
        port = (char*)"5555";
    }
    else if (argc > 2)
    {
        fprintf(stderr, "ERROR, too many arguments!\n 0 or 1 arguments expected. Expected port number!\n");
        exit(EXIT_FAILURE);
    }
    else /* 1 argument */
    {
        /* Test that argument is valid */
        if ( atoi(argv[1]) < 0 || atoi(argv[1]) > 65535 )
        {
            printf("ERROR: invalid port number %s!",argv[1]);
            exit(EXIT_FAILURE);
        }

        /* Get port number of server from the arguments */
        port = argv[1];
    }

    return port;
} /* check_parameters() */


static char  longStr[] = "Server testing very long message to client\nServer testing very long message to client\nServer testing very long message to client\nServer testing very long message to client\nServer testing very long message to client\nServer testing very long message to client\nServer testing very long message to client\nServer testing very long message to client\nServer testing very long message to client\nServer testing very long message to client\nServer testing very long message to client\nServer testing very long message to client\nServer testing very long message to client\nServer testing very long message to client\nServer testing very long message to client\nServer testing very long message to client\n";

// For testing only
int main(int argc, char *argv[])
{
    /*----------------------------------
    |             VARIABLES             |
    ------------------------------------*/
    char* port;   /* Port number for socket server */
    

    /*----------------------------------
    |            CHECK INPUT            |
    ------------------------------------*/
    port = check_parameters(argc, argv);


    /*----------------------------------
    |            START SERVER           |
    ------------------------------------*/
    if( 0 > socket_server_init(port, recv_msg, client_connect, client_disconnect) )
    {
        printf("Failed to init server\n");
        exit(EXIT_FAILURE);
    }

    /* Run the server.  Accept connection requests, 
        and receive messages. Run forever. */
    socket_server_execute();

    if(socket_server_is_executing() == false)
    {
        printf("Failed: server not executing\n");
        exit(EXIT_FAILURE);
    }

    sleep(10);

    if ( 13 > socket_server_send_data_all("TEST MESSAGE",13) )
    {
        printf("Failed: server did not send all bytes of the message\n");
        exit(EXIT_FAILURE);
    }

    sleep(3);

    printf("\ntest_socket_server: test sending long message of %zu bytes.\n", strlen(longStr));
    if ( (int)strlen(longStr) > socket_server_send_data_all(longStr, strlen(longStr)) )
    {
        printf("Failed: server did not send all bytes of the message\n");
        exit(EXIT_FAILURE);
    }

    printf("\ntest_socket_server: test sending long message of %zu bytes.\n", strlen(longStr));
    if ( (int)strlen(longStr) > socket_server_send_data_all(longStr, strlen(longStr)) )
    {
        printf("Failed: server did not send all bytes of the message\n");
        exit(EXIT_FAILURE);
    }

    printf("Press enter to quit server...\n");
    /* block until ready to quit */
    getchar();

    socket_server_quit();
    
    sleep(2);
    
    if(socket_server_is_executing() == true)
    {
        printf("Failed: server is still executing\n");
        exit(EXIT_FAILURE);
    }

    /* If we ever return from socket_server_execute, there was a serious error */
    return 1;
} /* main() */
