# Generic Bluetooth LE Gamepad

This project has been converted from an Xbox-specific controller to a **generic Bluetooth LE gamepad** that is compatible with a wide range of devices and platforms.

## Key Changes Made

### 1. **Updated HID Descriptor**
- Changed from Xbox-specific 16-button layout to standard 12-button gamepad layout
- Uses generic HID usage codes that are widely supported
- More compatible with various operating systems (Windows, macOS, Linux, Android, iOS)
- Reduced report size from 9 bytes to 8 bytes for better efficiency

### 2. **Generic Button Layout**
The gamepad now uses a standard button mapping:
- **Face Buttons**: Button 1-4 (A/Cross, B/Circle, X/Square, Y/Triangle equivalents)
- **Shoulder Buttons**: L1, R1 (Left/Right shoulder buttons)
- **System Buttons**: Select/Back, Start/Menu, Home/Guide
- **Stick Clicks**: L3, R3 (Left/Right stick press)
- **Extra Button**: 12th programmable button

### 3. **Updated Device Identity**
- **Device Name**: Changed from "BT Gamepad" to "Generic Gamepad"
- **Advertising**: Uses standard HID gamepad appearance code
- **Compatibility**: Works with standard HID gamepad drivers

### 4. **Control Layout**
- **Left Analog Stick**: X/Y axes (-127 to +127)
- **Right Analog Stick**: Z/Rz axes (-127 to +127) 
- **Triggers**: Left/Right analog triggers (0-255)
- **D-Pad**: 8-direction hat switch (0-7, 8=neutral)
- **Buttons**: 12 digital buttons

## Platform Compatibility

### ✅ **Fully Supported Platforms**
- **Windows 10/11**: Recognized as "HID-compliant game controller"
- **Android**: Works with standard gamepad APIs
- **Linux**: Compatible with evdev/js input systems
- **Steam**: Recognized as generic gamepad in Steam Input
- **RetroArch**: Works with standard gamepad profiles

### ⚠️ **Limited Support**
- **Xbox consoles**: May not work (requires Xbox-specific authentication)
- **PlayStation consoles**: May not work (requires PlayStation-specific authentication)

### ✅ **Wide Application Support**
- **Gaming**: Steam games, emulators, indie games
- **Productivity**: Media control, presentation remotes
- **Development**: Game testing, input device development

## Demo Functionality

The device runs an automatic demo that cycles through:
1. **Individual button presses** (12 buttons)
2. **Analog stick movements** (both sticks, all directions)
3. **Trigger presses** (left and right triggers)
4. **D-pad directions** (8 directions)
5. **Combination inputs** (multiple buttons + triggers)

Each demo step lasts 100ms and prints status to the debug console.

## Hardware Requirements

- **Raspberry Pi Pico W** (with WiFi/Bluetooth chip)
- **BTStack** Bluetooth library
- **Pico SDK** 2.1.1 or later

## Building and Flashing

1. **Build the project**:
   ```bash
   # Use VS Code task or ninja directly
   ninja -C build
   ```

2. **Flash to device**:
   - Hold BOOTSEL button while connecting USB
   - Copy `BTTest2.uf2` to the mounted RPI-RP2 drive
   - Or use the "Run Project" VS Code task

## Usage as Input Device

Once flashed and running:

1. **Pairing**: The device advertises as "Generic Gamepad"
2. **Connection**: Use your device's Bluetooth settings to pair
3. **Testing**: The automatic demo will show all inputs working
4. **Custom Code**: Modify the demo functions to send your own input data

## Customization Options

### Adding More Buttons
- Modify the HID descriptor to support up to 32 buttons
- Update the report structure and button definitions
- Ensure your target platform supports additional buttons

### Changing Analog Ranges
- Modify logical min/max in HID descriptor
- Update the demo code to use different ranges
- Some platforms prefer 0-255 instead of -127 to +127

### Adding Features
- **Gyroscope/Accelerometer**: Add motion sensor data
- **Vibration**: Implement force feedback (requires additional hardware)
- **Audio**: Add Bluetooth audio capabilities
- **LED Control**: Add RGB LED support for visual feedback

## Troubleshooting

### Device Not Recognized
1. Check that Bluetooth LE is supported on target device
2. Clear Bluetooth cache and re-pair
3. Verify the device appears in Bluetooth device list

### Input Not Working
1. Check if application supports generic HID gamepads
2. Try with a gamepad testing application
3. Verify button mappings match your expectations

### Connection Issues
1. Ensure device is in pairing mode
2. Remove existing pairings and re-pair
3. Check for interference from other Bluetooth devices

## Code Structure

- **`main.cpp`**: Main gamepad implementation
- **`hog_keyboard_demo.gatt`**: GATT profile definition
- **`btstack_config.h`**: Bluetooth stack configuration
- **`CMakeLists.txt`**: Build configuration

## Further Development

This generic gamepad provides a solid foundation for:
- **Custom Gaming Controllers**: Racing wheels, flight sticks, etc.
- **Accessibility Devices**: Adaptive controllers for disabled users
- **IoT Input Devices**: Remote controls for smart home systems
- **Development Tools**: Input device prototyping and testing

The codebase is designed to be easily extensible and customizable for specific use cases while maintaining broad compatibility with standard HID gamepad implementations.
