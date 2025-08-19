#include "Rfid_135MHz.h"
#include <Arduino.h>

// Constructor
Rfid_135MHz::Rfid_135MHz(uint8_t ss_pin, uint8_t irq_pin, uint8_t led_pin)
    : nfc(ss_pin, irq_pin , "Spi_IRQ" ), ledPin(led_pin), irqPin(irq_pin), stateLed(false), readerDisabled(false), pn532PoweredDown(false), timeLastCardRead(0) {
    dbgVerbose = false;
    dbgNormal = false;
}

// Initialize the NFC reader
bool Rfid_135MHz::begin() {
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);

    nfc.begin();
    bool init = getFirmwareVersion();

    startListening();

    return init;
}

// Get PN532 firmware version
bool Rfid_135MHz::getFirmwareVersion() {
    uint32_t versiondata = nfc.getFirmwareVersion();
    if (!versiondata) {
        Serial.println("Didn't find PN53x board");
        return false;
    }
    if (dbgNormal) Serial.print("Found chip PN5"); 
	if (dbgNormal) Serial.println((versiondata >> 24) & 0xFF, HEX);
    if (dbgNormal) Serial.print("Firmware ver. "); 
	if (dbgNormal) Serial.print((versiondata >> 16) & 0xFF, DEC);
    if (dbgNormal) Serial.print('.'); 
	if (dbgNormal) Serial.println((versiondata >> 8) & 0xFF, DEC);
    return true;
}

void Rfid_135MHz::resetTag() {
	tagIsHere = false;
	currentTag = 0;
	memset(currentTagStr,0,sizeof(currentTagStr));		
}

// Start listening for NFC cards
void Rfid_135MHz::startListening() {
    irqPrev = irqCurr = HIGH;
    if (dbgNormal) Serial.println("Starting passive read for an ISO14443A Card ...");
    if (!nfc.startPassiveTargetIDDetection(PN532_MIFARE_ISO14443A)) {	
		resetTag();
        if (dbgNormal) Serial.println("No card found. Waiting...");
        if (stateLed) {
            if (dbgNormal) Serial.println("LED_135MHZ Closed");
            digitalWrite(ledPin, LOW);
            stateLed = false;
        }
    } else {
        if (dbgNormal) Serial.println("Card already present.");
        handleCardDetected();
    }
}

// Handle card detection
void Rfid_135MHz::handleCardDetected() {
    uint8_t success = false;
    uint8_t uid[7] = {0};  // Buffer to store the returned UID
    uint8_t uidLength;     // Length of the UID (4 or 7 bytes depending on the card type)

    // Read the NFC tag's info
    if (nfc.readDetectedPassiveTargetID(uid, &uidLength)) {
		tagIsHere = true;
        if (dbgNormal) Serial.println("Read successful");
        if (dbgNormal) Serial.println("Found an ISO14443A card");
        if (dbgNormal) Serial.print("  UID Length: "); 
		if (dbgNormal) Serial.print(uidLength, DEC); 
		if (dbgNormal) Serial.println(" bytes");
        if (dbgNormal) Serial.print("  UID Value: ");
        if (dbgNormal) nfc.PrintHex(uid, uidLength);

        // If UID length is 4, display card ID (could add more sophisticated handling)
        if (uidLength == 4) {
            tagStr(uid, uidLength);
            //extractDump(uid, uidLength, success);
            //extractUUID(uid);
        }

        // Update LED state
        if (!stateLed) {
            if (dbgNormal) Serial.println("LED_135MHZ Open");
            digitalWrite(ledPin, HIGH);
            stateLed = true;
        }
        timeLastCardRead = millis();
        readerDisabled = true;
    } else {
        if (dbgNormal) Serial.println("Read failed (not a card?)");
    }
}

// Function to extract UUID
void Rfid_135MHz::extractUUID(uint8_t* uid) {
    uint32_t cardid = uid[0];
    cardid <<= 8;
    cardid |= uid[1];
    cardid <<= 8;
    cardid |= uid[2];
    cardid <<= 8;
    cardid |= uid[3];
    if (dbgNormal) Serial.print("Seems to be a Mifare Classic card #");
    if (dbgNormal) Serial.println(cardid);
}

