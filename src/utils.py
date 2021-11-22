import time
import struct
import threading
import time 
import socket

class wifiDataLink():
    def __init__(self, remote_ip, remote_port, local_ip = None, local_port = None):
        self.remote_ip=remote_ip
        self.local_ip=local_ip
        self.remote_port=remote_port
        self.local_port=local_port
        self.out_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        # Only Transfer ?
        if self.local_ip is not None:
            self.in_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.in_socket.bind((self.local_ip, self.local_port))
        
    def transmitData(self, data_lest):
        buffer = struct.pack(f'{len(data_lest)}d', *data_lest)
        self.out_socket.sendto(buffer, (self.remote_ip, self.remote_port))
        
    def getData(self, length):
        data, addr = self.in_socket.recvfrom(length)
        return data   


class hypervisorTelemetry():
    def __init__(self, hypervisor_ip, hypervisor_port, state_ids, state_vals):
        self.last_hypervisor_update = time.time()
        self.hypterLink = wifiDataLink(hypervisor_ip, hypervisor_port)
        self.state_ids = state_ids
        self.state_vals = state_vals

    def update(self):
        if time.time() - self.last_hypervisor_update > 0.2:
            self.last_hypervisor_update = time.time()
            states = []
            for state in self.state_ids:
                states.append(self.state_ids[state])
                states.append(float(self.state_vals[state]))

            self.hypterLink.transmitData(states)

