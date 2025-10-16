#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>           
#include "app_config.h"
#include "app_state.h"
#include "mqtt_config.h"

static WiFiClient   net;
static PubSubClient mqtt(net);
static Preferences  prefs;        

/* ================== NGƯỠNG ĐỘNG (mặc định lấy từ app_config.h) ================== */
struct Thresholds {
  float tempOn  = TEMP_FAN_ON_C;
  float tempOff = TEMP_FAN_OFF_C;
  float soilOn  = SOIL_PCT_PUMP_ON;
  float soilOff = SOIL_PCT_PUMP_OFF;
  float luxOn   = LUX_LIGHT_ON;
  float luxOff  = LUX_LIGHT_OFF;
} gTh;

/* ================== KHAI BÁO HÀM ================== */
static void mqttConnect();
static void publishRelayStatus();
static void publishModeStatus();
static void publishThresholdsStatus();
static void runAutoControlLogic();
static void handleMqttCallback(char* topic, byte* payload, unsigned int length);
static void setRelayState(int relayIndex, bool state);

/* ---- NVS: load/save ---- */
static void loadPersistedSettings() {
  // namespace "aiot" ở chế độ read-only
  if (!prefs.begin("aiot", true)) return;

  // đọc về, nếu chưa có thì giữ mặc định hiện tại
  gTh.tempOn  = prefs.getFloat("t_on",  gTh.tempOn);
  gTh.tempOff = prefs.getFloat("t_off", gTh.tempOff);
  gTh.soilOn  = prefs.getFloat("s_on",  gTh.soilOn);
  gTh.soilOff = prefs.getFloat("s_off", gTh.soilOff);
  gTh.luxOn   = prefs.getFloat("l_on",  gTh.luxOn);
  gTh.luxOff  = prefs.getFloat("l_off", gTh.luxOff);

  // Chế độ Auto/Manual cũng lưu
  gState.autoMode = prefs.getBool("auto", gState.autoMode);

  prefs.end();
}

static void saveThresholdsToNVS() {
  if (!prefs.begin("aiot", false)) return;
  prefs.putFloat("t_on",  gTh.tempOn);
  prefs.putFloat("t_off", gTh.tempOff);
  prefs.putFloat("s_on",  gTh.soilOn);
  prefs.putFloat("s_off", gTh.soilOff);
  prefs.putFloat("l_on",  gTh.luxOn);
  prefs.putFloat("l_off", gTh.luxOff);
  prefs.end();
}

static void saveModeToNVS() {
  if (!prefs.begin("aiot", false)) return;
  prefs.putBool("auto", gState.autoMode);
  prefs.end();
}

/* ================== HELPERS ================== */
static void setRelayState(int relayIndex, bool state) {
  uint8_t pin = 0;
  bool*   cur = nullptr;

  switch (relayIndex) {
    case 1: pin = PIN_RELAY1; cur = &gState.relay1; break;
    case 2: pin = PIN_RELAY2; cur = &gState.relay2; break;
    case 3: pin = PIN_RELAY3; cur = &gState.relay3; break;
    case 4: pin = PIN_RELAY4; cur = &gState.relay4; break;
    default: return;
  }
  if (*cur == state) return;

  *cur = state;
  digitalWrite(pin, state ? RELAY_ACTIVE_LEVEL : !RELAY_ACTIVE_LEVEL);
  Serial.printf("[CONTROL] Relay %d -> %s\n", relayIndex, state ? "BẬT" : "TẮT");
  publishRelayStatus();
}

