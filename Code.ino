#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <EEPROM.h>
#include <Servo.h>
#include <WebSocketsServer.h> // Include WebSockets library

// Pins
const int servoPin = D5;
const int upButtonPin = D6;
const int downButtonPin = D7;
const int buzzerPin = D8; // Define the buzzer pin

Servo derailleurServo;

// Variables for gear and position tracking
int currentGear = 1;  // Start with gear 1
int maxGear = 5;
const int minGear = 1;
float currentPosition = 0; // Servo position in degrees

// Winch parameters
const float drumCircumference = 31.42; // mm (10mm diameter drum)
float gearCablePull[12] = {0, 3.6, 7.2, 10.8, 14.4, 18.0, 21.6, 25.2, 28.8, 32.4, 36.0, 39.6}; // mm per gear

// Wi-Fi credentials
char ssid[32] = "DerailleurControl";
char password[32] = "12345678";

// Web server
AsyncWebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81); // Initialize WebSocket server on port 81
bool hotspotActive = false;

// Button debouncing and device activation
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;
unsigned long deviceActivationTime = 0;
const unsigned long deviceActivationDelay = 3000; // 3 seconds
const unsigned long deviceDeactivationDelay = 5000; // 5 seconds

void setup() {
  derailleurServo.attach(servoPin);

  pinMode(upButtonPin, INPUT_PULLUP);
  pinMode(downButtonPin, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT); // Set buzzer pin as output

  Serial.begin(115200);

  // Initialize EEPROM
  EEPROM.begin(512);
  loadLastGear();
  loadWiFiSettings();

  // Move servo to the last saved position
  moveToGear(currentGear);

  webSocket.begin(); // Start WebSocket server
  webSocket.onEvent(onWebSocketEvent); // Define WebSocket event handler
}

void loop() {
  unsigned long currentTime = millis();

  // Check button states with debounce
  if (currentTime - lastDebounceTime > debounceDelay) {
    bool upPressed = digitalRead(upButtonPin) == LOW;
    bool downPressed = digitalRead(downButtonPin) == LOW;

    if (upPressed && downPressed) {
      if (deviceActivationTime == 0) {
        deviceActivationTime = currentTime;
      } else if (currentTime - deviceActivationTime >= deviceActivationDelay && !hotspotActive) {
        toggleDevice();
        deviceActivationTime = 0; // Reset activation time
      } else if (currentTime - deviceActivationTime >= deviceDeactivationDelay && hotspotActive) {
        toggleDevice();
        deviceActivationTime = 0; // Reset activation time
      }
    } else {
      deviceActivationTime = 0; // Reset activation time if buttons are released

      // Only change gear if not both buttons are pressed
      if (upPressed && !downPressed) {
        shiftUp();
        lastDebounceTime = currentTime;
      } else if (downPressed && !upPressed) {
        shiftDown();
        lastDebounceTime = currentTime;
      }
    }
  }

  webSocket.loop(); // Handle WebSocket events
}

// Shifting up
void shiftUp() {
  if (currentGear < maxGear) {
    currentGear++;
    moveToGear(currentGear);
    saveLastGear(); // Save the updated gear position
    notifyClients(); // Notify WebSocket clients
  }
}

// Shifting down
void shiftDown() {
  if (currentGear > minGear) {
    currentGear--;
    moveToGear(currentGear);
    saveLastGear(); // Save the updated gear position
    notifyClients(); // Notify WebSocket clients
  }
}

// Move to a specific gear
void moveToGear(int gear) {
  if (gear < minGear || gear > maxGear) {
    Serial.println("Invalid gear! Ignoring command.");
    return;
  }

  // Calculate target position
  float targetPull = gearCablePull[gear - 1];
  float targetPosition = (targetPull / drumCircumference) * 360; // Convert mm to degrees

  // Clamp position within servo range (0° to 360°)
  if (targetPosition < 0) targetPosition = 0;
  if (targetPosition > 360) targetPosition = 360;

  // Move servo to target position
  derailleurServo.write(targetPosition / 180.0 * 180); // Map to servo range
  currentPosition = targetPosition;

  // Update current gear
  currentGear = gear;

  Serial.println("Moved to gear " + String(gear) + 
                 " (Cable Pull: " + String(targetPull) + 
                 " mm, Servo Position: " + String(targetPosition) + " degrees)");
}


