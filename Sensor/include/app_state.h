#ifndef APP_STATE_H
#define APP_STATE_H
#include <Arduino.h>

struct AppState {
  float temperature = 0.0f;
  float humidity    = 0.0f;
  float soilPercent = 0.0f;
  float lux         = 0.0f;
  int   soilADC     = 0;

  bool relay1 = false; // quạt
  bool relay2 = false; // bơm
  bool relay3 = false; // đèn
  bool relay4 = false; // dự phòng

  bool autoMode = true;    // <-- MẶC ĐỊNH AUTO = true
};

extern AppState gState;

#endif
