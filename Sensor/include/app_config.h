#ifndef APP_CONFIG_H
#define APP_CONFIG_H

// ===== PIN MAP =====
#define PIN_DHT       14
#define DHT_TYPE      DHT22
#define PIN_SOIL_AO   34
#define PIN_I2C_SDA   21
#define PIN_I2C_SCL   22
#define PIN_RELAY1    25
#define PIN_RELAY2    26
#define PIN_RELAY3    27
#define PIN_RELAY4    33

// ===== SOIL CALIBRATION (ADC 0..4095) =====
#define SOIL_ADC_DRY  3700
#define SOIL_ADC_WET  1500

// ===== THRESHOLDS =====
#define TEMP_FAN_ON_C        32.0f
#define TEMP_FAN_OFF_C       28.0f
#define LUX_LIGHT_ON         100.0f
#define LUX_LIGHT_OFF        300.0f
#define SOIL_PCT_PUMP_ON     35.0f
#define SOIL_PCT_PUMP_OFF    45.0f

// ===== WIFI PORTAL (WiFiManager) =====
#define WIFI_CONNECT_TIMEOUT_S   15
#define WIFI_PORTAL_TIMEOUT_S    180
#define WIFI_PORTAL_SSID         "ESP32-Setup"
#define WIFI_PORTAL_PASS         "12345678"   

#endif
