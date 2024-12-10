#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Servo.h>

// Pins
const int servoPin = D5;
const int upButtonPin = D6;
const int downButtonPin = D7;

Servo derailleurServo;

// Variables for gear and position tracking
int currentGear = 1;
const int maxGear = 12;
const int minGear = 1;
float currentPosition = 0; // Current servo position in degrees

// Winch parameters
const float drumCircumference = 31.42; // mm (10mm diameter drum)
float gearCablePull[12] = {0, 3.6, 7.2, 10.8, 14.4, 18.0, 21.6, 25.2, 28.8, 32.4, 36.0, 39.6}; // mm per gear

// Wi-Fi credentials
const char *ssid = "DerailleurControl";
const char *password = "12345678";

// Web server
AsyncWebServer server(80);
bool hotspotActive = false;

void setup() {
  derailleurServo.attach(servoPin);

  pinMode(upButtonPin, INPUT_PULLUP);
  pinMode(downButtonPin, INPUT_PULLUP);

  Serial.begin(115200);

  // Initialize servo position
  moveToGear(currentGear);
}

void loop() {
  // Check button states
  bool upPressed = digitalRead(upButtonPin) == LOW;
  bool downPressed = digitalRead(downButtonPin) == LOW;

  // Toggle hotspot if both buttons are pressed simultaneously
  if (upPressed && downPressed) {
    toggleHotspot();
    delay(500); // Debounce
  } else if (upPressed) {
    shiftUp();
    delay(300); // Debounce
  } else if (downPressed) {
    shiftDown();
    delay(300); // Debounce
  }
}

// Shifting up
void shiftUp() {
  if (currentGear < maxGear) {
    currentGear++;
    moveToGear(currentGear);
  }
}

// Shifting down
void shiftDown() {
  if (currentGear > minGear) {
    currentGear--;
    moveToGear(currentGear);
  }
}

// Move to a specific gear
void moveToGear(int gear) {
  float targetPull = gearCablePull[gear - 1];
  float targetPosition = (targetPull / drumCircumference) * 360; // Convert mm to degrees

  derailleurServo.write(targetPosition / 180.0 * 180); // Convert degrees to servo range
  currentPosition = targetPosition;

  Serial.println("Moved to gear " + String(gear) + " (Cable Pull: " + String(targetPull) +
                 " mm, Servo Position: " + String(targetPosition) + " degrees)");
}

// Toggle hotspot
void toggleHotspot() {
  if (hotspotActive) {
    WiFi.softAPdisconnect(true);
    server.end();
    hotspotActive = false;
    Serial.println("Hotspot and web server deactivated.");
  } else {
    WiFi.softAP(ssid, password);
    setupWebServer();
    hotspotActive = true;
    Serial.println("Hotspot and web server activated.");
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
    request->send(200, "text/html", "Settings updated! <a href='/'>Back</a>");
  });

  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request) {
    float defaultPulls[12] = {0, 3.6, 7.2, 10.8, 14.4, 18.0, 21.6, 25.2, 28.8, 32.4, 36.0, 39.6};
    memcpy(gearCablePull, defaultPulls, sizeof(gearCablePull));
    request->send(200, "text/html", "Settings reset to default! <a href='/'>Back</a>");
  });

  server.begin();
}
