# ARAS VIO End-Effector
A general purpose visual-inertial module with ov9281 global shutter camera and BMI088 IMU sensor for perception and state estimation research. The system uses a Raspberry Pi 4 as the computing module and uses the [python-serial-ntp](https://github.com/Rooholla-KhorramBakht/python-serial-ntp.git) for synchronizing the clock with the ground-station. 
## Requirements
The softwares in this repository are based on the MMAL camera stack of the raspberry pi and are tested on the bulster image. Furtheremore, the following packages need to be installed:

```bash
sudo apt-get update && sudo apt-get install libzbar-dev libopencv-dev
sudo apt-get install python-opencv
```

## Installation
To install the Arducam's custom library and enable the I2C, you need to cd into the camera_node directory and run the following:

```bash
make install
chmod +x enable_i2c_vc.sh 
./enable_i2c_vc.sh
```

It is also important to note that the defauld i2c name in the dev directory does not match the assumption made by the arducam, a workaround is to link it to the name assumed by the arducam:

```bash
sudo ln -s /dev/i2c-0 /dev/i2c-10
```

Finally, enable the camera interface through the raspi-config command.