static void mqttConnect() {
  mqtt.setBufferSize(512);
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(handleMqttCallback);

  Serial.println("[MQTT] Kết nối broker...");
  while (!mqtt.connected()) {
    if (mqtt.connect(MQTT_CLIENTID, MQTT_USER, MQTT_PASS)) {
      Serial.println("[MQTT] OK");
      mqtt.subscribe(MQTT_TOPIC_RELAY_COMMAND);
      mqtt.subscribe(MQTT_TOPIC_MODE_COMMAND);
      mqtt.subscribe(MQTT_TOPIC_THRESHOLDS_SET);

      // Publish retained để UI đồng bộ ngay
      publishModeStatus();
      publishRelayStatus();
      publishThresholdsStatus();

      if (gState.autoMode) runAutoControlLogic();
    } else {
      Serial.printf("[MQTT] Fail rc=%d, đợi 5s...\n", mqtt.state());
      delay(5000);
    }
  }
}

static void publishRelayStatus() {
  if (!mqtt.connected()) return;
  JsonDocument doc;
  doc["relay1"] = gState.relay1;
  doc["relay2"] = gState.relay2;
  doc["relay3"] = gState.relay3;
  doc["relay4"] = gState.relay4;
  char buf[128];
  serializeJson(doc, buf);
  mqtt.publish(MQTT_TOPIC_RELAY_STATUS, buf, true); // retained
}

static void publishModeStatus() {
  if (!mqtt.connected()) return;
  JsonDocument doc;
  doc["auto_mode"] = gState.autoMode;
  char buf[64];
  serializeJson(doc, buf);
  mqtt.publish(MQTT_TOPIC_MODE_STATUS, buf, true); // retained
}

static void publishThresholdsStatus() {
  if (!mqtt.connected()) return;
  JsonDocument d;
  d["temp_on"]  = gTh.tempOn;
  d["temp_off"] = gTh.tempOff;
  d["soil_on"]  = gTh.soilOn;
  d["soil_off"] = gTh.soilOff;
  d["lux_on"]   = gTh.luxOn;
  d["lux_off"]  = gTh.luxOff;
  char buf[192];
  serializeJson(d, buf);
  mqtt.publish(MQTT_TOPIC_THRESHOLDS_STATUS, buf, true); // retained
}

/* ================== AUTO LOGIC ================== */
static void runAutoControlLogic() {
  if (!gState.autoMode) return;

  // Quạt theo nhiệt độ (hysteresis)
  if (gState.temperature >= gTh.tempOn)       setRelayState(1, true);
  else if (gState.temperature <= gTh.tempOff) setRelayState(1, false);

  // Bơm theo % ẩm đất (hysteresis)
  if (gState.soilPercent <= gTh.soilOn)       setRelayState(2, true);
  else if (gState.soilPercent >= gTh.soilOff) setRelayState(2, false);

  // Đèn theo lux (hysteresis)
  if (gState.lux <= gTh.luxOn)                setRelayState(3, true);
  else if (gState.lux >= gTh.luxOff)          setRelayState(3, false);
}

