/**
 * @file mqtt_task.cpp
 * @brief Tệp mã nguồn được cấu trúc lại cho việc xử lý MQTT trên ESP32.
 *
 * - Tách biệt các chủ đề MQTT để quản lý dữ liệu tốt hơn.
 * - Thêm callback để nhận lệnh điều khiển từ Node-RED.
 * - Sử dụng ArduinoJson để phân tích và tạo chuỗi JSON một cách an toàn.
 * - Cải thiện cấu trúc để dễ đọc và bảo trì.
 * - **Mới:** Thêm chế độ Tự Động/Thủ Công và hàm điều khiển relay vật lý.
 *
 * @note Bạn cần cài đặt thư viện ArduinoJson.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "app_state.h"     // File này phải chứa `bool autoMode` trong struct gState
#include "mqtt_config.h"   // Chứa thông tin cấu hình MQTT
#include "app_config.h" // Chứa các ngưỡng cho chế độ tự động

// Khởi tạo các đối tượng WiFi và MQTT client
static WiFiClient net;
static PubSubClient mqtt(net);

// Khai báo trước các hàm sẽ sử dụng
void publishRelayStatus();
void publishModeStatus();
void handleMqttCallback(char* topic, byte* payload, unsigned int length);
void runAutoControlLogic();

/**
 * @brief Hàm trung tâm để điều khiển relay.
 * Cập nhật trạng thái trong phần mềm và điều khiển cả phần cứng.
 * @param relayIndex Số thứ tự relay (1-4).
 * @param state Trạng thái mong muốn (true = BẬT, false = TẮT).
 */
void setRelayState(int relayIndex, bool state) {
    bool stateChanged = false;
    switch (relayIndex) {
        case 1: if (gState.relay1 != state) { gState.relay1 = state; stateChanged = true; } break;
        case 2: if (gState.relay2 != state) { gState.relay2 = state; stateChanged = true; } break;
        case 3: if (gState.relay3 != state) { gState.relay3 = state; stateChanged = true; } break;
        case 4: if (gState.relay4 != state) { gState.relay4 = state; stateChanged = true; } break;
        default: return; // Thoát nếu relay không hợp lệ
    }

    if (stateChanged) {
        Serial.printf("[CONTROL] Relay %d -> %s\n", relayIndex, state ? "BẬT" : "TẮT");
        
        // ===================================================================
        // @todo: THÊM CODE ĐIỀU KHIỂN PHẦN CỨNG (GPIO) CỦA BẠN VÀO ĐÂY
        // Ví dụ:
        // switch (relayIndex) {
        //     case 1: digitalWrite(RELAY_PIN_1, state ? HIGH : LOW); break;
        //     case 2: digitalWrite(RELAY_PIN_2, state ? HIGH : LOW); break;
        //     ...
        // }
        // ===================================================================

        publishRelayStatus(); // Gửi trạng thái mới nhất lên Node-RED
    }
}


/**
 * @brief Kết nối hoặc kết nối lại tới MQTT broker.
 */
static void mqttConnect() {
    mqtt.setServer(MQTT_BROKER, MQTT_PORT);
    mqtt.setCallback(handleMqttCallback); 

    Serial.println("[MQTT] Đang kết nối đến broker...");
    while (!mqtt.connected()) {
        if (mqtt.connect(MQTT_CLIENTID, MQTT_USER, MQTT_PASS)) {
            Serial.println("[MQTT] Đã kết nối thành công!");
            // Đăng ký nhận lệnh điều khiển relay
            mqtt.subscribe(MQTT_TOPIC_RELAY_COMMAND);
            // Đăng ký nhận lệnh chuyển đổi chế độ
            mqtt.subscribe(MQTT_TOPIC_MODE_COMMAND);

            // Gửi ngay trạng thái ban đầu khi vừa kết nối
            publishModeStatus();
            publishRelayStatus();
        } else {
            Serial.print("[MQTT] Kết nối thất bại, rc=");
            Serial.print(mqtt.state());
            Serial.println(". Thử lại sau 500ms.");
            delay(500);
        }
    }
}

/**
 * @brief Gửi trạng thái hiện tại của tất cả các relay lên MQTT broker.
 */
void publishRelayStatus() {
    if (!mqtt.connected()) return;
    JsonDocument doc;
    doc["relay1"] = gState.relay1;
    doc["relay2"] = gState.relay2;
    doc["relay3"] = gState.relay3;
    doc["relay4"] = gState.relay4;
    char buffer[128];
    serializeJson(doc, buffer);
    mqtt.publish(MQTT_TOPIC_RELAY_STATUS, buffer, true); // Retained
}

/**
 * @brief Gửi trạng thái chế độ (Tự động/Thủ công) lên MQTT broker.
 */