// Function to extract dump from the card
void Rfid_135MHz::extractDump(uint8_t* uid, uint8_t uidLength, uint8_t &success) {
    uint8_t keyuniversal[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t currentblock;
    bool authenticated = false;
    uint8_t data[16];

    Serial.println("Seems to be a Mifare Classic card (4 byte UID)");
    for (currentblock = 0; currentblock < 64; currentblock++) {
        if (nfc.mifareclassic_IsFirstBlock(currentblock)) authenticated = false;

        if (!authenticated) {
            Serial.print("------------------------Sector ");
            Serial.print(currentblock / 4, DEC);
            Serial.println("-------------------------");
            success = nfc.mifareclassic_AuthenticateBlock(uid, uidLength, currentblock, 1, keyuniversal);
            if (!success) {
                Serial.println("Authentication error, trying key B");
                success = nfc.mifareclassic_AuthenticateBlock(uid, uidLength, currentblock, 0, keyuniversal);
            }
            authenticated = success;
        }

        if (!authenticated) {
            Serial.print("Block ");
            Serial.print(currentblock, DEC);
            Serial.println(" unable to authenticate");
        } else {
            success = nfc.mifareclassic_ReadDataBlock(currentblock, data);
            if (success) {
                Serial.print("Block ");
                Serial.print(currentblock, DEC);
                if (currentblock < 10) Serial.print("  ");
                else Serial.print(" ");
                nfc.PrintHexChar(data, 16);
            } else {
                Serial.print("Block ");
                Serial.print(currentblock, DEC);
                Serial.println(" unable to read this block");
            }
        }
    }
}

// Function to print tag string
void Rfid_135MHz::tagStr(uint8_t* uid, uint8_t uidLength) {
    
    uint32_t thisCardTag = 0;

    /*
    https://www.aliexpress.com/item/1005007189455258.html?spm=a2g0o.detail.pcDetailBottomMoreOtherSeller.1.55a07PMg7PMgRR&gps-id=pcDetailBottomMoreOtherSeller&scm=1007.40050.354490.0&scm_id=1007.40050.354490.0&scm-url=1007.40050.354490.0&pvid=ee9080a4-99d3-4d8f-a013-f0265751de50&_t=gps-id:pcDetailBottomMoreOtherSeller,scm-url:1007.40050.354490.0,pvid:ee9080a4-99d3-4d8f-a013-f0265751de50,tpp_buckets:668%232846%238114%231999&pdp_npi=4%40dis%21EUR%2155.73%2120.08%21%21%21422.54%21152.27%21%40211b813f17290727486171686eaae9%2112000039746334132%21rec%21GR%21%21ABX&utparam-url=scene%3ApcDetailBottomMoreOtherSeller%7Cquery_from%3A
    10 Frequency NFC Smart Card Reader Writer Duplicator 125K 13.56MHz RFID Copier USB Fob Programmer Copy Encrypted Key Replicator    
    This RFID Reader on IC-Type A only reads 3 Bytes
    */
    // Reverse byte order to handle endianness issues if necessary
    for (int i = tagStrdecodedBytesLength - 1; i >= 0; i--) {
        thisCardTag = (thisCardTag << 8) | uid[i];
    }
	
	currentTag = thisCardTag;
	
    // Convert the UID to a decimal string
    sprintf(currentTagStr, "%010lu", thisCardTag);

}

// Enable the RF field
void Rfid_135MHz::enableRFField() {
    uint8_t rf_config_command[] = { 0xD4, 0x32, 0x01, 0x01 }; // Enable RF field
    nfc.sendCommandCheckAck(rf_config_command, sizeof(rf_config_command), 1000);
    if (dbgNormal) Serial.println("RF field enabled");
    pn532PoweredDown = false;
}

// Disable the RF field
void Rfid_135MHz::disableRFField() {
    uint8_t rf_config_command[] = { 0xD4, 0x32, 0x01, 0x00 }; // Disable RF field
    nfc.sendCommandCheckAck(rf_config_command, sizeof(rf_config_command), 1000);
    if (dbgNormal) Serial.println("RF field disabled");
    pn532PoweredDown = true;
}

// Power down the PN532
void Rfid_135MHz::powerDown() {
    disableRFField();  // Disable the RF field before powering down
    uint8_t power_down_command[] = { 0xD4, 0x16, 0x00, 0x00 }; // PowerDown, wake up on IRQ
    nfc.sendCommandCheckAck(power_down_command, sizeof(power_down_command), 1000);
    if (dbgNormal) Serial.println("PN532 powered down");
    pn532PoweredDown = true;
}

// Wake up and reinitialize the PN532
void Rfid_135MHz::wakeUp() {
    if (pn532PoweredDown) {
        nfc.begin();           // Reinitialize the PN532
        nfc.SAMConfig();       // Re-enable the SAM configuration
        enableRFField();       // Enable the RF field
        if (dbgNormal) Serial.println("PN532 is fully operational again.");
        pn532PoweredDown = false;
    }
}

// Main loop for managing IRQ and card detection
bool Rfid_135MHz::loop() {
	
	static uint32_t lastTag = 0;
    bool haveTag = tagIsHere;
	bool updated = false;
	
    if (readerDisabled) {
        if (millis() - timeLastCardRead > delayBetweenCards) {
            readerDisabled = false;
            startListening();
        }
    } else {
        irqCurr = digitalRead(irqPin);
        if (irqCurr == LOW && irqPrev == HIGH) {
            if (dbgNormal) Serial.println("Got NFC IRQ");
            if (!pn532PoweredDown) {
                handleCardDetected();
            } else {
                if (dbgNormal) Serial.println("Ignoring IRQ, PN532 is powered down.");
            }
        }
        irqPrev = irqCurr;
    }
	
	if (haveTag && currentTag!=0) {	
		if (currentTag != lastTag ) {  
			lastTag = currentTag; 
			updated = true;
		}
		
	}
	if (!haveTag && lastTag!=0) { 
		lastTag = 0;
		updated = true;
	}
	
	return updated;
}
