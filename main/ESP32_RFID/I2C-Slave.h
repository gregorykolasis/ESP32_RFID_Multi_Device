#include <Wire.h>
#include <ArduinoJson.h>

#define I2C_BASE_ADDR 0x50 //addresses above 0x7F (0x80 to 0xFF) are invalid because they exceed the 7-bit range.            
uint8_t I2C_ADDR = I2C_BASE_ADDR + 0 ;

#define I2C_MAX_BYTES_RX_TX 161
#define I2C_JSON_BUFFER 512
#define I2C_MINIMUM_LENGTH_MESSAGE 4
#define I2C_MAX_LEN_ALLOWED_JSON I2C_MAX_BYTES_RX_TX-1

char i2cBuffer[I2C_JSON_BUFFER/2]; // Buffer for receiving I2C data
uint16_t i2cIndex = 0;
StaticJsonDocument<I2C_JSON_BUFFER> i2cJson;
bool I2C_ERROR = false;
uint32_t I2C_LIFE_ERRORS = 0;

void dummmyI2C() {

  i2cJson["rfid"]        = "12345678901234567890";  // Example RFID tag number
  i2cJson["status"]      = "success";   // Status of the scan
  i2cJson["result"]      = "ohmygodlookatthatbutt";   // Status of the scan
  i2cJson["field2"]      = "lolenmyfriend";   // Status of the scan
  i2cJson["attenuation"] = "attenuatorr";   // Status of the scan
  i2cJson["boleklolen"]  = "this1234";   // Status of the scan  
  
}

void onErrorI2C(const char* msg, int address) {
  I2C_ERROR = true;
  I2C_LIFE_ERRORS++;
  Serial.printf("[onErrorI2C] Address:%d Message:%s \n",address,msg);
}

void receiveEventI2C(int howMany) {
	
  i2cIndex = 0;
  memset(i2cBuffer,0,sizeof(i2cBuffer));
  i2cJson.clear();

  while (Wire.available() && i2cIndex < sizeof(i2cBuffer) - 1) {
    i2cBuffer[i2cIndex++] = Wire.read();
  } 
  i2cBuffer[i2cIndex] = '\0'; 

  // Parse the incoming JSON
  DeserializationError error = deserializeJson(i2cJson, i2cBuffer);
  if (!error) {
    const char* mode = i2cJson["mode"];
    int interval     = i2cJson["interval"];
    Serial.print("Received mode: ");
    Serial.println(mode);
    Serial.print("Received interval: ");
    Serial.println(interval);
  } else {
    Serial.println("Error: Failed to parse configuration.");
  }
  
}

void requestEventi2C() {

  //dummmyI2C();
  
  i2cJson.clear();
  prepareJsonI2C();

  // Measure the size of the serialized JSON
  size_t jsonSize = measureJson(i2cJson);
  
  Serial.print("Size of JSON to send: ");
  Serial.println(jsonSize);

  // Check if the JSON size exceeds the I2C buffer (248 bytes)
  if (jsonSize > I2C_MAX_LEN_ALLOWED_JSON) {
    Serial.println("Error: JSON size exceeds I2C buffer limit!");
    return;  // Stop if too large
  }

  serializeJson(i2cJson, Serial);
  Serial.println();
  serializeJson(i2cJson, Wire);

  // Send the RFID JSON data to the Master
  //char buf[248];
  //serializeJson(i2cJson, buf);  // Serialize JSON into the buffer
  //for (uint8_t i = 0; i < strlen(buf); i++) {
  //  Wire.write(buf[i]);  // Send byte by byte over I2C
  //}

  
}

void readAddrI2C() {

  I2C_ADDR = 0;
  for (uint8_t i = 0; i < sizeof(I2C_ADDR_PINS); i++) {
    uint8_t bit_value = digitalRead(I2C_ADDR_PINS[i]);  // Read each pin state
    I2C_ADDR |= (bit_value << i);  // Shift the bit according to its position (LSB first)
  }
  I2C_ADDR = I2C_BASE_ADDR + I2C_ADDR;

  Serial.print("[I2C-Addr] 0x");
  Serial.println(I2C_ADDR,HEX);
  
}

void setupSlaveI2C() {

  for (uint8_t i = 0; i < sizeof(I2C_ADDR_PINS); i++) {
    pinMode(I2C_ADDR_PINS[i], INPUT);  // External pull-up/pull-down resistor
  }
  readAddrI2C();
  
  Wire.begin(I2C_ADDR);
  Wire.setBufferSize(248);
  Wire.onError(onErrorI2C);
  Wire.onReceive(receiveEventI2C);  // Register receive event
  Wire.onRequest(requestEventi2C);  // Register request event  
}
