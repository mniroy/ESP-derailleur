# ESP-derailleur

## Overview
This project controls a derailleur using an ESP8266 microcontroller. The system allows for shifting gears via buttons and a web interface. The current gear and settings are saved to EEPROM to retain state between power cycles.

## Components
- **ESP8266**: The microcontroller used for Wi-Fi connectivity and control.
- **Servo**: Controls the derailleur position.
- **Buttons**: Used for shifting up and down.
- **EEPROM**: Stores the current gear and settings.

## Pin Configuration
- **servoPin (D5)**: Connected to the servo motor.
- **upButtonPin (D6)**: Connected to the button for shifting up.
- **downButtonPin (D7)**: Connected to the button for shifting down.

## Key Variables
- **currentGear**: Tracks the current gear.
- **maxGear**: Maximum gear limit.
- **minGear**: Minimum gear limit.
- **currentPosition**: Current servo position in degrees.
- **gearCablePull**: Array storing the cable pull in mm for each gear.

## Wi-Fi and Web Server
- **ssid**: Wi-Fi SSID for the hotspot.
- **password**: Password for the Wi-Fi hotspot.
- **server**: AsyncWebServer instance for handling web requests.
- **hotspotActive**: Boolean indicating if the hotspot is active.

## Web Interface
The web interface allows users to:
- View and update gear cable pull settings.
- Reset settings to default values.

### Editing Derailleur Settings
1. **Connect to the Wi-Fi hotspot**: Use the SSID and password provided in the code.
2. **Open a web browser**: Navigate to the default web server address: `http://192.168.4.1`.
3. **Update gear settings**:
   - On the main page, you will see a form with input fields for each gear.
   - Enter the desired cable pull values (in mm) for each gear.
   - Click the "Update" button to save the new settings.
4. **Reset to default settings**:
   - Click the "Reset to Default" link to reset all gear settings to their default values.

## Usage
1. **Power on the device**: The system initializes and moves the servo to the last saved gear position.
2. **Shift gears**: Use the up and down buttons to shift gears.
3. **Access the web interface**: Connect to the Wi-Fi hotspot and navigate to the web server to update settings.
   - Default web server address: `http://192.168.4.1`