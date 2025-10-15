#include <Arduino.h>
#include <app_config.h>
#include <app_state.h>
#include <tasks.h>
#include <wifi_portal.h>
#include <hw_init.h>

void setup() {
  Serial.begin(9600);
  delay(200);

  // 1) Wi-Fi
  wifiAutoOrPortal();

  // 2) Sensor & Relay
  hwInitSensors();
  hwInitRelays();

  Serial.println("=== ESP32 Greenhouse – FreeRTOS Core (Modular) ===");

  // 3) Tasks
  xTaskCreatePinnedToCore(TaskDHT,   "TaskDHT",   4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(TaskSoil,  "TaskSoil",  4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(TaskLight, "TaskLight", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(TaskRelay, "TaskRelay", 4096, NULL, 1, NULL, 1);
}

void loop() {
  // FreeRTOS điều phối
}
