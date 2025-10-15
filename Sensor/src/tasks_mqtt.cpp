#include <Arduino.h>                 
#include <WiFi.h>                    
#include <PubSubClient.h>            
#include <app_state.h>               
#include <mqtt_config.h>

static WiFiClient net;               
static PubSubClient mqtt(net);       

static void mqttConnect() {          
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);        
  while (!mqtt.connected()) {                    
    bool ok = mqtt.connect(MQTT_CLIENTID, MQTT_USER, MQTT_PASS); 
    // Nếu broker cho anonymous có thể dùng: bool ok = mqtt.connect(MQTT_CLIENTID);
    if (!ok) delay(500);                          
  }
}
/* ===== publish trạng thái relay ===== */
static void publishRelayStatus() {
  char buf[80];
  snprintf(buf, sizeof(buf), "{\"r1\":%d,\"r2\":%d,\"r3\":%d,\"r4\":%d}",
           gState.relay1?1:0, gState.relay2?1:0, gState.relay3?1:0, gState.relay4?1:0);
  mqtt.publish("esp32/greenhouse/status", buf, true); // retain để UI vào là thấy ngay
}
void TaskMQTT(void *pvParameters) { 
  (void) pvParameters;              
  const uint32_t PUB_MS = 2000;     //chu kỳ publish (2 giây)
  uint32_t last = 0;                //mốc thời gian lần publish trước

  mqttConnect();                    //kết nối lần đầu

  for (;;) {                       
    if (WiFi.status() != WL_CONNECTED) WiFi.reconnect(); 
    if (!mqtt.connected()) mqttConnect();                
    mqtt.loop();                                         // xử lý keep-alive/ping

    uint32_t now = millis();                             // thời gian hiện tại
    if (now - last >= PUB_MS && mqtt.connected()) {      // đủ chu kỳ và đang kết nối
      last = now;                                        // cập nhật mốc thời gian

      //dựng chuỗi JSON; dùng snprintf để tránh cấp phát động
      char payload[256];
      snprintf(payload, sizeof(payload),
        "{\"ts\":%lu,\"temp\":%.2f,\"hum\":%.2f,"
        "\"soil_pct\":%.1f,\"soil_adc\":%d,\"lux\":%.2f}",
        (unsigned long) now,            // timestamp (ms từ khi boot)
        gState.temperature,             // nhiệt độ °C
        gState.humidity,                // độ ẩm không khí %
        gState.soilPercent,             // ẩm đất %
        gState.soilADC,                 // ADC đất (debug)
        gState.lux                      // ánh sáng lux
      );

      mqtt.publish(MQTT_TOPIC, payload); // gửi lên broker (QoS 0)
      Serial.print("[MQTT] "); Serial.println(payload); //log ra Serial
    }

    vTaskDelay(pdMS_TO_TICKS(50));    //nhường CPU 50ms
  }
}