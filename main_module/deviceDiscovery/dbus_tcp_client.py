#!/usr/bin/env python
import ctypes
from time import sleep
import socket
import os
import signal

EXIT_SUCCESS = 0
EXIT_FAILURE= 1

DBUS_TCP_SEND_MSG          = "TCP_SEND_MSG"
DBUS_TCP_RECV_SIGNAL       = "TCP_RECV_SIGNAL"
DBUS_TCP_CONNECT_SIGNAL    = "TCP_CONNECT_SIGNAL"
DBUS_TCP_DISCONNECT_SIGNAL = "TCP_DISCONNECT_SIGNAL"


tcp_dbus_clnt_lib = ctypes.CDLL('libdashcam_tcp_dbus_clnt.so')

# typedef void (*tcp_rx_signal_callback)(const char* tcp_clnt_uuid, const char* data, unsigned int data_sz);
tcp_rx_signal_callback_type = ctypes.CFUNCTYPE(None, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_uint)

null_ptr = ctypes.POINTER(ctypes.c_char)()

# dbus_clnt_id tcp_dbus_client_create();
tcp_dbus_client_create = tcp_dbus_clnt_lib.tcp_dbus_client_create
tcp_dbus_client_create.argtypes = None
tcp_dbus_client_create.restype = ctypes.c_int

# int tcp_dbus_client_init(dbus_clnt_id clnt_id, char const ** srv_version);
tcp_dbus_client_init = tcp_dbus_clnt_lib.tcp_dbus_client_init
tcp_dbus_client_init.argtypes = [ctypes.c_int, ctypes.POINTER(ctypes.c_char_p)]
tcp_dbus_client_init.restype = ctypes.c_int

# int tcp_dbus_client_Subscribe2Recv(dbus_clnt_id clnt_id, char* signal, tcp_rx_signal_callback callback);
tcp_dbus_client_Subscribe2Recv = tcp_dbus_clnt_lib.tcp_dbus_client_Subscribe2Recv
tcp_dbus_client_Subscribe2Recv.argtypes = [ctypes.c_int, ctypes.c_char_p, tcp_rx_signal_callback_type]
tcp_dbus_client_Subscribe2Recv.restype = ctypes.c_int

# int tcp_dbus_send_msg(dbus_clnt_id clnt_id, const char* tcp_clnt_uuid, char* data, uint data_sz);
tcp_dbus_send_msg = tcp_dbus_clnt_lib.tcp_dbus_send_msg
tcp_dbus_send_msg.argtypes = [ctypes.c_int, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_uint]
tcp_dbus_send_msg.restype = ctypes.c_int


# int tcp_dbus_client_UnsubscribeRecv(dbus_clnt_id clnt_id, char* signal);
tcp_dbus_client_UnsubscribeRecv = tcp_dbus_clnt_lib.tcp_dbus_client_UnsubscribeRecv
tcp_dbus_client_UnsubscribeRecv.argtypes = [ctypes.c_int, ctypes.c_char_p]
tcp_dbus_client_UnsubscribeRecv.restype = ctypes.c_int

# void tcp_dbus_client_disconnect(dbus_clnt_id clnt_id);
tcp_dbus_client_disconnect = tcp_dbus_clnt_lib.tcp_dbus_client_disconnect
tcp_dbus_client_disconnect.argtypes = [ctypes.c_int]
tcp_dbus_client_disconnect.restype = None

# void tcp_dbus_client_delete(dbus_clnt_id clnt_id);
tcp_dbus_client_delete = tcp_dbus_clnt_lib.tcp_dbus_client_delete
tcp_dbus_client_delete.argtypes = [ctypes.c_int]
tcp_dbus_client_delete.restype = None
