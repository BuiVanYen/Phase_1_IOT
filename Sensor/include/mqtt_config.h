#pragma once                         //tránh include trùng nhiều lần file header

// mqtt_config.h
#define MQTT_BROKER   "816f8ccfbe1f457d88e0d150fd568a73.s1.eu.hivemq.cloud"   // host Cloud
#define MQTT_PORT     8883                                // TLS
#define MQTT_USER     "admin"
#define MQTT_PASS     "Admin123"
//topic mqtt
#define MQTT_TOPIC_STATUS               "greenhouse/esp32/status"
#define MQTT_TOPIC_TELEMETRY            "greenhouse/esp32/telemetry"
#define MQTT_TOPIC_RELAY_COMMAND        "greenhouse/esp32/relay/command"
#define MQTT_TOPIC_RELAY_STATUS         "greenhouse/esp32/relay/status"
#define MQTT_TOPIC_MODE_COMMAND         "greenhouse/esp32/mode/command"
#define MQTT_TOPIC_MODE_STATUS          "greenhouse/esp32/mode/status"
#define MQTT_TOPIC_THRESHOLDS_SET       "greenhouse/esp32/thresholds/set"
#define MQTT_TOPIC_THRESHOLDS_STATUS    "greenhouse/esp32/thresholds/status"

#define RELAY_ACTIVE_LEVEL HIGH
#define MQTT_CLIENTID "esp32-greenhouse-01"

