#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <EEPROM.h>
#include <Servo.h>

// Pins
const int servoPin = D5;
const int upButtonPin = D6;
const int downButtonPin = D7;
const int buzzerPin = D8; // Define the buzzer pin

Servo derailleurServo;

// Variables for gear and position tracking
int currentGear = 1;  // Start with gear 1
const int maxGear = 5;
const int minGear = 1;
float currentPosition = 0; // Servo position in degrees

// Winch parameters
const float drumCircumference = 31.42; // mm (10mm diameter drum)
float gearCablePull[12] = {0, 3.6, 7.2, 10.8, 14.4, 18.0, 21.6, 25.2, 28.8, 32.4, 36.0, 39.6}; // mm per gear

// Wi-Fi credentials
const char *ssid = "DerailleurControl";
const char *password = "12345678";

// Web server
AsyncWebServer server(80);
bool hotspotActive = false;

// Button debouncing and hotspot activation
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;
unsigned long hotspotActivationTime = 0;
const unsigned long hotspotActivationDelay = 3000; // 3 seconds
const unsigned long hotspotDeactivationDelay = 5000; // 5 seconds

void setup() {
  derailleurServo.attach(servoPin);

  pinMode(upButtonPin, INPUT_PULLUP);
  pinMode(downButtonPin, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT); // Set buzzer pin as output

  Serial.begin(115200);

  // Initialize EEPROM
  EEPROM.begin(512);
  loadLastGear();

  // Move servo to the last saved position
  moveToGear(currentGear);
}

void loop() {
  unsigned long currentTime = millis();

  // Check button states with debounce
  if (currentTime - lastDebounceTime > debounceDelay) {
    bool upPressed = digitalRead(upButtonPin) == LOW;
    bool downPressed = digitalRead(downButtonPin) == LOW;

    if (upPressed && downPressed) {
      if (hotspotActivationTime == 0) {
        hotspotActivationTime = currentTime;
      } else if (currentTime - hotspotActivationTime >= hotspotActivationDelay && !hotspotActive) {
        toggleHotspot();
        hotspotActivationTime = 0; // Reset activation time
      } else if (currentTime - hotspotActivationTime >= hotspotDeactivationDelay && hotspotActive) {
        toggleHotspot();
        hotspotActivationTime = 0; // Reset activation time
      }
    } else {
      hotspotActivationTime = 0; // Reset activation time if buttons are released
    }

    if (upPressed && !downPressed) {
      shiftUp();
      lastDebounceTime = currentTime;
    } else if (downPressed && !upPressed) {
      shiftDown();
      lastDebounceTime = currentTime;
    }
  }
}

// Shifting up
void shiftUp() {
  if (currentGear < maxGear) {
    currentGear++;
    moveToGear(currentGear);
    saveLastGear(); // Save the updated gear position
  }
}

// Shifting down
void shiftDown() {
  if (currentGear > minGear) {
    currentGear--;
    moveToGear(currentGear);
    saveLastGear(); // Save the updated gear position
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


// Toggle hotspot
void toggleHotspot() {
  if (hotspotActive) {
    WiFi.softAPdisconnect(true);
    server.end();
    hotspotActive = false;
    Serial.println("Hotspot and web server deactivated.");
    beep(3); // 3 beeps for deactivation
  } else {
    WiFi.softAP(ssid, password);
    setupWebServer();
    hotspotActive = true;
    Serial.println("Hotspot and web server activated.");
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
    String html = "<h1>Derailleur Control</h1>";
    html += "<form method='GET' action='/set'>";
    for (int i = 0; i < 12; i++) {
      html += "Gear " + String(i + 1) + ": <input type='number' name='pull" + String(i + 1) +
               "' value='" + String(gearCablePull[i]) + "'> mm<br>";
    }
    html += "<input type='submit' value='Update'>";
    html += "</form>";
    html += "<p><a href='/reset'>Reset to Default</a></p>";
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

  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request) {
    float defaultPulls[12] = {0, 3.6, 7.2, 10.8, 14.4, 18.0, 21.6, 25.2, 28.8, 32.4, 36.0, 39.6};
    memcpy(gearCablePull, defaultPulls, sizeof(gearCablePull));
    saveSettings(); // Save default settings to EEPROM
    request->send(200, "text/html", "Settings reset to default and saved! <a href='/'>Back</a>");
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
  for (int i = 0; i < 12; i++) {
    EEPROM.put(i * sizeof(float) + sizeof(int), gearCablePull[i]);
  }
  EEPROM.commit();
  Serial.println("Settings saved to EEPROM.");
}

void loadSettings() {
  for (int i = 0; i < 12; i++) {
    EEPROM.get(i * sizeof(float) + sizeof(int), gearCablePull[i]);
    if (gearCablePull[i] < 0 || gearCablePull[i] > 50) {
      gearCablePull[i] = i * 3.6; // Reset to default if values are out of range
    }
  }
  Serial.println("Settings loaded from EEPROM.");
}
