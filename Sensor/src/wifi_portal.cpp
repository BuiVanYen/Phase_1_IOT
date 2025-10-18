#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <app_config.h>
#include <time.h>


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

    // Time sync via NTP
  configTime(7*3600, 0, "pool.ntp.org", "time.google.com");
  struct tm ti;
  if (getLocalTime(&ti, 5000)) {
    Serial.printf("[NTP] %04d-%02d-%02d %02d:%02d:%02d\n",
                  ti.tm_year+1900, ti.tm_mon+1, ti.tm_mday,
                  ti.tm_hour, ti.tm_min, ti.tm_sec);
  } else {
    Serial.println("[NTP] sync failed (5s)");
  }

}
