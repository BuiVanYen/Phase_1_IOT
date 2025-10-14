#include <Arduino.h>
#include <Wire.h>
#include <DHT.h>
#include <BH1750.h>

// ---------------- CẤU HÌNH PHẦN CỨNG ----------------
#define DHTPIN   14
#define DHTTYPE  DHT22
#define SOIL_AO  34
#define RELAY1   25
#define RELAY2   26
#define RELAY3   27
#define RELAY4   33

// ---------------- BIẾN TOÀN CỤC ----------------
DHT dht(DHTPIN, DHTTYPE);
BH1750 lightMeter;

float temperature = 0, humidity = 0, lux = 0, soilPercent=0;
int soilADC = 0;


// ---------------- prototype ----------------
void TaskDHT(void *pvParameters);
void TaskSoil(void *pvParameters);
void TaskLight(void *pvParameters);
void TaskRelay(void *pvParameters);

// =====================================================
void setup() {
  Serial.begin(9600);
  delay(500);

  // --- Khởi tạo cảm biến ---
  dht.begin();
  Wire.begin(21, 22);
  lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);

  // --- Relay ---
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(RELAY3, OUTPUT);
  pinMode(RELAY4, OUTPUT);

  // Tất cả relay TẮT lúc khởi động 
  digitalWrite(RELAY1, LOW);
  digitalWrite(RELAY2, LOW);
  digitalWrite(RELAY3, LOW);
  digitalWrite(RELAY4, LOW);


  Serial.println("=== ESP32 FreeRTOS Sensor Reader ===");

  // --- Tạo task ---
  xTaskCreatePinnedToCore(TaskDHT,   "TaskDHT",   4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(TaskSoil,  "TaskSoil",  4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(TaskLight, "TaskLight", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(TaskRelay, "TaskRelay", 4096, NULL, 1, NULL, 1);
}

void loop() {
  // Không dùng loop – FreeRTOS tự quản lý task
}

// =====================================================
// TASK 1: Đọc nhiệt độ & độ ẩm DHT22
void TaskDHT(void *pvParameters) {
  for (;;) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) {
      temperature = t;
      humidity = h;
      Serial.printf("[DHT22] T=%.1f°C  H=%.1f%%\n", temperature, humidity);
    } else {
      Serial.println("[DHT22] ❌ Lỗi đọc cảm biến");
    }
    vTaskDelay(pdMS_TO_TICKS(3000));
  }
}

// =====================================================
// TASK 2: Đọc độ ẩm đất (Analog + Digital)
void TaskSoil(void *pvParameters) {
  const int ADC_DRY = 3700;  // hiệu chỉnh: đất khô
  const int ADC_WET = 1500;  // hiệu chỉnh: đất ướt

  for (;;) {
    soilADC = analogRead(SOIL_AO);

    // Chuyển sang phần trăm ẩm đất
    soilPercent = map(soilADC, ADC_DRY, ADC_WET, 0, 100);
    soilPercent = constrain(soilPercent, 0, 100);

    Serial.printf("[Soil] ADC=%d | %.1f%% ẩm đất\n", soilADC, soilPercent);

    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

// =====================================================
// TASK 3: Đọc cảm biến ánh sáng BH1750
void TaskLight(void *pvParameters) {
  for (;;) {
    lux = lightMeter.readLightLevel();
    Serial.printf("[Light] %.2f lux\n", lux);
    vTaskDelay(pdMS_TO_TICKS(4000));
  }
}

// =====================================================
// TASK 4: Điều khiển relay theo điều kiện cơ bản
void TaskRelay(void *pvParameters) {
  for (;;) {
    // nhiệt độ cao bật quạt (relay1)
    if (temperature > 32.0) digitalWrite(RELAY1, HIGH);
    else digitalWrite(RELAY1, LOW);

    // Đất khô (DO = 1) bật bơm (relay2)
    if (soilPercent <= 35.0) digitalWrite(RELAY2, HIGH);
    else if (soilPercent >= 45.0) digitalWrite(RELAY2, LOW);

    // Thiếu sáng bật đèn (relay3)
    if (lux < 100.0) digitalWrite(RELAY3, HIGH);
    else digitalWrite(RELAY3, LOW);

    // Relay4: dự phòng, chưa dùng
    digitalWrite(RELAY4, LOW);

    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}
