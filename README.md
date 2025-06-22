# RaspberryPi Esc Controller
 RaspberryPi Esc Controller
 
## Hardware Requirements

- Raspberry Pi 5 (4 or 3 may work with timing adjustments)

## Raspberry Pi Setup
 
 ```bash
 # Install build essentials
 sudo apt-get install build-essential devscripts
 
 # Install Qt6 development packages
 sudo apt install qt6-base-dev qt6-base-dev-tools
 sudo apt install qt6-connectivity-dev libqt6bluetooth6 qt6-base-dev
 
 # Setup qmake symlink
 sudo ln -sf /usr/lib/qt6/bin/qmake6 /usr/local/bin/qmake
 
 # Install I2C development packages
 sudo apt install libi2c-dev i2c-tools 

 ```
