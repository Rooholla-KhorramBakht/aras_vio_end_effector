#!/usr/bin/python
import serial
import time
import struct
import signal
import sys
import numpy as np
from src.utils import *


#This is for sending the node's to the ground station
state_ids = {'imu_stream_state':4}
state_vals = {'imu_stream_state':False}  
hyperTele = hypervisorTelemetry('10.42.0.1', 10000, state_ids, state_vals)


imu_dataset = []
port = serial.Serial('/dev/ttyAMA0', 921600, timeout=1)
port.flush()

try:
    while True:
        data=port.read_until( terminator=''.join(['abc\n']).encode('utf-8'))[:-4]
        if(len(data)==36):
            packed_data = struct.unpack('3f3ff2i',data)
            ax,ay,az,wx,wy,wz,temp, imu_ts,camera_ts=packed_data
            #print(ax,ay,az,wx,wy,wz,temp, imu_ts,camera_ts)
            imu_dataset.append([time.time_ns(), ax,ay,az,wx,wy,wz,temp, imu_ts,camera_ts])
            state_vals['imu_stream_state'] = True
        else:
            state_vals['imu_stream_state'] = False
            print("Failed to receive")
            print(len(data))

        time.sleep(1/250)
        hyperTele.update()

except KeyboardInterrupt:
    imu_dataset = np.vstack(imu_dataset)
    print('saving the dataset')
    np.savetxt("data_tmp_dir/imu_dataset.csv", imu_dataset, delimiter=",")
    port.flush()
    port.close()

