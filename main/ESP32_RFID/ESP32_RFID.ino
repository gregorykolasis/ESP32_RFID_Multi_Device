#include "CONFIG.h"

// Mode persistence functions
void loadModeFromSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed, formatting...");
    SPIFFS.format();
    SPIFFS.begin(true);
  }
  
  File file = SPIFFS.open(MODE_CONFIG_FILE, "r");
  if (file) {
    uint8_t savedMode = file.parseInt();
    if (savedMode <= VERIFIED_SCANNING) {
      currentMode = (ScanningMode)savedMode;
    } else {
      currentMode = SIMPLE_SCANNING;
    }
    file.close();
  } else {
    currentMode = SIMPLE_SCANNING;
    saveModeToSPIFFS();
  }
}

void saveModeToSPIFFS() {
  File file = SPIFFS.open(MODE_CONFIG_FILE, "w");
  if (file) {
    file.print(currentMode);
    file.close();
  } else {
    Serial.println("Failed to save mode to SPIFFS");
  }
}

void toggleMode() {
  if (currentMode == SIMPLE_SCANNING) {
    currentMode = VERIFIED_SCANNING;
    Serial.println("125KHz Tag Reader switched to VERIFIED_SCANNING mode");
  } else {
    currentMode = SIMPLE_SCANNING;
    Serial.println("125KHz Tag Reader switched to SIMPLE_SCANNING mode");
  }
  saveModeToSPIFFS();
  // Reset verified scanning variables (only for 125KHz)
  lastVerifiedTag125KHz = 0;
  consecutiveScans125KHz = 0;
  verifiedStartTime125KHz = 0;
}

// LED indication functions (only for 125KHz tag reader modes)
void initiateLedBlink() {
  targetBlinks = (currentMode == SIMPLE_SCANNING) ? 2 : 4;
  Serial.print("125KHz Tag Reader Mode: ");
  Serial.println(currentMode == SIMPLE_SCANNING ? "SIMPLE_SCANNING" : "VERIFIED_SCANNING");
  
  blinkCount = 0;
  blinkState = false;
  currentBlinkLed = LED_125KHZ;
  blinkStartTime = millis();
}

void processLedBlink() {
  if (targetBlinks == 0) return;
  unsigned long currentTime = millis();
  if (currentTime < 1000) return;
  
  // Only blink 125KHz LED to show mode (modes only apply to 125KHz tag reader)
  if (!blinkState && (currentTime - blinkStartTime >= BLINK_INTERVAL)) {
    digitalWrite(LED_125KHZ, HIGH);
    blinkState = true;
    blinkStartTime = currentTime;
    //Serial.printf("Blinking HIGH\n");
  } else if (blinkState && (currentTime - blinkStartTime >= BLINK_DURATION)) {
    digitalWrite(LED_125KHZ, LOW);
    //Serial.printf("Blinking LOW\n");
    blinkState = false;
    blinkCount++;
    blinkStartTime = currentTime;
    
    if (blinkCount >= targetBlinks) {
      // Done blinking
      targetBlinks = 0;
      currentBlinkLed = 0;
      Serial.printf("Done blinking\n");

      ledStartupFinished = true;
    }
  }
}

