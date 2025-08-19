#include <Wire.h>
#include <ArduinoJson.h>

#define I2C_SLAVE_ADDR 0x51
uint8_t addresses [] = {0x51,0x52};
const unsigned long loopEvery = 5;

#define I2C_MAX_BYTES_RX_TX 161
#define I2C_JSON_BUFFER 512
#define I2C_MINIMUM_LENGTH_MESSAGE 4
char i2cBuffer[I2C_JSON_BUFFER/2]; // Buffer for receiving I2C data
uint16_t i2cIndex = 0;
StaticJsonDocument<I2C_JSON_BUFFER> i2cJson;
bool I2C_ERROR = false;
uint32_t I2C_LIFE_ERRORS = 0;

void onErrorI2C(const char* msg, int address) {
  I2C_LIFE_ERRORS++;
  I2C_ERROR = true;
  Serial.printf("[onErrorI2C] Address:%d Message:%s \n",address,msg);
}

// Function to send JSON data
void sendJSON(uint8_t address) {
  Wire.beginTransmission(address);
  serializeJson(i2cJson, Wire);  // Send the JSON document
  Wire.endTransmission();
}

void requestJSON(uint8_t address) {

  i2cIndex = 0;
  memset(i2cBuffer,0,sizeof(i2cBuffer));
  i2cJson.clear();
  
  static bool available = false;

  unsigned long microsBefore = micros();
  
  Wire.requestFrom(address, I2C_MAX_BYTES_RX_TX);

  while (Wire.available() && i2cIndex < sizeof(i2cBuffer) - 1) {
    i2cBuffer[i2cIndex++] = Wire.read();
  } 
  i2cBuffer[i2cIndex] = '\0'; 
  //Serial.printf("Raw data length:%d received:%s\n",i2cIndex,i2cBuffer);
   
  if (i2cIndex>I2C_MINIMUM_LENGTH_MESSAGE) {
    available = true;
  }
  else {
    available = false;  
  }

  if (available) {
    DeserializationError error = deserializeJson(i2cJson, i2cBuffer);
    if (error) {
      Serial.print("[Json-ERROR] ");
      Serial.println(i2cBuffer);  // Print raw data if parsing fails
    } else {   
      Serial.println("RFID Data received:");
      serializeJsonPretty(i2cJson, Serial);  // Print the parsed JSON
      Serial.printf("\nTotal execution time is %d in mS\n",(micros()-microsBefore)/1000);    
    }

  } 
  else {
     Serial.println("No response");      
  }

}

void setup() {
  Serial.begin(115200);
  
  uint32_t theFreg = 100e3;//400000;
  Wire.begin (21, 22 , theFreg); // sda= GPIO_21 /scl= GPIO_22 ESP32 //
  Wire.onError(onErrorI2C);
  Wire.setBufferSize(248);

  //i2cJson["mode"] = "scan";
  //i2cJson["interval"] = 500;  // Set scanning interval
  //sendJSON(I2C_SLAVE_ADDR);

}

void loop() {
	
  static unsigned long callI2C = 0;
  if (millis() - callI2C > loopEvery ) {
    //Wire.setClock(100e3);
    for (uint8_t i=0;i<sizeof(addresses);i++){
		requestJSON(addresses[i]);
	}
    //Wire.setClock(100e3);
    callI2C = millis();
  }


}
