#include <Arduino.h>
#include <app_state.h>
#include <app_config.h>

void TaskRelay(void *pvParameters) {
  (void) pvParameters;

  for (;;) {
    // 1. Nhiệt độ cao → bật quạt (relay1)
    if (gState.temperature > TEMP_FAN_ON_C)
      digitalWrite(PIN_RELAY1, HIGH);
    else
      digitalWrite(PIN_RELAY1, LOW);

    // 2. Ẩm đất thấp → bật bơm (relay2)
    if (gState.soilPercent <= SOIL_PCT_PUMP_ON)
      digitalWrite(PIN_RELAY2, HIGH);
    else if (gState.soilPercent >= SOIL_PCT_PUMP_OFF)
      digitalWrite(PIN_RELAY2, LOW);

    // 3. Ánh sáng yếu → bật đèn (relay3)
    if (gState.lux < LUX_LIGHT_ON)
      digitalWrite(PIN_RELAY3, HIGH);
    else
      digitalWrite(PIN_RELAY3, LOW);

    // 4. Relay4: dự phòng, luôn tắt
    digitalWrite(PIN_RELAY4, LOW);

    // Debug
    Serial.printf("[Relay] T=%.1f°C | Soil=%.1f%% | Lux=%.1f  --> R1=%d R2=%d R3=%d\n",
                  gState.temperature, gState.soilPercent, gState.lux,
                  digitalRead(PIN_RELAY1),
                  digitalRead(PIN_RELAY2),
                  digitalRead(PIN_RELAY3));

    vTaskDelay(pdMS_TO_TICKS(2000)); // kiểm tra mỗi 2 giây
  }
}
