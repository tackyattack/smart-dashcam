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

def create_cstring(py_str):
    str_list = list(py_str)
    str_sz = len(str_list) + 1
    cstr = (ctypes.c_char*str_sz)()
    for i in range(str_sz-1):
        cstr[i] = str_list[i]
    cstr[str_sz-1] = '\0'
    sz = ctypes.c_uint(str_sz)
    return cstr, sz

class AuxDevice:
    def __init__(self, hostname, ip):
        self.hostname = hostname
        self.ip = ip

class DiscoverMixin(object):
    def __init__(self):
        self.running = True
        self.dbus_connected = False
        self.id = tcp_dbus_client_create()

        val = -1
        server_version = ctypes.POINTER(ctypes.c_char_p)()
        timeout = 5
        while((val != EXIT_SUCCESS) and self.running and (timeout>0)):
            val = tcp_dbus_client_init(self.id, server_version)
            if val == EXIT_FAILURE:
                raise Exception('Failed to initialize client!')
            sleep(1)
            timeout = timeout - 1


        if timeout == 0:
            self.terminate()
            return

        # IMPORTANT: callback must be assigned to variable so that it doesn't get
        #            garbage collected and cause segfault
        self.rx_callback = self.getRxCallbackFunc()
        tcp_dbus_client_Subscribe2Recv(self.id, DBUS_TCP_RECV_SIGNAL, self.rx_callback)
        self.dbus_connected = True

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
        self.running = False
        if self.dbus_connected:
            tcp_dbus_client_UnsubscribeRecv(self.id, DBUS_TCP_RECV_SIGNAL)
            tcp_dbus_client_disconnect(self.id);
            tcp_dbus_client_delete(self.id)

class DeviceFinder(DiscoverMixin):
    def __init__(self, stream_port=None):
        super(DeviceFinder, self).__init__()
        self.hostnames = set([])
        self.stream_port = stream_port

    def get_hostname(self):
        host_name = None
        try:
            host_name = socket.gethostname()
        except:
            print("unable to get hostname")
        return host_name

    def tcp_rx_callback(self, tcp_clnt_uuid, data, data_sz):
        if 'discover' in data:
            hostname_port_packet = 'me:' + self.get_hostname() + ':' + str(self.stream_port)
            hostname_reply, sz = create_cstring(hostname_port_packet)
            tcp_dbus_send_msg(self.id, null_ptr, hostname_reply, sz)
        if 'me:' in data:
            hostname = data[3:]
            self.hostnames.add(hostname)

    def request_hostnames(self):
        self.hostnames = set([])
        discover_request, sz = create_cstring('discover')
        tcp_dbus_send_msg(self.id, null_ptr, discover_request, sz)

    def get_aux_devices(self, time_to_wait):
        self.request_hostnames()
        device_list = []
        sleep(time_to_wait)
        for hostname_port in self.hostnames:
            # 'tcp://192.168.0.152:8080'
            hostname = ''.join(hostname_port.split(':')[:-1])
            port = hostname_port.split(':')[-1]
            host_ip = socket.gethostbyname(hostname)
            device = 'tcp://' + host_ip + ':' + port
            device_list.append((device, hostname))
        return device_list

# test it
if __name__ == "__main__":
    finder = DeviceFinder(8080)
    try:
        while True:
            print(finder.get_aux_devices(1))
            sleep(1)
    except KeyboardInterrupt:
        finder.terminate()
