/****************************************************************************************************************************
  Async_ConfigOnDoubleReset_minimal.ino
  For ESP8266 / ESP32 boards
  Built by Khoi Hoang https://github.com/khoih-prog/ESP_WiFiManager
  Licensed under MIT license
 *****************************************************************************************************************************/
#if defined(ESP32)
  #define USE_SPIFFS            true
  #define ESP_DRD_USE_EEPROM    true
#else
  #error This code is intended to run on the ESP32 platform! Please check your Tools->Board setting.
#endif
#define DRD_TIMEOUT             1
#define DRD_ADDRESS             0
#include <ESP_DoubleResetDetector.h>            //https://github.com/khoih-prog/ESP_DoubleResetDetector
DoubleResetDetector* drd;

void setup()
{
  Serial.begin(115200);
  drd = new DoubleResetDetector(DRD_TIMEOUT, DRD_ADDRESS);
  if (drd->detectDoubleReset())
  {
    Serial.println(F("DRD"));
  }
}
void loop()
{
  drd->loop();
}
