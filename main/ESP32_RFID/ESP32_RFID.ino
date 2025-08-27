#include "CONFIG.h"

void prepareJsonI2C() {
  i2cJson["v125"] = tag125KHzNumber;
  i2cJson["vNFC"] = tag135MHzNumber;
}

void sendRDM6300uart(uint32_t tagNumber) {
    static unsigned long sendUartTime = 0;
    if (millis()-sendUartTime>RDM6300_SEND_EVERY) {
      sendUartTime = millis();
      char rdm6300[16];
      tag.createRDM6300Frame(tagNumber,(char*)&rdm6300,sizeof(rdm6300));
      if (dbgRdm6300) Serial.print("[Buffer] ");
      for (uint8_t i=0;i<14;i++) {
        Serial2.print(rdm6300[i]);
        if (dbgRdm6300) Serial.print(rdm6300[i],DEC);
        if (dbgRdm6300) Serial.print(" ");
      }
      if (dbgRdm6300) Serial.println();
      Serial2.flush();
    }
}

void readRFID() {

    if (tag125KHzNumber) {
      sendRDM6300uart(tag125KHzNumber);
    }
    if (tag135MHzNumber && !tag125KHzNumber) {
      sendRDM6300uart(tag135MHzNumber);
    }

    if ( tag.loop() ) {
      sprintf(tag125KHzStr,"%s",tag.currentTagStr);
      if (strlen(tag125KHzStr)>0) {    
        tag125KHzNumber = strtol(tag125KHzStr, NULL, 10);
        //Serial.printf("Tag 125KHz is %s\n",tag125KHzStr);
        Serial.printf("Tag 125KHz is %d\n",tag125KHzNumber);  
      } else {
        Serial.println("Tag 125KMHz is out");   
        tag125KHzNumber = 0;
      }
    }

    if (!nfcReady) return;
      
    if ( nfc.loop() ) {
      sprintf(tag135MHzStr,"%s",nfc.currentTagStr);
      if (strlen(tag135MHzStr)>0) {
        tag135MHzNumber = strtol(tag135MHzStr, NULL, 10);
        //Serial.printf("Tag 13.5MHz is %s\n",tag135MHzStr);
        Serial.printf("Tag 13.5MHz is %d\n",tag135MHzNumber);
        
      } else {
        Serial.println("Tag 13.5MHz is out");
        tag135MHzNumber = 0;
      }
    }
  
}

void readButtonGPIO0() {
  
    // Handle button press for waking up or powering down the NFC module
    if (digitalRead(BUTTON_PIN) == LOW) {
        static bool state = LOW;
        if (state == LOW) {
            Serial.println("Button pressed! Powering down PN532...");
            nfc.disableRFField();
            nfc.powerDown();
        } else {
            Serial.println("Button pressed! Waking up PN532...");
            nfc.wakeUp();
            nfc.startListening();
        }
        state = !state;
        delay(50);  // Debounce the button
    }  
  
}

void setup125Khz() {

  tag.dbgNormal = false;
  tag.setup();

  Serial2.begin(RDM6300_BAUDRATE, SERIAL_8N1, RDM6300_RX_PIN , RDM6300_TX_PIN); //RDM6300 emulation on TX pin
}

void setupNfc() {

  nfc.setReadType(NFC_SIMPLE_3BYTE_READ);
  nfc.delayBetweenCards = 0;
  nfc.dbgNormal = false;
  if ( !nfc.begin() ) {
    //Serial.printf("Couldn't find RFID Reader at 13.5MHz\n");  
  }

}

void setupDrd() {
  drd = new DoubleResetDetector(DRD_TIMEOUT, DRD_ADDRESS);
  if (drd->detectDoubleReset())
  {
    Serial.println(F("DRD"));
  }
}

void setup() {
    Serial.begin(115200);
    setupDrd();
    //setupSlaveI2C();      //Setup slaveI2C
    setup125Khz();
    //setupNfc();
    pinMode(BUTTON_PIN, INPUT_PULLUP);  // Configure button with internal pull-up
}

void loop() {
 
    readRFID();
    //readButtonGPIO0();
    drd->loop();
}
