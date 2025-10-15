#include <Arduino.h>
#include <Wire.h>
#include <DHT.h>
#include <BH1750.h>
#include <app_config.h>
#include <sensors.h>

void hwInitSensors() {
  dht.begin();
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);

  // Tuỳ chọn ADC soil:
  analogSetPinAttenuation(PIN_SOIL_AO, ADC_11db);
  analogReadResolution(12);
}

void hwInitRelays() {
  pinMode(PIN_RELAY1, OUTPUT);
  pinMode(PIN_RELAY2, OUTPUT);
  pinMode(PIN_RELAY3, OUTPUT);
  pinMode(PIN_RELAY4, OUTPUT);

  // Kích mức CAO = bật → set LOW để tắt ban đầu
  digitalWrite(PIN_RELAY1, LOW);
  digitalWrite(PIN_RELAY2, LOW);
  digitalWrite(PIN_RELAY3, LOW);
  digitalWrite(PIN_RELAY4, LOW);
}
