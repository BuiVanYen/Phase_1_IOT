#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "secrets.h"
#include <app_state.h>
#include "thingspeak.h"

// Giới hạn ThingSpeak free: >= 15 giây / 1 lần update / kênh
static const uint32_t TS_MIN_INTERVAL_MS = 15000;
static uint32_t s_lastPost = 0;

bool ts_publish(float t, float h, float soil, float lux) {
  if (millis() - s_lastPost < TS_MIN_INTERVAL_MS) return false;
  if (WiFi.status() != WL_CONNECTED) return false;

  HTTPClient http;
  http.begin("https://api.thingspeak.com/update.json");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

 
  String body = String("api_key=") + TS_WRITE_API_KEY +
                "&field1=" + String(t, 1) +
                "&field2=" + String(h, 1) +
                "&field3=" + String(soil, 1) +
                "&field4=" + String(lux, 1);

  int code = http.POST(body);
  String resp = http.getString();
  http.end();

  if (code == 200) {
    s_lastPost = millis();
    Serial.printf("[TS] OK: %s\n", resp.c_str()); 
    return true;
  } else {
    Serial.printf("[TS] FAIL %d: %s\n", code, resp.c_str());
    return false;
  }
}
void TaskThingSpeak(void *pvParameters) {
  (void) pvParameters;

  for (;;) {
    
    float t = gState.temperature;
    float h = gState.humidity;
    float s = gState.soilPercent;
    float l = gState.lux;

    
    ts_publish(t, h, s, l);

    vTaskDelay(pdMS_TO_TICKS(200)); 
  }
}