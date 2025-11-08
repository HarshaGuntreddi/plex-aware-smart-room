// light_controller.ino
// esp32 http relay for lights

#include <WiFi.h>
#include <WebServer.h>

const char* WIFI_SSID = "your_ssid";
const char* WIFI_PASS = "your_password";

const int RELAY_PIN = 5;

WebServer server(80);

void handle_dim() {
  digitalWrite(RELAY_PIN, HIGH);
  server.send(200, "text/plain", "dim");
}

void handle_normal() {
  digitalWrite(RELAY_PIN, LOW);
  server.send(200, "text/plain", "normal");
}

void handle_root() {
  server.send(200, "text/plain", "ok");
}

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  server.on("/", handle_root);
  server.on("/lights/dim", handle_dim);
  server.on("/lights/normal", handle_normal);
  server.begin();
}

void loop() {
  server.handleClient();
}
