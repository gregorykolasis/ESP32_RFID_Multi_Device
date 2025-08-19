
#define LED_125KHZ 14
static bool stateLed125Khz = false;

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>

#define PN532_SS  5
#define PN532_IRQ 13
Adafruit_PN532 nfc(PN532_SS, 13, "Spi_IRQ");

#define LED_135MHZ 15
static bool stateLed135Mhz = false;

#define BUTTON_PIN 0  // GPIO 0 for BOOT button


bool pn532PoweredDown = false;
void disableRFField() {
  uint8_t rf_config_command[] = { 0xD4, 0x32, 0x01, 0x00 }; // RFConfiguration, disable RF field
  nfc.sendCommandCheckAck(rf_config_command, sizeof(rf_config_command), 1000);
  Serial.println("RF field disabled");
}

void enableRFField() {
  uint8_t rf_config_command[] = { 0xD4, 0x32, 0x01, 0x01 }; // RFConfiguration, enable RF field
  nfc.sendCommandCheckAck(rf_config_command, sizeof(rf_config_command), 1000);
  Serial.println("RF field enabled");
}

// Function to power down the PN532
void powerDownPN532() {
  uint8_t power_down_command[] = { 0xD4, 0x16, 0x00, 0x00 }; // PowerDown, wakeup on IRQ
  nfc.sendCommandCheckAck(power_down_command, sizeof(power_down_command), 1000);
  Serial.println("PN532 powered down");
  pn532PoweredDown = true;
}

void wakeUpPN532() {
  // Reinitialize the PN532
  nfc.begin();
  
  // Re-enable the SAM (Security Access Module) configuration
  nfc.SAMConfig();
  
  // Enable the RF field
  enableRFField();
  Serial.println("PN532 is fully operational again.");

  pn532PoweredDown = false;
}






const int DELAY_BETWEEN_CARDS = 25;
long timeLastCardRead = 0;
boolean readerDisabled = false;
int irqCurr;
int irqPrev;

void startListeningToNFC() {
  // Reset our IRQ indicators
  irqPrev = irqCurr = HIGH;
  Serial.println("Starting passive read for an ISO14443A Card ...");
  if (!nfc.startPassiveTargetIDDetection(PN532_MIFARE_ISO14443A)) {
    Serial.println("No card found. Waiting...");

    if (stateLed135Mhz) {
      Serial.println("LED_135MHZ Closed");
      digitalWrite(LED_135MHZ,LOW);
      stateLed135Mhz = false;  
    }
    
  } else {
    Serial.println("Card already present.");
    handleCardDetected();
  }
}

void extractUUID(uint8_t* uid) {

  // We probably have a Mifare Classic card ...
  uint32_t cardid = uid[0];
  cardid <<= 8;
  cardid |= uid[1];
  cardid <<= 8;
  cardid |= uid[2];
  cardid <<= 8;
  cardid |= uid[3];
  Serial.print("Seems to be a Mifare Classic card #");
  Serial.println(cardid);
  
}

