#include <Arduino.h>
#include <sensors.h>      // extern DHT dht;
#include <app_state.h>    // gState
#include <app_config.h>

void TaskDHT(void *pvParameters) {
  (void) pvParameters;
  for (;;) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (!isnan(t) && !isnan(h)) {
      gState.temperature = t;
      gState.humidity    = h;
      Serial.printf("[DHT22] T=%.1f°C  H=%.1f%%\n", t, h);
    } else {
      Serial.println("[DHT22]  NaN");
    }

    vTaskDelay(pdMS_TO_TICKS(3000));
  }
}
