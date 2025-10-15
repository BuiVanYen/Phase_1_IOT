#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <app_config.h>

static WiFiManager wm;

void wifiAutoOrPortal() {
  wm.setConnectTimeout(WIFI_CONNECT_TIMEOUT_S);
  wm.setConfigPortalTimeout(WIFI_PORTAL_TIMEOUT_S);

  bool ok = wm.autoConnect(WIFI_PORTAL_SSID, WIFI_PORTAL_PASS);
  if (!ok) {
    Serial.println("[WiFi] Portal timeout -> restart");
    ESP.restart();
  }

  Serial.print("[WiFi] Connected SSID: ");
  Serial.println(WiFi.SSID());
  Serial.print("[WiFi] IP: ");
  Serial.println(WiFi.localIP());
}