// Toggle device
void toggleDevice() {
  if (hotspotActive) {
    WiFi.softAPdisconnect(true);
    server.end();
    hotspotActive = false;
    Serial.println("Device and web server deactivated.");
    beep(3); // 3 beeps for deactivation
  } else {
    WiFi.softAP(ssid, password);
    setupWebServer();
    hotspotActive = true;
    Serial.println("Device and web server activated.");
    beep(2); // 2 beeps for activation
  }
}

// Beep function
void beep(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(buzzerPin, HIGH);
    delay(100);
    digitalWrite(buzzerPin, LOW);
    delay(100);
  }
}

// Setup web server
void setupWebServer() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1.0'><style>";
    html += "body{font-family:'Roboto', sans-serif;max-width:600px;margin:auto;padding:10px;background-color:#f4f4f9;color:#333;}";
    html += "h1{text-align:center;color:#444;}";
    html += "h2{text-align:center;}";
    html += "form{display:flex;flex-direction:column;align-items:flex-start;}";
    html += ".gear-info{font-size:1.5em;font-weight:bold;color:#007bff;margin:10px 0;}";
    html += "input[type=number]{width:60px;padding:5px;margin:5px 0;}";
    html += "button{width:30px;height:30px;margin:5px;background-color:#007bff;color:white;border:none;border-radius:5px;cursor:pointer;}";
    html += "button:hover{background-color:#0056b3;}";
    html += ".sent{background-color:lightgreen;}";
    html += "a{color:#007bff;text-decoration:none;}";
    html += "a:hover{text-decoration:underline;}";
    html += ".reset-button{display:inline-block;padding:10px 20px;margin:10px 0;background-color:#dc3545;color:white;text-align:center;text-decoration:none;border-radius:5px;}";
    html += ".reset-button:hover{background-color:#c82333;}";
    html += "small{font-size:0.8em;font-style:italic;color:#666;}"; // Add style for small italic text
    html += "</style></head><body>";
    html += "<h1>RBE e-Derailleur</h1>";
    html += "<form id='gearForm'>";
    html += "<div>Max Gear:</div>"; // Place Max Gear text above
    html += "<div><button type='button' onclick='changeValue(\"maxGear\", -1)'>-</button>";
    html += "<input type='number' id='maxGear' name='maxGear' value='" + String(maxGear) + "' step='1' min='1' max='12' readonly pattern='[0-9]*' style='background-color:#e9ecef;border:none;font-weight:bold;'>";
    html += "<button type='button' onclick='changeValue(\"maxGear\", 1)'>+</button>";
    html += "<button type='button' onclick='sendMaxGear()'>OK</button></div><br>";
    html += "<hr style='margin:20px 0;'>"; // Add line separator
    html += "<small>Input value in mm. Default setting is for Shimano Deore XT 12 speed.</small>"; // Add information in smaller font and italic
    for (int i = 0; i < maxGear; i++) {
      html += "<div>Gear " + String(i + 1) + ": <button type='button' onclick='changeValue(\"pull" + String(i + 1) + "\", -0.1)'>-</button>";
      html += "<input type='number' id='pull" + String(i + 1) + "' name='pull" + String(i + 1) + "' value='" + String(gearCablePull[i]) + "' step='0.1'>";
      html += "<button type='button' onclick='changeValue(\"pull" + String(i + 1) + "\", 0.1)'>+</button>";
      html += "<button type='button' onclick='sendSetting(" + String(i + 1) + ")'>OK</button></div><br>";
    }
    html += "</form>";
    html += "<a href='/reset' class='reset-button'>Reset to Default</a>"; // Style Reset to Default link
    html += "<hr style='margin:20px 0;'>"; // Add line separator
    html += "<h2>Wireless Gear Shifter</h2>"; // Add title
    html += "<p>Current Gear: <span id='currentGear' class='gear-info'>" + String(currentGear) + "</span></p>"; // Move current gear info
    html += "<div style='display:flex;justify-content:space-between;'>";
    html += "<button style='width:45%;height:50px;font-size:1.5em;' onclick='shiftGear(\"down\")'>DOWN</button>";
    html += "<button style='width:45%;height:50px;font-size:1.5em;' onclick='shiftGear(\"up\")'>UP</button>";
    html += "</div><br>";
    html += "<hr style='margin:20px 0;'>"; // Add line separator
    html += "<p><a href='/settings'>Wi-Fi Settings</a></p>";
    html += "<p>To deactivate wireless setting, press physical up and down buttons for 10 seconds.</p>"; // Add info text
    html += "<script>function changeValue(id, delta) { var input = document.getElementById(id); var newValue = parseInt(input.value) + delta; if (newValue > 12) newValue = 12; if (newValue < 1) newValue = 1; input.value = newValue; }</script>";
    html += "<script>function sendSetting(gear) { var input = document.getElementById('pull' + gear); var xhr = new XMLHttpRequest(); xhr.open('GET', '/set?pull' + gear + '=' + input.value, true); xhr.onload = function() { if (xhr.status == 200) { document.getElementById('gear' + gear).classList.add('sent'); location.reload(); } }; xhr.send(); }</script>";
    html += "<script>document.getElementById('maxGear').addEventListener('change', function() { var xhr = new XMLHttpRequest(); xhr.open('GET', '/setMaxGear?maxGear=' + this.value, true); xhr.onload = function() { if (xhr.status == 200) { location.reload(); } }; xhr.send(); });</script>";
    html += "<script>function sendMaxGear() { var input = document.getElementById('maxGear'); var xhr = new XMLHttpRequest(); xhr.open('GET', '/setMaxGear?maxGear=' + input.value, true); xhr.onload = function() { if (xhr.status == 200) { location.reload(); } }; xhr.send(); }</script>";
    html += "<script>function shiftGear(direction) { var xhr = new XMLHttpRequest(); xhr.open('GET', '/' + direction, true); xhr.send(); }</script>";
    html += "<script>var ws = new WebSocket('ws://' + window.location.hostname + ':81/');";
    html += "ws.onmessage = function(event) { var data = JSON.parse(event.data); document.getElementById('currentGear').innerText = data.gear; };</script>";
    html += "</body></html>";
    request->send(200, "text/html", html);
  });

  server.on("/set", HTTP_GET, [](AsyncWebServerRequest *request) {
    for (int i = 0; i < 12; i++) {
      String paramName = "pull" + String(i + 1);
      if (request->hasParam(paramName)) {
        gearCablePull[i] = request->getParam(paramName)->value().toFloat();
      }
    }
    saveSettings(); // Save to EEPROM
    request->send(200, "text/html", "Settings updated and saved! <a href='/'>Back</a>");
  });

  server.on("/setMaxGear", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("maxGear")) {
      maxGear = request->getParam("maxGear")->value().toInt();
      saveSettings(); // Save to EEPROM
    }
    request->send(200, "text/html", "Max gear updated and saved! <a href='/'>Back</a>");
  });

  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request) {
    float defaultPulls[12] = {0, 3.6, 7.2, 10.8, 14.4, 18.0, 21.6, 25.2, 28.8, 32.4, 36.0, 39.6};
    memcpy(gearCablePull, defaultPulls, sizeof(gearCablePull));
    saveSettings(); // Save default settings to EEPROM
    request->send(200, "text/plain", "Settings reset to default.");
  });

  server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1.0'><style>";
    html += "body{font-family:'Roboto', sans-serif;max-width:600px;margin:auto;padding:10px;background-color:#f4f4f9;color:#333;}";
    html += "h1{color:#444;}";
    html += "form{display:flex;flex-direction:column;align-items:flex-start;}";
    html += "input[type=text]{width:200px;padding:5px;margin:5px 0;}";
    html += "button{width:100px;height:30px;margin:5px;background-color:#007bff;color:white;border:none;border-radius:5px;cursor:pointer;}";
    html += "button:hover{background-color:#0056b3;}";
    html += "a{color:#007bff;text-decoration:none;}";
    html += "a:hover{text-decoration:underline;}";
    html += "</style></head><body>";
    html += "<h1>Wi-Fi Settings</h1>";
    html += "<form action='/applySettings' method='POST'>";
    html += "<div>SSID: <input type='text' name='ssid' value='" + String(ssid) + "'></div><br>";
    html += "<div>Password: <input type='text' name='password' value='" + String(password) + "'></div><br>";
    html += "<button type='submit'>Apply</button>";
    html += "</form>";
    html += "<p><a href='/'>Back to Main Page</a></p>";
    html += "</body></html>";
    request->send(200, "text/html", html);
  });

  server.on("/applySettings", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("ssid", true) && request->hasParam("password", true)) {
      strncpy(ssid, request->getParam("ssid", true)->value().c_str(), sizeof(ssid));
      strncpy(password, request->getParam("password", true)->value().c_str(), sizeof(password));
      saveWiFiSettings();
      request->send(200, "text/html", "Settings applied! Redirecting to main page...");
      delay(2000); // Delay to allow the response to be sent
      request->redirect("/"); // Redirect to main page
      delay(2000); // Delay to allow the redirect to complete
      ESP.restart(); // Restart the device to apply new settings
    } else {
      request->send(400, "text/html", "Invalid input! <a href='/settings'>Back</a>");
    }
  });

  server.on("/restartDevice", HTTP_GET, [](AsyncWebServerRequest *request) {
    toggleDevice();
    request->send(200, "text/html", "Device restarted! <a href='/'>Back to Main Page</a>");
  });

  server.on("/up", HTTP_GET, [](AsyncWebServerRequest *request) {
    shiftUp();
    request->send(200, "text/plain", "Shifted Up");
  });

  server.on("/down", HTTP_GET, [](AsyncWebServerRequest *request) {
    shiftDown();
    request->send(200, "text/plain", "Shifted Down");
  });

  server.begin();
}

