import subprocess
import time
import os
import argparse
from shutil import move

parser = argparse.ArgumentParser('Record a sequece of dataset')
parser.add_argument('data_path', metavar='path', type=str,
                    help='The path to the dataset directory')

args = parser.parse_args()
dataset_path = args.data_path

if not os.path.exists(dataset_path):
    print('Creating the dataset path')
    os.mkdir(dataset_path)


files = os.listdir('data_tmp_dir')
if len(files):
    print('Removing the old data files...')
    for file in files:
        if os.path.isfile(os.path.join('dataset', file)):
            os.remove(os.path.join('dataset', file))

print('starting the IMU process')
imu_process = subprocess.Popen(args = ['python3', 'imu_capture.py'], stdout=subprocess.PIPE)
time.sleep(2)

print('starting the Caemra process')

if not os.path.exists('/dev/i2c-10'):
    subprocess.Popen(args = ['sudo', 'ln', '-s', '/dev/i2c-0', '/dev/i2c-10'], stdout=subprocess.PIPE)

camera_process = subprocess.Popen(args = ['./camera_node/video'], stdout=subprocess.PIPE)

def terminateCamera():
    camera_process.terminate()
    print('Waiting for the camera process to terminate...')
    while camera_process.poll() is None:
        time.sleep(0.1)
    print('camera_process process is terminated!')

def terminateIMU():
    imu_process.terminate()
    print('Waiting for the IMU processes to terminate...')
    while imu_process.poll() is None:
        time.sleep(0.1)
    print('IMU process is terminated!')

try:
    while(True):
        time.sleep(0.2)
        if camera_process.poll() is not None:
            print(f'Error: Camera process is terminated with code {camera_process.poll()}')
            terminateIMU()
            exit(-1)

        if imu_process.poll() is not None:
            print(f'Error: IMU process is terminated with code {imu_process.poll()}')
            terminateCamera()
            exit(-1)

        
except KeyboardInterrupt:
    terminateCamera()
    time.sleep(0.5)
    terminateIMU()
    time.sleep(2)
    files = os.listdir('data_tmp_dir')
    if len(files):
        print('Moving the recorded dataset ...')
        for file in files:
            if os.path.isfile(os.path.join('data_tmp_dir', file)):
                move(os.path.join('data_tmp_dir', file), dataset_path)
    exit(0)

