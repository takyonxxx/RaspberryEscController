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

#### Detailed Message Breakdown:

| Position | Hex Value | Decimal | Field | Description |
|----------|-----------|---------|-------|-------------|
| 0 | 0xb0 | 176 | **Header** | Fixed message header identifier |
| 1 | 0x08 | 8 | **Length** | Payload length (8 bytes for 4 PWM values) |
| 2 | 0x01 | 1 | **RW** | Read/Write flag (0x01 = Write command) |
| 3 | 0xa0 | 160 | **Command** | Servo command (0xa0 = mSERVO1) |
| 4 | 0x40 | 64 | **PWM1 Low** | Motor 1 PWM low byte |
| 5 | 0x06 | 6 | **PWM1 High** | Motor 1 PWM high byte |
| 6 | 0xDC | 220 | **PWM2 Low** | Motor 2 PWM low byte |
| 7 | 0x05 | 5 | **PWM2 High** | Motor 2 PWM high byte |
| 8 | 0x78 | 120 | **PWM3 Low** | Motor 3 PWM low byte |
| 9 | 0x05 | 5 | **PWM3 High** | Motor 3 PWM high byte |
| 10 | 0xA4 | 164 | **PWM4 Low** | Motor 4 PWM low byte |
| 11 | 0x06 | 6 | **PWM4 High** | Motor 4 PWM high byte |

#### PWM Value Reconstruction (Little-Endian):

**Motor 1 PWM**: 
```
Low Byte:  0x40 = 64
High Byte: 0x06 = 6
PWM = (High << 8) | Low = (6 × 256) + 64 = 1600μs (Forward)
```

**Motor 2 PWM**:
```
Low Byte:  0xDC = 220  
High Byte: 0x05 = 5
PWM = (5 × 256) + 220 = 1500μs (Neutral)
```

**Motor 3 PWM**:
```
Low Byte:  0x78 = 120
High Byte: 0x05 = 5  
PWM = (5 × 256) + 120 = 1400μs (Reverse)
```

**Motor 4 PWM**:
```
Low Byte:  0xA4 = 164
High Byte: 0x06 = 6
PWM = (6 × 256) + 164 = 1700μs (Forward)
```

#### Message Result:
This example message sets:
- **Motor 1**: 1600μs (slight forward)
- **Motor 2**: 1500μs (neutral/stopped)  
- **Motor 3**: 1400μs (slight reverse)
- **Motor 4**: 1700μs (moderate forward)

#### Implementation:
```cpp
// Mobile app creates message:
int pwm1 = 1600, pwm2 = 1500, pwm3 = 1400, pwm4 = 1700;
payload.append(static_cast<char>(pwm1 & 0xFF));        // Low byte
payload.append(static_cast<char>((pwm1 >> 8) & 0xFF)); // High byte
// ... repeat for pwm2, pwm3, pwm4

// Raspberry Pi decodes message:
uint16_t pwm1 = (message.data[1] << 8) | message.data[0]; // 1600μs
uint16_t pwm2 = (message.data[3] << 8) | message.data[2]; // 1500μs  
uint16_t pwm3 = (message.data[5] << 8) | message.data[4]; // 1400μs
uint16_t pwm4 = (message.data[7] << 8) | message.data[6]; // 1700μs
```

## Troubleshooting

### Common Issues
1. **ESC not responding**: Check GPIO connections and power supply
2. **BLE connection fails**: Ensure Bluetooth is enabled and device is discoverable
3. **PWM timing issues**: Verify WiringPi installation and GPIO permissions
4. **Build errors**: Check Qt6 and development package installations

## License
This project is open source. Please check the license file for details.

## Contributing
Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.
