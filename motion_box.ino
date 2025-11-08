// motion_box.ino
// esp32 + pir
// if no motion for 2 min send pause to server

#include <WiFi.h>
#include <HTTPClient.h>

const char* WIFI_SSID = "your_ssid";
const char* WIFI_PASS = "your_password";

const char* SERVER_URL = "http://your-server-ip:5000/api/pause-from-motion";
const char* MOTION_SECRET = "change_this_secret";

const int PIR_PIN = 14;
const unsigned long NO_MOTION_MS = 2UL * 60UL * 1000UL;

unsigned long lastMotion = 0;
bool sent = false;

void setup() {
  pinMode(PIR_PIN, INPUT);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  lastMotion = millis();
}

void sendPause() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "application/json");
  String body = String("{\"secret\":\"") + MOTION_SECRET + "\"}";
  int code = http.POST(body);
  http.end();
}

void loop() {
  int motion = digitalRead(PIR_PIN);
  unsigned long now = millis();

  if (motion == HIGH) {
    lastMotion = now;
    sent = false;
  }

  if (!sent && (now - lastMotion) > NO_MOTION_MS) {
    sendPause();
    sent = true;
  }

  delay(200);
}
