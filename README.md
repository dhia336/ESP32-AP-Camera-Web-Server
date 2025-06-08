# ESP32 Camera Web Server with Flashlight Control

A web server implementation for ESP32 camera module that allows remote viewing and control of the camera feed, including flashlight functionality.

## Features

- Live video streaming from ESP32 camera
- Remote flashlight control
- Web-based interface for easy access
- WiFi Access Point mode for direct connection

## Hardware Requirements

- ESP32 development board
- ESP32 camera module (compatible with OV2640)
- LED flashlight module (connected to ESP32 GPIO)

## Setup Instructions

1. Connect the camera module to ESP32 following the pinout diagram
2. Connect the flashlight LED to the designated GPIO pin
3. Upload the code to your ESP32 using Arduino IDE or PlatformIO
4. Power on the ESP32
5. Connect to the ESP32's WiFi network
6. Open a web browser and navigate to the ESP32's IP address

## Usage

1. Access the web interface by connecting to the ESP32's WiFi network
2. View the live camera feed in your web browser
3. Control the flashlight using the web interface
   - Note: Flashlight control is currently only available when the video stream is off (to be fixed in future updates)

## Known Limitations

- Flashlight control is temporarily disabled during video streaming
- This limitation will be addressed in a future update
