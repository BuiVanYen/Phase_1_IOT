#include <sensors.h>

// Tạo đối tượng thật sự (1 bản duy nhất trong toàn chương trình)
DHT dht(PIN_DHT, DHT_TYPE);
BH1750 lightMeter;
