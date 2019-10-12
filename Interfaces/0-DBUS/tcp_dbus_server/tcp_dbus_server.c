/*-------------------------------------
|           PRIVATE INCLUDES           |
--------------------------------------*/

#include "prv_tcp_dbus_srv.h"

/**
 * This implements 'Get' method of DBUS_INTERFACE_PROPERTIES so a
 * client can inspect the properties/attributes of 'TestInterface'.
 */
DBusHandlerResult server_get_properties_handler(const char *property, DBusConnection *conn, DBusMessage *reply)
{
    if (!strcmp(property, "Version")) 
    {
        dbus_message_append_args(reply, DBUS_TYPE_STRING, &srv_sftw_version, DBUS_TYPE_INVALID);
    } 
    else
    {
        /* Unknown property */
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
    if (!dbus_connection_send(conn, reply, NULL))
    {
        return DBUS_HANDLER_RESULT_NEED_MEMORY;
    }
    return DBUS_HANDLER_RESULT_HANDLED;
} /* server_get_properties_handler */

/**
 * This implements 'GetAll' method of DBUS_INTERFACE_PROPERTIES. This
 * one seems required by g_dbus_proxy_get_cached_property().
 */
DBusHandlerResult server_get_all_properties_handler(DBusConnection *conn, DBusMessage *reply)
{
    DBusHandlerResult result;
    DBusMessageIter array, dict, iter, variant;
    const char *property = "Version";

    /*
     * All dbus functions used below might fail due to out of
     * memory error. If one of them fails, we assume that all
     * following functions will fail too, including
     * dbus_connection_send().
     */
    result = DBUS_HANDLER_RESULT_NEED_MEMORY;

    dbus_message_iter_init_append(reply, &iter);
    dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &array);

    /* Append all properties name/value pairs */
    property = "Version";
    dbus_message_iter_open_container(&array, DBUS_TYPE_DICT_ENTRY, NULL, &dict);
    dbus_message_iter_append_basic(&dict, DBUS_TYPE_STRING, &property);
    dbus_message_iter_open_container(&dict, DBUS_TYPE_VARIANT, "s", &variant);
    dbus_message_iter_append_basic(&variant, DBUS_TYPE_STRING, &srv_sftw_version);
    dbus_message_iter_close_container(&dict, &variant);
    dbus_message_iter_close_container(&array, &dict);

    dbus_message_iter_close_container(&iter, &array);

    if (dbus_connection_send(conn, reply, NULL))
    {
        result = DBUS_HANDLER_RESULT_HANDLED;
    }

    return result;
} /* server_get_all_properties_handler() */

dbus_srv_id tcp_dbus_srv_create()
{
    /*-------------------------------------
    |              VARIABLES               |
    --------------------------------------*/

    dbus_srv_id new_srv_id;


    /*-------------------------------------
    |       FIND FIRST AVAILABLE ID        |
    --------------------------------------*/

    for(new_srv_id = 0; new_srv_id < MAX_NUM_SERVERS && SRV_CONFIGS_ARRY[new_srv_id] != NULL; new_srv_id++);


    /*-------------------------------------
    |        VERIFY ID IS AVAILABLE        |
    --------------------------------------*/

    if ( new_srv_id == MAX_NUM_SERVERS || SRV_CONFIGS_ARRY[new_srv_id] != NULL )
    {
        printf("ERROR: attemping to create more than MAX_NUM_SERVERS!!!");
        exit(EXIT_FAILURE);
    }


    /*-------------------------------------
    |           ALLOCATE SERVER            |
    --------------------------------------*/

    SRV_CONFIGS_ARRY[new_srv_id] = malloc(sizeof(struct dbus_srv_config));
    bzero(SRV_CONFIGS_ARRY[new_srv_id], sizeof(struct dbus_srv_config));

    return new_srv_id;
} /* tcp_dbus_srv_create() */

void tcp_dbus_srv_delete(dbus_srv_id server_id)
{
    /*-------------------------------------
    |       VERIFY SERVER_ID EXISTS        |
    --------------------------------------*/

    if ( SRV_CONFIGS_ARRY[server_id] == NULL )
    {
        return;
    }


    /*-------------------------------------
    |          DEALLOCATE SERVER           |
    --------------------------------------*/

    free(SRV_CONFIGS_ARRY[server_id]);
    SRV_CONFIGS_ARRY[server_id] = NULL;
} /* tcp_dbus_srv_delete() */

