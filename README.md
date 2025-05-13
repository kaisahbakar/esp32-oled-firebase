# esp32-oled-firebase


🙋‍♂️ Author

Developed by Nur Kaisah binti Abu Bakar
Sensor-based System
Universiti Utara Malaysia (UUM)

# ESP32 Firebase OLED Display System

This project demonstrates an IoT system using the ESP32 microcontroller that:

- Stores WiFi and device credentials using EEPROM
- Connects to a Firebase Realtime Database
- Retrieves and displays text data on an OLED screen (SSD1306)
- Allows WiFi reconfiguration through Access Point (AP) mode and a simple web interface
- Displays connection status, device ID, and live text from Firebase

## Features

- **EEPROM Storage**: Remembers WiFi SSID, password, and device ID
- **OLED Display**: Shows connection status, messages from Firebase, and AP mode IP
- **AP Mode Web Config**: Reconfigure credentials without hardcoding
- **Firebase Realtime Sync**: Retrieves string from Firebase and displays on screen

## Hardware Used

- ESP32 Development Board
- SSD1306 OLED Display (I2C)
- Push Button (for entering AP mode)
- LED Indicator (optional)

## File Structure

ESP32_Firebase_Display/
├── main.cpp # Full source code
├── demo.mp4 # Demonstration video of the working system
├── README.md # Project documentation


> ⚠️ If `demo.mp4` is not available due to GitHub upload limits, view the demo video here:  
> 📺 [Watch Demo on YouTube](https://youtu.be/iPszaxT2nEo)

## 🚀 How to Use

1. Flash the code to your ESP32 using Arduino IDE or PlatformIO
2. On first boot or if the button is pressed:
   - Device enters AP mode (default SSID: `ESP32_Setup`, password: `87654321`)
   - Connect via phone/laptop and open the browser to `192.168.4.1`
   - Enter your WiFi SSID, password, and a unique Device ID
3. ESP32 will restart and attempt to connect to WiFi
4. If connected, it will sync with Firebase and display the text from path:  
