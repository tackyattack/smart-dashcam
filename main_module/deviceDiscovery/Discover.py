#!/usr/bin/env python
import ctypes
from time import sleep

EXIT_SUCCESS = 0
EXIT_FAILURE= 1

DBUS_TCP_SEND_MSG          = "TCP_SEND_MSG"
DBUS_TCP_RECV_SIGNAL       = "TCP_RECV_SIGNAL"
DBUS_TCP_CONNECT_SIGNAL    = "TCP_CONNECT_SIGNAL"
DBUS_TCP_DISCONNECT_SIGNAL = "TCP_DISCONNECT_SIGNAL"


tcp_dbus_clnt_lib = ctypes.CDLL('libdashcam_tcp_dbus_clnt.so')

# typedef void (*tcp_rx_signal_callback)(const char* tcp_clnt_uuid, const char* data, unsigned int data_sz);
tcp_rx_signal_callback_type = ctypes.CFUNCTYPE(None, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_uint)

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


class AuxDevice:
    def __init__(self, hostname, ip):
        self.hostname = hostname
        self.ip = ip

class DiscoverMixin(object):
    def __init__(self):
        self.id = tcp_dbus_client_create()

        val = -1
        server_version = ctypes.POINTER(ctypes.c_char_p)()
        while(val != EXIT_SUCCESS):
            val = tcp_dbus_client_init(self.id, server_version)
            if val == EXIT_FAILURE:
                raise Exception('Failed to initialize client!')
            sleep(1)

        # IMPORTANT: callback must be assigned to variable so that it doesn't get
        #            garbage collected and cause segfault
        self.rx_callback = self.getRxCallbackFunc()
        tcp_dbus_client_Subscribe2Recv(self.id, DBUS_TCP_RECV_SIGNAL, self.rx_callback)

    # Note: you must use closure to work with ctypes
    def getRxCallbackFunc(self):
        def tcp_rx_signal_callback(tcp_clnt_uuid, data, data_sz):
            self.tcp_rx_callback(tcp_clnt_uuid, data,data_sz)
        return tcp_rx_signal_callback_type(tcp_rx_signal_callback)

    # this is the python callback that can be implemented in child class
    def tcp_rx_callback(self, tcp_clnt_uuid, data, data_sz):
        print('data_sz:' + str(data_sz))
        print(tcp_clnt_uuid)

    def terminate(self):
        tcp_dbus_client_UnsubscribeRecv(self.id, DBUS_TCP_RECV_SIGNAL)
        tcp_dbus_client_disconnect(self.id);
        tcp_dbus_client_delete(self.id);

class DeviceFinder(DiscoverMixin):
    def __init__(self):
        super(DeviceFinder, self).__init__()

    def test_send(self):
        while True:
            null_ptr = ctypes.POINTER(ctypes.c_char)()
            data = (ctypes.c_char*100)()
            sz = ctypes.c_uint(100)
            print(sz)
            print(tcp_dbus_send_msg(self.id, null_ptr, data, sz))
            sleep(1)


    def request_hostnames(self):
        pass

    def get_aux_devices(self):
        pass


class AuxDeviceDiscoveryManager:

    def __init__(self):
        pass

finder = DeviceFinder()
try:
    finder.test_send()
except KeyboardInterrupt:
    finder.terminate()
