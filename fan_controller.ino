// fan_controller.ino
// esp32 + ds18b20 + relay
// if plex playing and temp high => fan on

#include <WiFi.h>
#include <HTTPClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

const char* WIFI_SSID = "your_ssid";
const char* WIFI_PASS = "your_password";

const char* SERVER_URL = "http://your-server-ip:5000/api/is-playing";

const int ONE_WIRE_BUS = 4;
const int RELAY_PIN = 5;
const float TEMP_THRESHOLD = 28.0;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

unsigned long lastCheck = 0;
const unsigned long CHECK_MS = 10000;

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  sensors.begin();
}

bool isPlaying() {
  if (WiFi.status() != WL_CONNECTED) return false;

  HTTPClient http;
  http.begin(SERVER_URL);
  int code = http.GET();
  if (code != 200) {
    http.end();
    return false;
  }
  String body = http.getString();
  http.end();

  return body.indexOf("\"playing\":true") != -1;
}

float readTemp() {
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(0);
}

void loop() {
  unsigned long now = millis();
  if (now - lastCheck > CHECK_MS) {
    lastCheck = now;

    float t = readTemp();
    bool playing = isPlaying();

    if (playing && t >= TEMP_THRESHOLD) {
      digitalWrite(RELAY_PIN, HIGH);
    } else {
      digitalWrite(RELAY_PIN, LOW);
    }
  }

  delay(200);
}