void publishModeStatus() {
    if (!mqtt.connected()) return;
    JsonDocument doc;
    doc["auto_mode"] = gState.autoMode;
    char buffer[64];
    serializeJson(doc, buffer);
    mqtt.publish(MQTT_TOPIC_MODE_STATUS, buffer, true); // Retained
}

/**
 * @brief Xử lý các tin nhắn MQTT nhận được.
 */
void handleMqttCallback(char* topic, byte* payload, unsigned int length) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload, length);
    if (error) {
        Serial.print(F("[MQTT] Lỗi phân tích JSON: "));
        Serial.println(error.c_str());
        return;
    }

    // --- Xử lý lệnh điều khiển relay ---
    if (strcmp(topic, MQTT_TOPIC_RELAY_COMMAND) == 0) {
        if (gState.autoMode) {
            Serial.println("[MQTT] Đang ở chế độ Tự Động, bỏ qua lệnh điều khiển thủ công.");
            return;
        }
        if (doc.containsKey("relay") && doc.containsKey("state")) {
            int relayIndex = doc["relay"];
            bool relayState = doc["state"];
            setRelayState(relayIndex, relayState);
        }
    }
    // --- Xử lý lệnh chuyển chế độ ---
    else if (strcmp(topic, MQTT_TOPIC_MODE_COMMAND) == 0) {
        if (doc.containsKey("auto_mode")) {
            gState.autoMode = doc["auto_mode"];
            Serial.printf("[CONTROL] Đã chuyển sang chế độ: %s\n", gState.autoMode ? "TỰ ĐỘNG" : "THỦ CÔNG");
            publishModeStatus(); // Gửi lại trạng thái chế độ để xác nhận
        }
    }
}

/**
 * @brief Logic điều khiển tự động dựa trên ngưỡng cảm biến.
 */
void runAutoControlLogic() {
    if (!gState.autoMode) return; // Chỉ chạy khi ở chế độ tự động

    // Logic cho Relay 1 (ví dụ: Quạt làm mát)
    if (gState.temperature > TEMP_THRESHOLD_HIGH) {
        setRelayState(1, true); // Bật quạt nếu quá nóng
    } else if (gState.temperature < TEMP_THRESHOLD_LOW) {
        setRelayState(1, false); // Tắt quạt nếu đã đủ mát
    }

    // Logic cho Relay 2 (ví dụ: Máy bơm)
    if (gState.soilPercent < SOIL_MOISTURE_THRESHOLD_LOW) {
        setRelayState(2, true); // Bật bơm nếu đất khô
    } else if (gState.soilPercent > SOIL_MOISTURE_THRESHOLD_HIGH) {
        setRelayState(2, false); // Tắt bơm nếu đất đủ ẩm
    }

    // Logic cho Relay 3 (ví dụ: Đèn)
    if (gState.lux < LIGHT_THRESHOLD_LOW) {
        setRelayState(3, true); // Bật đèn nếu trời tối
    } else if (gState.lux > LIGHT_THRESHOLD_HIGH) {
        setRelayState(3, false); // Tắt đèn nếu trời sáng
    }

    // Logic cho Relay 4 (ví dụ: Phun sương, dựa vào độ ẩm không khí)
    // ... Thêm logic của bạn ở đây ...
}


/**
 * @brief Task FreeRTOS chính cho việc xử lý MQTT.
 */
void TaskMQTT(void* pvParameters) {
    (void)pvParameters;
    const uint32_t PUBLISH_INTERVAL_MS = 1000;
    uint32_t lastPublishTime = 0;

    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    mqttConnect();

    for (;;) {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[MQTT] Mất kết nối WiFi. Đang thử kết nối lại...");
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }
        if (!mqtt.connected()) {
            mqttConnect();
        }

        mqtt.loop(); // Xử lý tin nhắn đến và keep-alive

        // Chạy logic điều khiển tự động
        runAutoControlLogic();

        // Gửi dữ liệu cảm biến định kỳ
        uint32_t currentTime = millis();
        if (currentTime - lastPublishTime >= PUBLISH_INTERVAL_MS) {
            lastPublishTime = currentTime;
            JsonDocument telemetryDoc;
            telemetryDoc["temperature"] = gState.temperature;
            telemetryDoc["humidity"] = gState.humidity;
            telemetryDoc["soil_moisture"] = gState.soilPercent;
            telemetryDoc["light_intensity"] = gState.lux;
            char payload[256];
            serializeJson(telemetryDoc, payload);
            mqtt.publish(MQTT_TOPIC_TELEMETRY, payload);
        }

        vTaskDelay(pdMS_TO_TICKS(100)); // Nhường CPU
    }
}

