# RaspberryPi ESC Controller
A Raspberry Pi-based Electronic Speed Controller system with mobile remote control capabilities.

<img src="https://github.com/takyonxxx/RaspberryPI_Esc_Controller/blob/main/RemoteControl/mobile.jpg" alt="Mobile Remote Control Interface" width="300">

## Hardware Requirements
- Raspberry Pi 5 (4 or 3 may work with timing adjustments)
- 4x ESC (Electronic Speed Controllers)
- 4x Brushless Motors
- Power supply for motors

## Motor GPIO Pinout Configuration

| Motor | GPIO Pin | Physical Pin | Description |
|-------|----------|--------------|-------------|
| ESC 1 | GPIO 18  | Pin 12      | Hardware PWM capable |
| ESC 2 | GPIO 12  | Pin 32      | Hardware PWM capable |
| ESC 3 | GPIO 13  | Pin 33      | Hardware PWM capable |
| ESC 4 | GPIO 19  | Pin 35      | Hardware PWM capable |

### ESC Connection Diagram
```
Raspberry Pi                    ESC
┌─────────────┐                ┌─────────────┐
│ GPIO 18     │───────────────→│ Signal      │ ESC 1
│ (Pin 12)    │                │             │
│             │                └─────────────┘
│ GPIO 12     │───────────────→│ Signal      │ ESC 2
│ (Pin 32)    │                │             │
│             │                └─────────────┘
│ GPIO 13     │───────────────→│ Signal      │ ESC 3
│ (Pin 33)    │                │             │
│             │                └─────────────┘
│ GPIO 19     │───────────────→│ Signal      │ ESC 4
│ (Pin 35)    │                │             │
│             │                └─────────────┘
│ GND         │───────────────→│ GND         │ All ESCs
│ (Pins 6,9,  │                │             │
│  14,20,25,  │                └─────────────┘
│  30,34,39)  │
└─────────────┘
```

### PWM Signal Specifications
- **Frequency**: 50Hz (20ms period)
- **Pulse Width Range**: 1000μs - 2000μs
- **Neutral Position**: 1500μs
- **Signal Type**: 3.3V PWM (compatible with most ESCs)

**Note**: All selected GPIO pins (18, 12, 13, 19) support hardware PWM for precise timing control.

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

# Install WiringPi from source
git clone https://github.com/WiringPi/WiringPi.git
cd WiringPi
sudo ./build
cd ..
```

<img src="https://github.com/takyonxxx/RaspberryPI_Esc_Controller/blob/main/raspberry_pinout.jpg" alt="Raspberry Pi Pinout Reference" width="800">

## Building the Project

### Main ESC Controller
```bash
cd RaspberryPI_Esc_Controller
qmake
make
```

### Mobile Remote Control App
```bash
cd RemoteControl
qmake
make
```

## Motor Control Features
- Individual PWM control for each motor (1000-2000μs range)
- Real-time mobile remote control via Bluetooth LE
- Safety features: Arm/Disarm system, Emergency stop
- Thread-safe ESC management with 50Hz update rate
- Hardware PWM support for precise timing

## System Architecture

### ESC Controller (Raspberry Pi)
- **ESCControl Class**: Individual ESC PWM generation with dedicated threads
- **ESCControlThread**: Manages all 4 ESCs with thread-safe command processing
- **ServoController**: BLE message handling and ESC coordination
- **GattServer**: Bluetooth LE server for mobile communication

### Mobile Remote Control
- **Touch-friendly interface** with vertical PWM sliders
- **Real-time PWM control** for each motor (1000-2000μs)
- **Visual feedback** with color-coded PWM values
- **Safety controls**: Connect/Disconnect, Arm/Disarm, Emergency Stop

## Usage

### Starting the ESC Controller
```bash
sudo ./esc_controller
```

### Using the Mobile App
1. Build and install the remote control app on your mobile device
2. Start the ESC controller on Raspberry Pi
3. Open the mobile app and tap "Connect"
4. Once connected, tap "Arm" to enable motor control
5. Use the vertical sliders to control individual motor PWM values:
   - **1000μs**: Full reverse
   - **1500μs**: Neutral (stopped)
   - **2000μs**: Full forward

### Safety Features
- System must be "Armed" before motor commands are accepted
- Automatic disarm and neutral position on disconnect
- Emergency stop button sets all motors to neutral
- Command timeout protection (auto-neutral after 2 seconds of no commands)

## Communication Protocol

### BLE Message Format
```
Header: 0xb0
Length: 8 bytes (4 PWM values × 2 bytes each)
RW: 0x01 (write command)
Command: 0xa0 (SERVO1), 0xa1 (SERVO2), 0xa2 (SERVO3), 0xa3 (SERVO4)
Data: [PWM1_LOW, PWM1_HIGH, PWM2_LOW, PWM2_HIGH, PWM3_LOW, PWM3_HIGH, PWM4_LOW, PWM4_HIGH]
```

### Example: Set Motor 1 to 1600μs
```
[0xb0, 0x08, 0x01, 0xa0, 0x40, 0x06, 0xDC, 0x05, 0x78, 0x05, 0xA4, 0x06]
```

## Troubleshooting

### Common Issues
1. **ESC not responding**: Check GPIO connections and power supply
2. **BLE connection fails**: Ensure Bluetooth is enabled and device is discoverable
3. **PWM timing issues**: Verify WiringPi installation and GPIO permissions
4. **Build errors**: Check Qt6 and development package installations

### Debug Output
The system provides detailed console output for:
- ESC initialization status
- BLE connection events
- PWM command processing
- Safety system activations

## License
This project is open source. Please check the license file for details.

## Contributing
Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.