// Save and load gear position to/from EEPROM
void saveLastGear() {
  EEPROM.put(0, currentGear);
  EEPROM.commit();
  Serial.println("Saved current gear: " + String(currentGear));
}

void loadLastGear() {
  EEPROM.get(0, currentGear);
  if (currentGear < minGear || currentGear > maxGear) {
    currentGear = 1; // Default to gear 1 if out of range
  }
  Serial.println("Loaded last gear: " + String(currentGear));
}

// Save and load gear settings to/from EEPROM
void saveSettings() {
  EEPROM.put(0, currentGear);
  EEPROM.put(sizeof(int), maxGear);
  for (int i = 0; i < 12; i++) {
    EEPROM.put(i * sizeof(float) + 2 * sizeof(int), gearCablePull[i]);
  }
  EEPROM.commit();
  Serial.println("Settings saved to EEPROM.");
}

void loadSettings() {
  EEPROM.get(0, currentGear);
  EEPROM.get(sizeof(int), maxGear);
  for (int i = 0; i < 12; i++) {
    EEPROM.get(i * sizeof(float) + 2 * sizeof(int), gearCablePull[i]);
    if (gearCablePull[i] < 0 || gearCablePull[i] > 50) {
      gearCablePull[i] = i * 3.6; // Reset to default if values are out of range
    }
  }
  Serial.println("Settings loaded from EEPROM.");
}

// Save and load Wi-Fi settings to/from EEPROM
void saveWiFiSettings() {
  EEPROM.put(2 * sizeof(int) + 12 * sizeof(float), ssid);
  EEPROM.put(2 * sizeof(int) + 12 * sizeof(float) + sizeof(ssid), password);
  EEPROM.commit();
  Serial.println("Wi-Fi settings saved to EEPROM.");
}

void loadWiFiSettings() {
  EEPROM.get(2 * sizeof(int) + 12 * sizeof(float), ssid);
  EEPROM.get(2 * sizeof(int) + 12 * sizeof(float) + sizeof(ssid), password);
  Serial.println("Wi-Fi settings loaded from EEPROM.");
  // Activate hotspot after loading Wi-Fi settings
  WiFi.softAP(ssid, password);
  setupWebServer();
  hotspotActive = true;
  Serial.println("Hotspot activated after loading Wi-Fi settings.");
}

void notifyClients() {
  String message = "{\"gear\":" + String(currentGear) + "}";
  webSocket.broadcastTXT(message); // Broadcast current gear to all WebSocket clients
}

void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  // Handle WebSocket events (if needed)
}
