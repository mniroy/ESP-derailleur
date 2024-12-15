# ESP-derailleur

## Overview
This project controls a derailleur using an ESP8266 microcontroller. The system allows for shifting gears via buttons and a web interface. The current gear and settings are saved to EEPROM to retain state between power cycles. The web interface now updates the current gear in real-time using WebSockets.

## Components
- **ESP8266**: The microcontroller used for Wi-Fi connectivity and control.
- **Servo**: Controls the derailleur position.
- **Buttons**: Used for shifting up and down.
- **EEPROM**: Stores the current gear and settings.
- **Buzzer**: Provides audio feedback for hotspot activation and deactivation.

## Pin Configuration
- **servoPin (D5)**: Connected to the servo motor.
- **upButtonPin (D6)**: Connected to the button for shifting up.
- **downButtonPin (D7)**: Connected to the button for shifting down.
- **buzzerPin (D8)**: Connected to the buzzer.

## Key Variables
- **currentGear**: Tracks the current gear.
- **maxGear**: Maximum gear limit. (up to 12)
- **minGear**: Minimum gear limit.
- **currentPosition**: Current servo position in degrees.
- **gearCablePull**: Array storing the cable pull in mm for each gear.

## Wi-Fi and Web Server
- **ssid**: Wi-Fi SSID for the hotspot.
- **password**: Password for the Wi-Fi hotspot.
- **server**: AsyncWebServer instance for handling web requests.
- **webSocket**: WebSocketsServer instance for real-time updates.
- **hotspotActive**: Boolean indicating if the hotspot is active.

## Web Interface
The web interface allows users to:
- View the current gear position in real-time.
- Update gear cable pull settings.
- Update the maximum gear limit.
- Reset settings to default values.
- Shift gears using UP and DOWN buttons.

### Editing Derailleur Settings
1. **Connect to the Wi-Fi hotspot**: Use the SSID and password provided in the code.
2. **Open a web browser**: Navigate to the default web server address: `http://192.168.4.1`.
3. **View current gear**: The current gear position is displayed at the top of the page and updates in real-time.
4. **Update gear settings**:
   - On the main page, you will see a form with input fields for each gear.
   - Enter the desired cable pull values (in mm) for each gear.
   - Click the "Send" button next to each gear to save the new setting.
   - The input field will turn green to indicate the setting has been sent.
5. **Update maximum gear limit**:
   - Change the value in the "Max Gear" input field.
   - The new maximum gear limit will be saved automatically.
6. **Shift gears**:
   - Use the "UP" and "DOWN" buttons to shift gears.
7. **Reset to default settings**:
   - Click the "Reset to Default" link to reset all gear settings to their default values.

## Usage
1. **Power on the device**: The system initializes and moves the servo to the last saved gear position.
2. **Shift gears**: Use the up and down buttons to shift gears.
3. **Activate the hotspot**: 
   - Press both the up and down buttons for 3 seconds to activate the hotspot.
   - The hotspot SSID is `DerailleurControl` and the password is `12345678`.
   - The buzzer will beep twice to indicate the hotspot is activated.
4. **Deactivate the hotspot**: 
   - Press both the up and down buttons for 5 seconds to deactivate the hotspot.
   - The buzzer will beep three times to indicate the hotspot is deactivated.
5. **Access the web interface**: Connect to the Wi-Fi hotspot and navigate to the web server to update settings.
   - Default web server address: `http://192.168.4.1`

## Dependencies
To compile and upload the code to the ESP8266, you need to install the following libraries:
- **ESP8266WiFi**: Provides Wi-Fi connectivity.
- **ESPAsyncWebServer**: Handles the web server functionality.
- **EEPROM**: Manages EEPROM read/write operations.
- **Servo**: Controls the servo motor.
- **WebSocketsServer**: Provides WebSocket functionality for real-time updates.

### Installing Dependencies
1. **ESP8266WiFi** and **EEPROM** libraries are included with the ESP8266 core for Arduino. Install the ESP8266 core by following the instructions [here](https://github.com/esp8266/Arduino#installing-with-boards-manager).
2. **ESPAsyncWebServer**:
   - Install the ESPAsyncWebServer library from the Arduino Library Manager or download it from [GitHub](https://github.com/me-no-dev/ESPAsyncWebServer).
   - Additionally, install the **AsyncTCP** library, which is a dependency for ESPAsyncWebServer.
3. **Servo**:
   - Install the Servo library from the Arduino Library Manager or download it from [GitHub](https://github.com/arduino-libraries/Servo).
4. **WebSocketsServer**:
   - Install the WebSocketsServer library from the Arduino Library Manager or download it from [GitHub](https://github.com/Links2004/arduinoWebSockets).

Make sure to include these libraries in your Arduino IDE before compiling and uploading the code.