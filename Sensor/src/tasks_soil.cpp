#include <Arduino.h>
#include <app_state.h>
#include <app_config.h>

void TaskSoil(void *pvParameters) {
  (void) pvParameters;

  for (;;) {
    // Đọc giá trị ADC của cảm biến độ ẩm đất
    int adc = analogRead(PIN_SOIL_AO);
    gState.soilADC = adc;

    // Chuyển sang % ẩm đất theo 2 mốc hiệu chuẩn
    float pct = map(adc, SOIL_ADC_DRY, SOIL_ADC_WET, 0, 100);
    pct = constrain(pct, 0, 100);
    gState.soilPercent = pct;

    Serial.printf("[Soil] ADC=%d  |  %.1f%% ẩm đất\n", adc, pct);

    vTaskDelay(pdMS_TO_TICKS(2000)); // đọc mỗi 2 giây
  }
}