void extractDump(uint8_t* uid , uint8_t uidLength , uint8_t & success) {

    // Keyb on NDEF and Mifare Classic should be the same
  uint8_t keyuniversal[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
  
  uint8_t currentblock;                     // Counter to keep track of which block we're on
  bool authenticated = false;               // Flag to indicate if the sector is authenticated
  uint8_t data[16];                         // Array to store block data during reads

  // We probably have a Mifare Classic card ...
  Serial.println("Seems to be a Mifare Classic card (4 byte UID)");
  // Now we try to go through all 16 sectors (each having 4 blocks)
  // authenticating each sector, and then dumping the blocks
  for (currentblock = 0; currentblock < 64; currentblock++)
  {
    // Check if this is a new block so that we can reauthenticate
    if (nfc.mifareclassic_IsFirstBlock(currentblock)) authenticated = false;

    // If the sector hasn't been authenticated, do so first
    if (!authenticated)
    {
      // Starting of a new sector ... try to to authenticate
      Serial.print("------------------------Sector ");Serial.print(currentblock/4, DEC);Serial.println("-------------------------");
      if (currentblock == 0)
      {
          // This will be 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF for Mifare Classic (non-NDEF!)
          // or 0xA0 0xA1 0xA2 0xA3 0xA4 0xA5 for NDEF formatted cards using key a,
          // but keyb should be the same for both (0xFF 0xFF 0xFF 0xFF 0xFF 0xFF)
          success = nfc.mifareclassic_AuthenticateBlock (uid, uidLength, currentblock, 1, keyuniversal);
      }
      else
      {
          // This will be 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF for Mifare Classic (non-NDEF!)
          // or 0xD3 0xF7 0xD3 0xF7 0xD3 0xF7 for NDEF formatted cards using key a,
          // but keyb should be the same for both (0xFF 0xFF 0xFF 0xFF 0xFF 0xFF)
          success = nfc.mifareclassic_AuthenticateBlock (uid, uidLength, currentblock, 1, keyuniversal);
      }
      if (success)
      {
        authenticated = true;
      }
      else
      {
        Serial.println("Authentication error");
      }
    }
    // If we're still not authenticated just skip the block
    if (!authenticated)
    {
      Serial.print("Block ");Serial.print(currentblock, DEC);Serial.println(" unable to authenticate");
    }
    else
    {
      // Authenticated ... we should be able to read the block now
      // Dump the data into the 'data' array
      success = nfc.mifareclassic_ReadDataBlock(currentblock, data);
      if (success)
      {
        // Read successful
        Serial.print("Block ");Serial.print(currentblock, DEC);
        if (currentblock < 10)
        {
          Serial.print("  ");
        }
        else
        {
          Serial.print(" ");
        }
        // Dump the raw data
        nfc.PrintHexChar(data, 16);
      }
      else
      {
        // Oops ... something happened
        Serial.print("Block ");Serial.print(currentblock, DEC);
        Serial.println(" unable to read this block");
      }
    }
  }
 
}

void tagStr(uint8_t *uid, uint8_t uidLength) {
    char currentTagStr[17];
    uint32_t thisCardTag = 0;

    // Reverse byte order to handle endianness issues if necessary
    for (int i = 3 - 1; i >= 0; i--) {
        thisCardTag = (thisCardTag << 8) | uid[i];
    }

    // Convert the UID to a decimal string
    sprintf(currentTagStr, "%010lu", thisCardTag);

    // Print the resulting tag string
    Serial.println(currentTagStr);
}


void handleCardDetected() {
  uint8_t success = false;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

  // read the NFC tag's info
  success = nfc.readDetectedPassiveTargetID(uid, &uidLength);
  Serial.println(success ? "Read successful" : "Read failed (not a card?)");

  if (success) {
    // Display some basic information about the card
    Serial.println("Found an ISO14443A card");
    Serial.print("  UID Length: "); Serial.print(uidLength, DEC); Serial.println(" bytes");
    Serial.print("  UID Value: ");
    nfc.PrintHex(uid, uidLength);

    if (uidLength == 4)
    {
        tagStr(uid, uidLength);
        extractDump((uint8_t*)&uid,uidLength,success);
        extractUUID((uint8_t*)&uid);
    }
    Serial.println("");
    timeLastCardRead = millis();

    if (!stateLed135Mhz) {
      Serial.println("LED_135MHZ Open");
      digitalWrite(LED_135MHZ,HIGH);
      stateLed135Mhz = true;  
    }
    
    
  }

  // The reader will be enabled again after DELAY_BETWEEN_CARDS ms will pass.
  readerDisabled = true;
}

void setup() {
  Serial.begin(115200);

  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("Firmware ver. "); Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.'); Serial.println((versiondata >> 8) & 0xFF, DEC);
  startListeningToNFC();
  pinMode(LED_135MHZ,OUTPUT);
  

  pinMode(BUTTON_PIN, INPUT);
  // Disable RF field


}

void loop() {

  //static unsigned long timeLoop = millis();

//  if (digitalRead(BUTTON_PIN) == LOW) {
//    static bool state = LOW;
//    if (state==LOW) {
//      Serial.println("Button pressed! Put to Sleep PN532...");
//      disableRFField();
//      powerDownPN532();
//    }
//    else {
//      Serial.println("Button pressed! Waking up PN532...");
//      wakeUpPN532();
//      startListeningToNFC();
//    }
//    state=!state;
//    // Wake up and reinitialize PN532
//    // Optional: Debounce the button by adding a delay
//    delay(500);
//  }



  if (readerDisabled) {
    if (millis() - timeLastCardRead > DELAY_BETWEEN_CARDS) {
      readerDisabled = false;
      startListeningToNFC();
    }
  } else {
    irqCurr = digitalRead(PN532_IRQ);

    // When the IRQ is pulled low - the reader has got something for us.
    if (irqCurr == LOW && irqPrev == HIGH) {
      Serial.println("Got NFC IRQ");
      if (!pn532PoweredDown) {
        handleCardDetected();        
      }
      else {
        Serial.println("Ignoring it im sleeping");
      }
      

    }

    irqPrev = irqCurr;
  }

  //  static unsigned long loopTook = millis()-timeLoop;
  //  timeLoop = millis();
  //  static unsigned long printIt = 0;
  //  if (millis()-printIt > 1000) {
  //    printIt = millis();
  //    Serial.printf("Loop took %d millis\n",loopTook );
  //  }

}
