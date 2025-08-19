#include <Wire.h>
#include <ArduinoJson.h>

#define I2C_SLAVE_ADDR 0x08
#define I2C_MAX_BYTES_RX_TX 161
#define I2C_MAX_LEN_ALLOWED_JSON I2C_MAX_BYTES_RX_TX-1

StaticJsonDocument<128> configDoc;  // Configuration storage
StaticJsonDocument<512> rfidDoc;    // RFID data storage

void receiveEvent(int howMany) {
  String jsonString = "";

  while (Wire.available()) {
    char c = Wire.read();
    jsonString += c;  // Collect received characters
  }

  // Parse the incoming JSON
  DeserializationError error = deserializeJson(configDoc, jsonString);
  if (!error) {
    // Process configuration (for example, update scanning mode or interval)
    const char* mode = configDoc["mode"];
    int interval = configDoc["interval"];
    Serial.print("Received mode: ");
    Serial.println(mode);
    Serial.print("Received interval: ");
    Serial.println(interval);
  } else {
    Serial.println("Error: Failed to parse configuration.");
  }
}

void requestEvent() {

  // Populate RFID JSON data (as an example)
  rfidDoc["rfid"] = "12345678901234567890";  // Example RFID tag number
  rfidDoc["status"] = "success";   // Status of the scan
  rfidDoc["result"] = "ohmygodlookatthatbutt";   // Status of the scan
  rfidDoc["field2"] = "lolenmyfriend";   // Status of the scan
  rfidDoc["attenuation"] = "attenuatorr";   // Status of the scan
  rfidDoc["boleklolen"] = "this1234";   // Status of the scan

  // Send the RFID JSON data to the Master

  // Measure the size of the serialized JSON
  size_t jsonSize = measureJson(rfidDoc);
  Serial.print("Size of JSON to send: ");
  Serial.println(jsonSize);

  // Check if the JSON size exceeds the I2C buffer (248 bytes)
  if (jsonSize > I2C_MAX_LEN_ALLOWED_JSON) {
    Serial.println("Error: JSON size exceeds I2C buffer limit!");
    return;  // Stop if too large
  }

  // Send the RFID JSON data to the Master
  char buf[248];
  serializeJson(rfidDoc, buf);  // Serialize JSON into the buffer

  for (uint8_t i = 0; i < strlen(buf); i++) {
    Wire.write(buf[i]);  // Send byte by byte over I2C
  }

  //serializeJson(rfidDoc, Wire);
}

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SLAVE_ADDR);  // Initialize I2C as Slave
  Wire.setBufferSize(248);
  Wire.onReceive(receiveEvent);  // Register receive event
  Wire.onRequest(requestEvent);  // Register request event

}

void loop() {
  // Your main loop code
  delay(100);
}
