#!/usr/bin/env python
import ctypes
from time import sleep
import socket
import os
import signal
import dbus_tcp_client

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
        self.id = dbus_tcp_client.tcp_dbus_client_create()

    def connect(self, timeout):
        val = -1
        server_version = ctypes.POINTER(ctypes.c_char_p)()
        while((val != dbus_tcp_client.EXIT_SUCCESS) and self.running and (timeout>0)):
            val = dbus_tcp_client.tcp_dbus_client_init(self.id, server_version)
            if val == dbus_tcp_client.EXIT_FAILURE:
                raise Exception('Failed to initialize client!')
            sleep(1)
            timeout = timeout - 1

        if timeout == 0:
            self.terminate()
            return False

        # IMPORTANT: callback must be assigned to variable so that it doesn't get
        #            garbage collected and cause segfault
        self.rx_callback = self.getRxCallbackFunc()
        dbus_tcp_client.tcp_dbus_client_Subscribe2Recv(self.id, dbus_tcp_client.DBUS_TCP_RECV_SIGNAL, self.rx_callback)
        self.dbus_connected = True
        return True


    # Note: you must use closure to work with ctypes
    def getRxCallbackFunc(self):
        def tcp_rx_signal_callback(tcp_clnt_uuid, data, data_sz):
            self.tcp_rx_callback(tcp_clnt_uuid, data,data_sz)
        return dbus_tcp_client.tcp_rx_signal_callback_type(tcp_rx_signal_callback)

    # this is the python callback that can be implemented in child class
    def tcp_rx_callback(self, tcp_clnt_uuid, data, data_sz):
        print('data_sz:' + str(data_sz))
        print(tcp_clnt_uuid)

    def terminate(self):
        self.running = False
        if self.dbus_connected:
            dbus_tcp_client.tcp_dbus_client_UnsubscribeRecv(self.id, dbus_tcp_client.DBUS_TCP_RECV_SIGNAL)
            dbus_tcp_client.tcp_dbus_client_disconnect(self.id);
            dbus_tcp_client.tcp_dbus_client_delete(self.id)

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
            dbus_tcp_client.tcp_dbus_send_msg(self.id, dbus_tcp_client.null_ptr, hostname_reply, sz)
        if 'me:' in data:
            hostname = data[3:]
            self.hostnames.add(hostname)

    def request_hostnames(self):
        self.hostnames = set([])
        discover_request, sz = create_cstring('discover')
        dbus_tcp_client.tcp_dbus_send_msg(self.id, dbus_tcp_client.null_ptr, discover_request, sz)

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