/**
 * This function implements the 'TestInterface' interface for the
 * 'Server' DBus object.
 *
 * It also implements 'Introspect' method of
 * 'org.freedesktop.DBus.Introspectable' interface which returns the
 * XML string describing the interfaces, methods, and signals
 * implemented by 'Server' object. This also can be used by tools such
 * as d-feet(1) and can be queried by:
 *
 * $ gdbus introspect --session --dest org.example.TestServer --object-path /org/example/TestObject
 */
DBusHandlerResult server_message_handler(DBusConnection *conn, DBusMessage *message, void *config)
{
    DBusHandlerResult result;
    DBusMessage *reply = NULL;
    DBusError err;
    bool quit = false;

    fprintf(stderr, "Got D-Bus request: %s.%s on %s\n",
                    dbus_message_get_interface(message),
                    dbus_message_get_member(message),
                    dbus_message_get_path(message));

    /*
     * Does not allocate any memory; the error only needs to be
     * freed if it is set at some point.
     */
    dbus_error_init(&err);

    if (dbus_message_is_method_call(message, DBUS_INTERFACE_INTROSPECTABLE, "Introspect")) {

        if (!(reply = dbus_message_new_method_return(message)))
        {
            goto fail;
        }

        dbus_message_append_args(reply,DBUS_TYPE_STRING, &server_introspection_xml, DBUS_TYPE_INVALID);
    }
    else if (dbus_message_is_method_call(message, DBUS_INTERFACE_PROPERTIES, "Get")) {
        const char *interface, *property;

        if (!dbus_message_get_args(message, &err, DBUS_TYPE_STRING, &interface, DBUS_TYPE_STRING, &property, DBUS_TYPE_INVALID))
        {
            goto fail;
        }
        if (!(reply = dbus_message_new_method_return(message)))
        {	
            goto fail;
        }

        result = server_get_properties_handler(property, conn, reply);
        dbus_message_unref(reply);
        return result;
    }
    else if (dbus_message_is_method_call(message, DBUS_INTERFACE_PROPERTIES, "GetAll")) 
    {
        if (!(reply = dbus_message_new_method_return(message)))
        {
            goto fail;
        }

        result = server_get_all_properties_handler(conn, reply);
        dbus_message_unref(reply);
        return result;
    }
    
    else if (dbus_message_is_method_call(message, DBUS_TCP_IFACE, DBUS_TCP_SEND_MSG)) 
    {
        /*-------------------------------------
        |              VARIABLES               |
        --------------------------------------*/

        dbus_bool_t return_val;
        int data_sz, i;
        DBusMessageIter iter,subiter;


        /*-------------------------------------
        |           INITIALIZATIONS            |
        --------------------------------------*/

        i = 0;
        return_val = EXIT_SUCCESS;

        if (!dbus_message_iter_init(message, &iter))
        {
            goto fail;
        }

        data_sz = dbus_message_iter_get_element_count(&iter);
        dbus_message_iter_recurse(&iter,&subiter);


        /*-------------------------------------
        |       METHOD ARG VERIFICATIONS       |
        --------------------------------------*/

        if( DBUS_TYPE_ARRAY != dbus_message_iter_get_arg_type(&iter) || DBUS_TYPE_BYTE != dbus_message_iter_get_arg_type(&subiter) )
        {
            printf("ERROR: method %s() was given incorrect arguments. Expected DBUS_TYPE_ARRAY of DBUS_TYPE_BYTE -> \"(ay)\"", DBUS_TCP_SEND_MSG);
            return EXIT_FAILURE;
        }

        // printf( "\n\nArg 1 Type: %c\n", dbus_message_iter_get_arg_type(&iter) );
        // printf( "Arg 1 SubType: %c\n\n", dbus_message_iter_get_arg_type(&subiter) );

    #if (0) /* This method is more efficient using dbus_message_iter_get_fixed_array()
                However, this method does not work for some reason. Only a few random bytes
                are given from the original data array. This method should work */
        
        /*-------------------------------------
        |       GET ARRAY OF DATA BYTES        |
        --------------------------------------*/

        char *data;

        if(dbus_type_is_fixed(dbus_message_iter_get_arg_type(&subiter)) == true)
        {
            printf("dbus message array elements are of fixed type\n");
            dbus_message_iter_get_fixed_array(&subiter, &data, &data_sz);
        }


        /*-------------------------------------
        |      PRINT ARRAY OF DATA BYTES       |
        --------------------------------------*/

        printf("\n****************tcp_dbus_server.c: send_msg function****************\n\n");

        printf("Received %d bytes. Data as follows:\n\"",data_sz);
        for (i = 0; i < data_sz; i++)
        {
            printf("%c",data[i]);
            i+=1;
        }

        //TODO implement callback

    #else /* So instead, we use this method. Less efficient iterating through all the elements, but accurate */
        
        /*-------------------------------------
        |         ADDITIONAL VARIABLES         |
        --------------------------------------*/

        char data[data_sz];
        bzero(data,data_sz);

        // printf("\n****************tcp_dbus_server.c: send_msg function****************\n\n");
        // printf("Received %d bytes. Data as follows:\n\"",data_sz);
        

        /*-------------------------------------
        |       GET ARRAY OF DATA BYTES        |
        --------------------------------------*/
        
        do
        {
            dbus_message_iter_get_basic(&subiter, &data[i]);
            // printf("%c",data[i]);
            i+=1;
        }while (dbus_message_iter_next(&subiter) == true &&  i < data_sz);


        /*-------------------------------------
        |   CALL USER CALLBACK WITH MSG DATA   |
        --------------------------------------*/

        if( ((struct dbus_srv_config*)config)->callback != NULL )
        {
            (*(((struct dbus_srv_config*)config)->callback))(data, data_sz);
        }
        

    #endif

        // printf("\"\n");
        // printf("\n****************END---tcp_dbus_server.c: send_msg function---END****************\n\n");



        /*-------------------------------------
        |         CREATE METHOD REPLY          |
        --------------------------------------*/

        if (!(reply = dbus_message_new_method_return(message)))
        {
            goto fail;
        }

        dbus_message_append_args(reply, DBUS_TYPE_BOOLEAN, &return_val, DBUS_TYPE_INVALID);

    } 
    else
    {
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

fail:
    if (dbus_error_is_set(&err))
    {
        if (reply)
        {
            dbus_message_unref(reply);
        }
        reply = dbus_message_new_error(message, err.name, err.message);
        dbus_error_free(&err);
    }

    /*
     * In any cases we should have allocated a reply otherwise it
     * means that we failed to allocate one.
     */
    if (!reply)
    {
        return DBUS_HANDLER_RESULT_NEED_MEMORY;
    }

    /* Send the reply which might be an error one too. */
    result = DBUS_HANDLER_RESULT_HANDLED;
    if (!dbus_connection_send(conn, reply, NULL))
    {
        result = DBUS_HANDLER_RESULT_NEED_MEMORY;
    }

    dbus_message_unref(reply);

    if (quit)
    {
        fprintf(stderr, "Message Handler received quit command!\n");
        g_main_loop_quit( ((struct dbus_srv_config*)config)->loop );
    }

    return result;
} /* server_message_handler() */

void* server_thread(void *config)
{
    printf("Starting dbus tiny server v%s\n", srv_sftw_version);
    /* Start the glib event loop */
    g_main_loop_run( ((struct dbus_srv_config *)config)->loop );
    
    printf("Server thread exiting....\n");
    return NULL;
}  /* server_thread() */

int tcp_dbus_srv_init(dbus_srv_id srv_id, tcp_send_msg_callback callback)
{
    /*-------------------------------------
    |              VARIABLES               |
    --------------------------------------*/

    const DBusObjectPathVTable server_vtable = {.message_function = server_message_handler};
    DBusError err;
    int val;
    struct dbus_srv_config *config;


    /*-------------------------------------
    |           INITIALIZATIONS            |
    --------------------------------------*/

    config = SRV_CONFIGS_ARRY[srv_id];
    dbus_error_init(&err);


    /*-------------------------------------
    |           VERIFY VALID ID            |
    --------------------------------------*/

    if ( config == NULL )
    {
        printf("WARNING, called tcp_dbus_srv_init for server id %d but server has not been created for that id!\n", srv_id);
        return EXIT_FAILURE;
    }

    /*-------------------------------------
    |  VERIFY ID NOT ALREADY INITIALIZED   |
    --------------------------------------*/

    if ( config->conn != NULL )
    {
        printf("WARNING, called tcp_dbus_srv_init for server id %d but server was already initialized!\n", srv_id);
        return EXIT_FAILURE;
    }


    /*-------------------------------------
    |        SET SEND_MSG CALLBACK         |
    --------------------------------------*/

    config->callback = callback;


    /*-------------------------------------
    |        CONNECT TO SYSTEM DBUS        |
    --------------------------------------*/

    /* connect to the daemon bus. Note, the 2 main buses are the system and session (user) buses.
        This is connecting to the system bus. */
    config->conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
    if (config->conn == NULL)
    {
        fprintf(stderr, "Failed to get a system DBus connection: %s\n", err.message);
        dbus_error_free(&err);
        return EXIT_FAILURE;
    }


    /*-------------------------------------
    |    REQUESTION COMMON NAME ON DBUS    |
    --------------------------------------*/

    val = dbus_bus_request_name(config->conn, DBUS_TCP_SERVER_NAME, DBUS_NAME_FLAG_REPLACE_EXISTING , &err);
    if (val != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
    {
        fprintf(stderr, "-----Note: This service is required to be run as root-----\nFailed to request name on bus: %s. Be sure to execute as root\n", err.message);
        dbus_error_free(&err);
        return EXIT_FAILURE;
    }


    /*-------------------------------------
    |     REGISTER SERVER OBJECT PATHS     |
    --------------------------------------*/

    val = dbus_connection_register_object_path(config->conn, DBUS_TCP_OPATH, &server_vtable, (void*)config);
    if (!val)
    {
        fprintf(stderr, "Failed to register object path for '%s'\n", DBUS_TCP_OPATH);
        dbus_error_free(&err);
        return EXIT_FAILURE;
    }


    /*-------------------------------------
    |          CREATE G_MAIN_LOOP          |
    --------------------------------------*/

    /*
     * For the sake of simplicity we're using glib event loop to
     * handle DBus messages. This is the only place where glib is
     * used.
     */
    config->loop = g_main_loop_new(NULL, false);

    /* Set up the DBus connection to work in a GLib event loop */
    dbus_connection_setup_with_g_main(config->conn, NULL);

    return EXIT_SUCCESS;
} /* tcp_dbus_srv_init() */

int tcp_dbus_srv_execute(dbus_srv_id srv_id)
{
    /*-------------------------------------
    |              VARIABLES               |
    --------------------------------------*/

    pthread_t thread_id;
    struct dbus_srv_config *config;
    
    
    /*-------------------------------------
    |           INITIALIZATIONS            |
    --------------------------------------*/

    config = SRV_CONFIGS_ARRY[srv_id];


    /*-------------------------------------
    |           VERIFY VALID ID            |
    --------------------------------------*/

    if ( config == NULL )
    {
        printf("WARNING, called tcp_dbus_srv_execute for server id %d but server has not been created nor initialized for that id!\n", srv_id);
        return EXIT_FAILURE;
    }


    /*-------------------------------------
    |       VERIFY SERVER INITIALIZED      |
    --------------------------------------*/

    if ( config->conn == NULL || config->loop == NULL )
    {
        printf("ERROR: Failed to execute server. Server %d has not been created/initialized!\nExiting.....\n", srv_id);
        return (EXIT_FAILURE);
    }


    /*-------------------------------------
    |         CREATE SERVER THREAD         |
    --------------------------------------*/

    if ( EXIT_SUCCESS != pthread_create(&thread_id, NULL, server_thread, (void*)config) )
    {
        printf("FAILED to create thread!");
        return (EXIT_FAILURE);
    }


    /*-------------------------------------
    |         DETACH SERVER THREAD         |
    --------------------------------------*/

    printf("Detach server thread!\n");
    if ( EXIT_SUCCESS != pthread_detach(thread_id) )
    {
        printf("FAILED to detach thread!");
        return (EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
} /* tcp_dbus_srv_execute() */

void tcp_dbus_srv_kill(dbus_srv_id srv_id)
{
    /*-------------------------------------
    |              VARIABLES               |
    --------------------------------------*/

    struct dbus_srv_config *config;


    /*-------------------------------------
    |           INITIALIZATIONS            |
    --------------------------------------*/

    config = SRV_CONFIGS_ARRY[srv_id];
   

    /*-------------------------------------
    |           VERIFY VALID ID            |
    --------------------------------------*/

    if ( config == NULL )
    {
        return;
    }


    /*-------------------------------------
    |        VERIFY ID INITIALIZED         |
    --------------------------------------*/

    if ( config == NULL || config->loop == NULL || config->conn == NULL )
    {
        printf("WARNING, called tcp_dbus_srv_kill() for server id %d but server is not created/initialized!\n", srv_id);
        return;
    }

    /*-------------------------------------
    |             KILL SERVER              |
    --------------------------------------*/

    printf("Kill server thread\n");
    g_main_loop_quit(config->loop); /* Kill server (aka message handling thread/g_main_loop) */
    g_main_loop_unref (config->loop);

    // TODO addition cleanup needed?
} /* tcp_dbus_srv_kill() */

bool tcp_dbus_srv_emit_msg_recv_signal(dbus_srv_id srv_id, const char *msg, uint msg_sz)
{
    /*-------------------------------------
    |              VARIABLES               |
    --------------------------------------*/

    bool return_val;
    DBusMessage *dbus_msg;
    struct dbus_srv_config *config;


    /*-------------------------------------
    |           INITIALIZATIONS            |
    --------------------------------------*/

    config = SRV_CONFIGS_ARRY[srv_id];
    return_val = EXIT_SUCCESS;
    dbus_msg = NULL;


    /*-------------------------------------
    |        VERIFY ID INITIALIZED         |
    --------------------------------------*/

    if ( config == NULL || config->loop == NULL || config->conn == NULL )
    {
        printf("WARNING, called tcp_dbus_srv_emit_msg_recv_signal() for server id %d but server is not created/initialized!\n", srv_id);
        return EXIT_FAILURE;
    }


    /*-------------------------------------
    |          VERIFY PARAMETERS           |
    --------------------------------------*/

    if ( msg == NULL || msg_sz == 0 )
    {
        printf("WARNING, called tcp_dbus_srv_emit_msg_recv_signal() for server id %d but  msg == NULL or msg_sz == 0!\n", srv_id);
        return EXIT_FAILURE;
    }


    /*-------------------------------------
    |             SEND SIGNAL              |
    --------------------------------------*/

    if ( (dbus_msg = dbus_message_new_signal(DBUS_TCP_OPATH, DBUS_TCP_IFACE, DBUS_TCP_RECV_SIGNAL)) )
    {
        dbus_message_append_args (dbus_msg, DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE, &msg, msg_sz, DBUS_TYPE_INVALID);

        if ( dbus_connection_send(config->conn, dbus_msg, NULL) )
        {
            /* Send a METHOD_RETURN dbus_msg. */
            dbus_msg = dbus_message_new_method_return(dbus_msg);
        }
        else
        {
            /* return DBUS_HANDLER_RESULT_NEED_MEMORY; */
            return_val = EXIT_FAILURE;
        }
    }


    /*-------------------------------------
    |             VERIFICATION             |
    --------------------------------------*/

    /* Send the reply which might be an error one too. */
    if (dbus_msg == NULL || !dbus_connection_send(config->conn, dbus_msg, NULL))
    {
        /* result = DBUS_HANDLER_RESULT_NEED_MEMORY; */
        dbus_message_unref(dbus_msg);
        return EXIT_FAILURE;
    }


    /*-------------------------------------
    |               CLEANUP                |
    --------------------------------------*/

    dbus_message_unref(dbus_msg);

    return return_val;
}

bool tcp_dbus_srv_emit_connect_signal(dbus_srv_id srv_id, const char *tcp_clnt_id, uint size)
{
    /*-------------------------------------
    |              VARIABLES               |
    --------------------------------------*/

    bool return_val;
    DBusMessage *dbus_msg;
    struct dbus_srv_config *config;


    /*-------------------------------------
    |           INITIALIZATIONS            |
    --------------------------------------*/

    config = SRV_CONFIGS_ARRY[srv_id];
    return_val = EXIT_SUCCESS;
    dbus_msg = NULL;


    /*-------------------------------------
    |        VERIFY ID INITIALIZED         |
    --------------------------------------*/

    if ( config == NULL || config->loop == NULL || config->conn == NULL )
    {
        printf("WARNING, called tcp_dbus_srv_emit_connect_signal() for server id %d but server is not created/initialized!\n", srv_id);
        return EXIT_FAILURE;
    }


    /*-------------------------------------
    |          VERIFY PARAMETERS           |
    --------------------------------------*/

    if ( tcp_clnt_id == NULL || size == 0 )
    {
        printf("WARNING, called tcp_dbus_srv_emit_connect_signal() for server id %d but  tcp_clnt_id == NULL or size == 0!\n", srv_id);
        return EXIT_FAILURE;
    }


    /*-------------------------------------
    |             SEND SIGNAL              |
    --------------------------------------*/

    if ( (dbus_msg = dbus_message_new_signal(DBUS_TCP_OPATH, DBUS_TCP_IFACE, DBUS_TCP_CONNECT_SIGNAL)) )
    {
        dbus_message_append_args (dbus_msg, DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE, &tcp_clnt_id, size, DBUS_TYPE_INVALID);

        if ( dbus_connection_send(config->conn, dbus_msg, NULL) )
        {
            /* Send a METHOD_RETURN dbus_msg. */
            dbus_msg = dbus_message_new_method_return(dbus_msg);
        }
        else
        {
            /* return DBUS_HANDLER_RESULT_NEED_MEMORY; */
            return_val = EXIT_FAILURE;
        }
    }


    /*-------------------------------------
    |             VERIFICATION             |
    --------------------------------------*/

    /* Send the reply which might be an error one too. */
    if (dbus_msg == NULL || !dbus_connection_send(config->conn, dbus_msg, NULL))
    {
        /* result = DBUS_HANDLER_RESULT_NEED_MEMORY; */
        dbus_message_unref(dbus_msg);
        return EXIT_FAILURE;
    }


    /*-------------------------------------
    |               CLEANUP                |
    --------------------------------------*/

    dbus_message_unref(dbus_msg);

    return return_val;
}

bool tcp_dbus_srv_emit_disconnect_signal(dbus_srv_id srv_id, const char *tcp_clnt_id, uint size)
{
    /*-------------------------------------
    |              VARIABLES               |
    --------------------------------------*/

    bool return_val;
    DBusMessage *dbus_msg;
    struct dbus_srv_config *config;


    /*-------------------------------------
    |           INITIALIZATIONS            |
    --------------------------------------*/

    config = SRV_CONFIGS_ARRY[srv_id];
    return_val = EXIT_SUCCESS;
    dbus_msg = NULL;


    /*-------------------------------------
    |        VERIFY ID INITIALIZED         |
    --------------------------------------*/

    if ( config == NULL || config->loop == NULL || config->conn == NULL )
    {
        printf("WARNING, called tcp_dbus_srv_emit_disconnect_signal() for server id %d but server is not created/initialized!\n", srv_id);
        return EXIT_FAILURE;
    }


    /*-------------------------------------
    |          VERIFY PARAMETERS           |
    --------------------------------------*/

    if ( tcp_clnt_id == NULL || size == 0 )
    {
        printf("WARNING, called tcp_dbus_srv_emit_disconnect_signal() for server id %d but  tcp_clnt_id == NULL or size == 0!\n", srv_id);
        return EXIT_FAILURE;
    }


    /*-------------------------------------
    |             SEND SIGNAL              |
    --------------------------------------*/

    if ( (dbus_msg = dbus_message_new_signal(DBUS_TCP_OPATH, DBUS_TCP_IFACE, DBUS_TCP_DISCONNECT_SIGNAL)) )
    {
        dbus_message_append_args (dbus_msg, DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE, &tcp_clnt_id, size, DBUS_TYPE_INVALID);

        if ( dbus_connection_send(config->conn, dbus_msg, NULL) )
        {
            /* Send a METHOD_RETURN dbus_msg. */
            dbus_msg = dbus_message_new_method_return(dbus_msg);
        }
        else
        {
            /* return DBUS_HANDLER_RESULT_NEED_MEMORY; */
            return_val = EXIT_FAILURE;
        }
    }


    /*-------------------------------------
    |             VERIFICATION             |
    --------------------------------------*/

    /* Send the reply which might be an error one too. */
    if (dbus_msg == NULL || !dbus_connection_send(config->conn, dbus_msg, NULL))
    {
        /* result = DBUS_HANDLER_RESULT_NEED_MEMORY; */
        dbus_message_unref(dbus_msg);
        return EXIT_FAILURE;
    }


    /*-------------------------------------
    |               CLEANUP                |
    --------------------------------------*/

    dbus_message_unref(dbus_msg);

    return return_val;
}