void prepareJsonI2C() {
  i2cJson["v125"] = tag125KHzNumber;
  i2cJson["vNFC"] = tag135MHzNumber;
  i2cJson["mode125"] = (currentMode == SIMPLE_SCANNING) ? "SIMPLE" : "VERIFIED";  // Mode only for 125KHz
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

// Simple LED control - no callbacks needed
void handle125KHzLED(uint32_t tagNumber) {

  if (ledStartupFinished == false) return;

  
  if (currentMode == SIMPLE_SCANNING) {
    // Simple mode: LED on when tag present, off when not
    if (tagNumber > 0) {
      digitalWrite(LED_125KHZ, HIGH);
      led125KHzState = true;
    } else {
      digitalWrite(LED_125KHZ, LOW);
      led125KHzState = false;
    }
  } else {
    // Verified mode: LED on only after verification
    if (tagNumber > 0) {

      if (smallBlinkVerificationFlag && ( millis() - smallBlinkVerificationTimer > 25 ) ) {
        //Serial.printf("Small Verification Led blink Starting point finished \n");
        smallBlinkVerificationFlag = false;
        digitalWrite(LED_125KHZ, LOW);  
      }

      if (tagNumber != lastVerifiedTag125KHz) {
        // New tag - start verification
        lastVerifiedTag125KHz = tagNumber;
        consecutiveScans125KHz = 1;
        verifiedStartTime125KHz = millis();
        digitalWrite(LED_125KHZ, LOW);
        led125KHzState = false;
        //Serial.printf("Tag 125KHz verification started: %d\n", tagNumber);

        smallBlinkVerificationFlag = true;
        smallBlinkVerificationTimer = millis();

        digitalWrite(LED_125KHZ, HIGH);
 
      } else {
        // Same tag - continue verification
        //consecutiveScans125KHz++;
        uint32_t stableScanDuration = millis() - verifiedStartTime125KHz;
        
        if (stableScanDuration >= VERIFIED_SCAN_TIME ) {//&& consecutiveScans125KHz >= VERIFIED_SCAN_MIN_COUNT) {
          if (!led125KHzState) {
            digitalWrite(LED_125KHZ, HIGH);
            led125KHzState = true;
            //Serial.printf("Tag 125KHz VERIFIED: %d (time: %dms) (consecutive: %d)\n", tagNumber, stableScanDuration, tag.consecutiveSameTagCount);
          }
        }
      }
    } else {
      // No tag - reset everything
      digitalWrite(LED_125KHZ, LOW);
      led125KHzState = false;
      lastVerifiedTag125KHz = 0;
      consecutiveScans125KHz = 0;
      verifiedStartTime125KHz = 0;
      smallBlinkVerificationFlag  = false;
      smallBlinkVerificationTimer = millis();
      
    }
  }
}

// Dummy callbacks to prevent crashes
void on125KHzInsert(uint32_t tagNumber) {}
void on125KHzRemove(uint32_t tagNumber) {
  Serial.printf("Tag 125KHz removed: %d (consecutive: %d)\n", tagNumber , tag.consecutiveSameTagCount);
}
void on125KHzChange(uint32_t tagNumber , uint32_t oldTagNumber) {
  Serial.printf("Tag 125KHz changed: %d (old: %d, consecutive: %d)\n", tagNumber , oldTagNumber , tag.consecutiveSameTagCount);
}

// Simple NFC LED control
void handleNFCLED(uint32_t tagNumber) {
  if (tagNumber > 0) {
    digitalWrite(LED_135MHZ, HIGH);
    led135MHzState = true;
  } else {
    digitalWrite(LED_135MHZ, LOW);
    led135MHzState = false;
  }
}

// Dummy callbacks to prevent crashes
void onNFCInsert(uint32_t tagNumber) {}
void onNFCRemove(uint32_t tagNumber) {}
void onNFCChange(uint32_t tagNumber) {}


void readRFID() {

    if (tag125KHzNumber) {
      sendRDM6300uart(tag125KHzNumber);
    }
    if (tag135MHzNumber && !tag125KHzNumber) {
      sendRDM6300uart(tag135MHzNumber);
    }

    if ( tag.loop() ) {

      tag125KHzNumber = tag.getCurrentValue();
      //[Buffer] 2 48 49 48 48 48 70 52 50 55 68 51 49 3

      //[Buffer] 2 48 49 48 48 48 70 52 50 55 68 51 49 3
      //char *endptr;
      //sprintf(tag125KHzStr,"%s",tag.currentTagStr);
      //tag125KHzNumber = strtol(tag125KHzStr, &endptr, 10);

      if (tag125KHzNumber > 0) {
        Serial.printf("Tag 125KHz is %d\n", tag125KHzNumber);
      }
      else {
        Serial.println("Tag 125KHz is out");
      }

      // sprintf(tag125KHzStr,"%s",tag.currentTagStr);
      // if (strlen(tag125KHzStr) > 0 ) {    
      //   char *endptr;
      //   tag125KHzNumber = strtol(tag125KHzStr, &endptr, 10);
      //   if (endptr == tag125KHzStr) {
      //       // No digits were found
      //       Serial.println("Invalid tag: not a number");
      //       tag125KHzNumber = 0;
      //   }
      //   else if (*endptr != '\0') {
      //       // Number had trailing garbage (e.g. "123abc")
      //       Serial.println("Tag contained extra characters");
      //   }
      //   else {
      //       Serial.printf("Tag 125KHz is %ld\n", tag125KHzNumber);
      //   }
      // } else {
      //   Serial.println("Tag 125KMHz is out");   
      //   tag125KHzNumber = 0;
      // }



    }
    
    // Handle LED based on current tag state
    handle125KHzLED(tag125KHzNumber);


    if (!nfcReady) return;
      
    if ( nfc.loop() ) {
      sprintf(tag135MHzStr,"%s",nfc.currentTagStr);
      if (strlen(tag135MHzStr)>0) {
        tag135MHzNumber = strtol(tag135MHzStr, NULL, 10);
        Serial.printf("Tag 13.5MHz is %d\n",tag135MHzNumber);
      } else {
        Serial.println("Tag 13.5MHz is out");
        tag135MHzNumber = 0;
      }
    }
    
    // Handle NFC LED
    handleNFCLED(tag135MHzNumber);
  
}

void readButtonGPIO0() {
    static unsigned long lastDebounceTime = 0;
    static bool lastButtonState = HIGH;
    static bool buttonState = HIGH;
    static bool nfcState = LOW;
    
    bool reading = digitalRead(BUTTON_PIN);
    
    if (reading != lastButtonState) {
        lastDebounceTime = millis();
    }
    
    if ((millis() - lastDebounceTime) > 50) {  // 50ms debounce time
        if (reading != buttonState) {
            buttonState = reading;
            
            if (buttonState == LOW) {  // Button pressed
                if (nfcState == LOW) {
                    Serial.println("Button pressed! Powering down PN532...");
                    nfc.disableRFField();
                    nfc.powerDown();
                } else {
                    Serial.println("Button pressed! Waking up PN532...");
                    nfc.wakeUp();
                    nfc.startListening();
                }
                nfcState = !nfcState;
            }
        }
    }
    
    lastButtonState = reading;
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
    Serial.println(F("DRD - Double Re+set Detected! Toggling 125KHz tag reader mode..."));
    for (uint8_t i=0;i<10;i++) {
      digitalWrite(LED_125KHZ, HIGH);
      delay(25);
      digitalWrite(LED_125KHZ, LOW);
      delay(25);
    }
    toggleMode();
  }
}

void setup() {
    Serial.begin(115200);

    // Initialize LED pins
    pinMode(LED_125KHZ, OUTPUT);
    pinMode(LED_135MHZ, OUTPUT);
    digitalWrite(LED_125KHZ, LOW);
    digitalWrite(LED_135MHZ, LOW);
    
    // Load mode from SPIFFS
    loadModeFromSPIFFS();

    // Setup callbacks for RFID readers
    tag.onInsert = on125KHzInsert;
    tag.onRemove = on125KHzRemove;
    tag.onChange = on125KHzChange;
    
    nfc.onInsert = onNFCInsert;
    nfc.onRemove = onNFCRemove;
    nfc.onChange = onNFCChange;
    
    //setupSlaveI2C();      //Setup slaveI2C
    setup125Khz();
    //setupNfc();
    pinMode(BUTTON_PIN, INPUT_PULLUP);  // Configure button with internal pull-up

    // Setup Double Reset Detection
    setupDrd();

    // Initiate LED blinking to show mode
    initiateLedBlink();
   
}

void loop() {
    // Process LED blinking for mode indication
    processLedBlink();
    
    readRFID();
    //readButtonGPIO0();
    drd->loop();
}
