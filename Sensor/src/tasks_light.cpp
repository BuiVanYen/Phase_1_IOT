#include <Arduino.h>
#include <sensors.h>      // extern BH1750 lightMeter;
#include <app_state.h>
#include <app_config.h>

void TaskLight(void *pvParameters) {
  (void) pvParameters;
  for (;;) {
    float lux = lightMeter.readLightLevel();
    gState.lux = lux;
    Serial.printf("[Light] %.2f lux\n", lux);

    vTaskDelay(pdMS_TO_TICKS(4000));
  }
}
