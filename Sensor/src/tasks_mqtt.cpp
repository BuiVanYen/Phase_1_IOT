#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "app_config.h"
#include "app_state.h"
#include "mqtt_config.h"

// Khởi tạo các đối tượng WiFi và MQTT client
static WiFiClient net;
static PubSubClient mqtt(net);

// Khai báo trước các hàm sẽ sử dụng
void publishRelayStatus();
void publishModeStatus();
void handleMqttCallback(char* topic, byte* payload, unsigned int length);
void runAutoControlLogic();
static void restoreManualSnapshot();
static void snapshotManualFromCurrent();
void setRelayState(int relayIndex, bool state);

static bool sLastManual[4] = {false, false, false, false};

static void snapshotManualFromCurrent() {
  sLastManual[0] = gState.relay1;
  sLastManual[1] = gState.relay2;
  sLastManual[2] = gState.relay3;
  sLastManual[3] = gState.relay4;
}

static void restoreManualSnapshot() {
  setRelayState(1, sLastManual[0]);
  setRelayState(2, sLastManual[1]);
  setRelayState(3, sLastManual[2]);
  setRelayState(4, sLastManual[3]);
}

void setRelayState(int relayIndex, bool state) {
  uint8_t pin;
  bool* currentState;

  switch (relayIndex) {
    case 1: pin = PIN_RELAY1; currentState = &gState.relay1; break;
    case 2: pin = PIN_RELAY2; currentState = &gState.relay2; break;
    case 3: pin = PIN_RELAY3; currentState = &gState.relay3; break;
    case 4: pin = PIN_RELAY4; currentState = &gState.relay4; break;
    default: return;
  }

  if (*currentState != state) {
    *currentState = state;
    digitalWrite(pin, state ? RELAY_ACTIVE_LEVEL : !RELAY_ACTIVE_LEVEL);
    Serial.printf("[CONTROL] Relay %d -> %s\n", relayIndex, state ? "BẬT" : "TẮT");
    publishRelayStatus();
  }
}

static void mqttConnect() {
  mqtt.setBufferSize(512);
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(handleMqttCallback);

  Serial.println("[MQTT] Đang kết nối đến broker...");
  while (!mqtt.connected()) {
    if (mqtt.connect(MQTT_CLIENTID, MQTT_USER, MQTT_PASS)) {
      Serial.println("[MQTT] Đã kết nối thành công!");
      mqtt.subscribe(MQTT_TOPIC_RELAY_COMMAND);
      mqtt.subscribe(MQTT_TOPIC_MODE_COMMAND);

      // Publish trạng thái ngay khi vừa kết nối
      publishModeStatus();
      publishRelayStatus();
    } else {
      Serial.printf("[MQTT] Kết nối thất bại, rc=%d. Thử lại sau 5s.\n", mqtt.state());
      delay(5000);
    }
  }
}

void publishRelayStatus() {
  if (!mqtt.connected()) return;
  JsonDocument doc;
  doc["relay1"] = gState.relay1;
  doc["relay2"] = gState.relay2;
  doc["relay3"] = gState.relay3;
  doc["relay4"] = gState.relay4;
  char buffer[128];
  serializeJson(doc, buffer);
  mqtt.publish(MQTT_TOPIC_RELAY_STATUS, buffer, true); // retained
}

void publishModeStatus() {
  if (!mqtt.connected()) return;
  JsonDocument doc;
  doc["auto_mode"] = gState.autoMode;
  char buffer[64];
  serializeJson(doc, buffer);
  mqtt.publish(MQTT_TOPIC_MODE_STATUS, buffer, true); // retained
}

void handleMqttCallback(char* topic, byte* payload, unsigned int length) {
  JsonDocument doc;
  if (deserializeJson(doc, payload, length)) return;

  if (strcmp(topic, MQTT_TOPIC_RELAY_COMMAND) == 0) {
    // AUTO thì bỏ qua lệnh tay
    if (gState.autoMode) return;
    if (doc.containsKey("relay") && doc.containsKey("state")) {
      int idx = doc["relay"];
      bool st = doc["state"];
      setRelayState(idx, st);
      if (idx >= 1 && idx <= 4) sLastManual[idx - 1] = st; // cập nhật snapshot tay
    }
  } else if (strcmp(topic, MQTT_TOPIC_MODE_COMMAND) == 0) {
    // {"auto_mode": true/false}
    if (doc.containsKey("auto_mode")) {
      bool newMode = doc["auto_mode"];
      if (gState.autoMode != newMode) {
        bool wasAuto = gState.autoMode;
        gState.autoMode = newMode;

        if (!wasAuto && newMode) {
          // MANUAL -> AUTO: chụp snapshot tay hiện tại
          snapshotManualFromCurrent();
        } else if (wasAuto && !newMode) {
          // AUTO -> MANUAL: khôi phục lại trạng thái tay trước đó
          restoreManualSnapshot();
        }

        Serial.printf("[CONTROL] Chế độ: %s\n", gState.autoMode ? "TỰ ĐỘNG" : "THỦ CÔNG");
        publishModeStatus();
      }
    }
  }
}

void runAutoControlLogic() {
  if (!gState.autoMode) return;

  // Fan theo nhiệt độ
  if (gState.temperature >= TEMP_FAN_ON_C) setRelayState(1, true);
  else if (gState.temperature <= TEMP_FAN_OFF_C) setRelayState(1, false);

  // Pump theo % ẩm đất
  if (gState.soilPercent <= SOIL_PCT_PUMP_ON) setRelayState(2, true);
  else if (gState.soilPercent >= SOIL_PCT_PUMP_OFF) setRelayState(2, false);

  // Đèn theo lux
  if (gState.lux <= LUX_LIGHT_ON) setRelayState(3, true);
  else if (gState.lux >= LUX_LIGHT_OFF) setRelayState(3, false);
}

void TaskMQTT(void* pvParameters) {
  (void)pvParameters;
  const uint32_t PUBLISH_INTERVAL_MS = 2000;
  uint32_t lastPublishTime = 0;

  // Mặc định AUTO khi khởi động
  gState.autoMode = true;

  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  mqttConnect();

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

    if (millis() - lastPublishTime >= PUBLISH_INTERVAL_MS) {
      lastPublishTime = millis();
      JsonDocument telemetryDoc;
      telemetryDoc["temperature"]    = (float)gState.temperature;
      telemetryDoc["humidity"]       = (float)gState.humidity;
      telemetryDoc["soil_moisture"]  = (float)gState.soilPercent;
      telemetryDoc["light_intensity"]= (float)gState.lux;
      char payload[256];
      serializeJson(telemetryDoc, payload);
      mqtt.publish(MQTT_TOPIC_TELEMETRY, payload);
      Serial.printf("[MQTT] Telemetry sent: %s\n", payload);
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
