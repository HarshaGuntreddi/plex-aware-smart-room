// now_playing_display.ino
// esp32 + ssd1306 oled
// show current plex now playing

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const char* WIFI_SSID = "your_ssid";
const char* WIFI_PASS = "your_password";

const char* SERVER_URL = "http://your-server-ip:5000/api/now-playing";

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void drawLines(const String &a, const String &b) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(a);
  display.setCursor(0, 16);
  display.println(b);
  display.display();
}

void setup() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for(;;) { delay(1000); }
  }

  drawLines("plex room", "starting");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(SERVER_URL);
    int code = http.GET();
    if (code == 200) {
      String payload = http.getString();
      StaticJsonDocument<1024> doc;
      if (!deserializeJson(doc, payload)) {
        bool playing = doc["playing"];
        if (playing) {
          JsonArray items = doc["items"].as<JsonArray>();
          if (!items.isNull() && items.size() > 0) {
            JsonObject item = items[0];
            String title = item["title"] | "unknown";
            String series = item["series"] | "";
            String state = item["state"] | "";
            String l1 = series.length() ? series : title;
            String l2 = series.length() ? title : state;
            drawLines(l1, l2);
          }
        } else {
          drawLines("idle", "");
        }
      }
    }
    http.end();
  }

  delay(3000);
}
