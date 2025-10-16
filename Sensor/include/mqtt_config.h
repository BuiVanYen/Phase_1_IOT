#pragma once                         //tránh include trùng nhiều lần file header

#define MQTT_BROKER   "192.168.2.4"//IP máy chạy Mosquitto (lệnh ipconfig)
#define MQTT_PORT     1883           //cổng MQTT chuẩn
#define MQTT_CLIENTID "esp32-greenhouse-01" //tên client duy nhất (đừng trùng giữa các ESP)
//#define MQTT_TOPIC    "esp32/greenhouse/data" //topic publish dữ liệu cảm biến

// (Có dùng user/pass trong broker → khai báo ở đây)
#define MQTT_USER     "iot"      // username đã tạo bằng mosquitto_passwd
#define MQTT_PASS     "admin"     //password tương ứng
// --- Định nghĩa các chủ đề (Topics) ---
#define RELAY_ACTIVE_LEVEL HIGH
// 1. Chủ đề ESP32 GỬI ĐI
#define MQTT_TOPIC_TELEMETRY "greenhouse/esp32/telemetry"
#define MQTT_TOPIC_RELAY_STATUS "greenhouse/esp32/relay/status"
#define MQTT_TOPIC_MODE_STATUS "greenhouse/esp32/mode/status"

// 2. Chủ đề ESP32 NHẬN VỀ
#define MQTT_TOPIC_RELAY_COMMAND "greenhouse/esp32/relay/command"
#define MQTT_TOPIC_MODE_COMMAND "greenhouse/esp32/mode/command"