/* ================== MQTT CALLBACK ================== */
static void handleMqttCallback(char* topic, byte* payload, unsigned int length) {
  JsonDocument doc;
  if (deserializeJson(doc, payload, length)) return;

  // 1) Lệnh relay tay
  if (strcmp(topic, MQTT_TOPIC_RELAY_COMMAND) == 0) {
    if (gState.autoMode) return; // Auto bỏ qua lệnh tay
    if (doc["relay"].is<int>() && doc["state"].is<bool>()) {
      setRelayState(doc["relay"].as<int>(), doc["state"].as<bool>());
    }
    return;
  }

  // 2) Lệnh đổi mode
  if (strcmp(topic, MQTT_TOPIC_MODE_COMMAND) == 0) {
    if (!doc["auto_mode"].is<bool>()) return;
    bool newAuto = doc["auto_mode"].as<bool>();
    if (gState.autoMode == newAuto) return;

    bool wasAuto = gState.autoMode;
    gState.autoMode = newAuto;
    saveModeToNVS();               // <-- LƯU CHẾ ĐỘ

    if (!wasAuto && newAuto) {
      // Manual -> Auto: đánh giá ngưỡng ngay
      runAutoControlLogic();
    }
    // Auto -> Manual: giữ nguyên trạng thái hiện tại
    Serial.printf("[CONTROL] Mode: %s\n", gState.autoMode ? "AUTO" : "MANUAL");
    publishModeStatus();
    return;
  }

  // 3) Cập nhật ngưỡng động (có thể gửi 1 key hoặc nhiều key)
  if (strcmp(topic, MQTT_TOPIC_THRESHOLDS_SET) == 0) {
    bool changed = false;

    // Đánh dấu key nào có mặt (để swap theo cặp, tránh "nhảy số")
    bool hasTempOn  = doc["temp_on"].is<float>()  || doc["temp_on"].is<int>();
    bool hasTempOff = doc["temp_off"].is<float>() || doc["temp_off"].is<int>();
    bool hasSoilOn  = doc["soil_on"].is<float>()  || doc["soil_on"].is<int>();
    bool hasSoilOff = doc["soil_off"].is<float>() || doc["soil_off"].is<int>();
    bool hasLuxOn   = doc["lux_on"].is<float>()   || doc["lux_on"].is<int>();
    bool hasLuxOff  = doc["lux_off"].is<float>()  || doc["lux_off"].is<int>();

    auto setf = [&](const char* key, float &dst) {
      if (doc[key].is<float>() || doc[key].is<int>()) {
        float v = doc[key].as<float>();
        if (dst != v) { dst = v; changed = true; }
      }
    };

    setf("temp_on",  gTh.tempOn);
    setf("temp_off", gTh.tempOff);
    setf("soil_on",  gTh.soilOn);
    setf("soil_off", gTh.soilOff);
    setf("lux_on",   gTh.luxOn);
    setf("lux_off",  gTh.luxOff);

    // Swap chỉ khi nhận CÙNG LÚC cả 2 vế
    if (hasTempOn && hasTempOff && gTh.tempOn < gTh.tempOff) {
      float t = gTh.tempOn; gTh.tempOn = gTh.tempOff; gTh.tempOff = t;
    }
    if (hasSoilOn && hasSoilOff && gTh.soilOn > gTh.soilOff) {
      float t = gTh.soilOn; gTh.soilOn = gTh.soilOff; gTh.soilOff = t;
    }
    if (hasLuxOn && hasLuxOff && gTh.luxOn > gTh.luxOff) {
      float t = gTh.luxOn; gTh.luxOn = gTh.luxOff; gTh.luxOff = t;
    }

    if (changed) {
      saveThresholdsToNVS();       // <-- LƯU NGƯỠNG
      publishThresholdsStatus();   // báo UI
      if (gState.autoMode) runAutoControlLogic(); // áp dụng ngay khi đang Auto
      Serial.println("[CONTROL] Thresholds updated & saved.");
    }
    return;
  }
}

/* ================== TASK MQTT ================== */
void TaskMQTT(void* pvParameters) {
  (void)pvParameters;

  const uint32_t PUB_MS = 2000;
  uint32_t lastPub = 0;

  gState.autoMode = true;      // mặc định
  loadPersistedSettings();     // <-- đọc lại NVS (ghi đè mặc định nếu có)

  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  mqttConnect();

  // Publish lại để Dashboard đồng bộ ngay cả sau mất điện
  publishThresholdsStatus();
  publishModeStatus();
  if (gState.autoMode) runAutoControlLogic();

  for (;;) {
    if (WiFi.status() != WL_CONNECTED) {
      vTaskDelay(pdMS_TO_TICKS(1000));
      continue;
    }
    if (!mqtt.connected()) {
      mqttConnect();
    }

    mqtt.loop();
    runAutoControlLogic();

    if (millis() - lastPub >= PUB_MS) {
      lastPub = millis();
      JsonDocument t;
      t["temperature"]     = (float)gState.temperature;
      t["humidity"]        = (float)gState.humidity;
      t["soil_moisture"]   = (float)gState.soilPercent;
      t["light_intensity"] = (float)gState.lux;
      char buf[256];
      serializeJson(t, buf);
      mqtt.publish(MQTT_TOPIC_TELEMETRY, buf);
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
