#include "Rfid_125KHz.h"
#include "driver/ledc.h"

// Static instance pointer initialization
Rfid_125KHz* Rfid_125KHz::instance = nullptr;
const char* Rfid_125KHz::codeStr[2] = {"EM4100", "FDX-B"};

// Constructor implementation
Rfid_125KHz::Rfid_125KHz(float frequency, int demod_pin, int clock_pin, int led_pin , bool modulationType)
    : rfidFreq(frequency), rfidDemod(demod_pin), rfidClock(clock_pin), ledPin(led_pin), encodeFDX(modulationType), captureIndex(0), captured(false), pulseIndex(0) {
    currentTag = 0;
    buttonPressed = false;
    dbgVerbose = false;
    dbgNormal = false;
    memset(currentTagStr, 0, sizeof(currentTagStr));
    memset(encodingStr, 0, sizeof(encodingStr));
}

// Public methods

void IRAM_ATTR Rfid_125KHz::demodISR() {
    if (instance) {
        instance->lastMicrosInterrupt = micros();
        instance->demod(); // Call the instance-specific method
    }
}

void Rfid_125KHz::demod() {

    bool invalidPulse = false;
    
    static uint8_t level = 1;
    static bool gotFirstShort = false;
    static uint32_t lastMicros = 0;
    uint32_t elapsedMicros;

    if (!captured) {
        elapsedMicros = simulation ? simPulse : micros() - lastMicros; 
        lastMicros = micros();
        if (captureIndex >= captureSize) {
            captured = true;
        } else {
            if (elapsedMicros > minPulse && elapsedMicros < maxPulse) {
                if (elapsedMicros > slBoundary) {
                    // Long pulse
                    if (gotFirstShort) invalidPulse = true;
                    else {
                        if (encodeFDX) {
                            bitBuffer[captureIndex++] = 1; // FDX-B long pulse = logic 1
                        } else {
                            level ^= 1; // EM4100 long pulse changes logic level
                            bitBuffer[captureIndex++] = level;
                        }
                    }
                } else {
                    // Short pulse, FDX-B = logic 0, EM4100 same as previous level
                    if (gotFirstShort) {
                        bitBuffer[captureIndex++] = encodeFDX ? 0 : level;
                        gotFirstShort = false;
                    } else {
                        gotFirstShort = true;
                    }
                }
                pulses[pulseIndex++] = elapsedMicros; // Debugging
            } else {
                invalidPulse = true;
            }

            // Restart capture on outliers unless minimum length achieved
            if (invalidPulse) {
                gotFirstShort = false;
                if (captureIndex < minLength) {
                    captureIndex = pulseIndex = 0; // Restart capture
                } else {
                    captured = true; // If minimum capture length achieved
                }
            }
        }
    }
    
}

void Rfid_125KHz::setup() {
  
    instance = this;

    codeLength = (encodeFDX) ? 128 : 64;
    if (!rfidFreq) rfidFreq = (encodeFDX) ? 134.2 : 125.0;
    minPulse = (encodeFDX) ? 85 : 170;
    maxPulse = (encodeFDX) ? 320 : 560;
    slBoundary = (encodeFDX) ? 180 : 360;
    captureSize = codeLength * 2;

    Serial.printf("codeLength %d, minPulse %d, maxPulse %d, slBoundary %d, captureSize %d \n", codeLength, minPulse, maxPulse, slBoundary, captureSize);
    if (!simulation) {
        pinMode(rfidDemod, INPUT_PULLUP);
        pinMode(rfidClock, OUTPUT);
        runClock();
        attachInterrupt(digitalPinToInterrupt(rfidDemod), Rfid_125KHz::demodISR, CHANGE);
    }
	pinMode(ledPin, OUTPUT);

    sprintf(encodingStr, "%s %0.1fkHz", codeStr[encodeFDX], rfidFreq);
    Serial.printf("Detect tag type: %s\n", encodingStr);
}

bool Rfid_125KHz::loop() {
	
    static uint64_t lastTag = 0;
    bool haveTag = false;
    bool isRetry = false;
	
	bool updated = false;

    //if (!currentTag) lastTag = 0;

    if (captured) {
        haveTag = getTag(isRetry);
        if (!haveTag && !encodeFDX) {
            for (int i = 0; i < bitBufferSize; i++) bitBuffer[i] ^= 1;
            isRetry = true;
            haveTag = getTag(isRetry); 
        }

        showRawData(haveTag);
        captureIndex = pulseIndex = 0;
        memset(bitBuffer, 0, sizeof(bitBuffer));
        captured = false;
    } else if (simulation) {
        demodSim();
    }
	
	unsigned long validRfidTimePassed = millis()-lastValidRead;

    if (haveTag && currentTag!=0) {
    //sprintf(currentTagS, "%llu", currentTag);
    //Serial.printf("currentTag = %s \n",currentTagS);   
	
        if (currentTag != lastTag ) {         
            //char lastTagS[21]; // Max 20 digits for uint64_t + 1 for null terminator
            //char currentTagS[21]; // Max 20 digits for uint64_t + 1 for null terminator          
            //sprintf(lastTagS, "%llu", lastTag); 
            //sprintf(currentTagS, "%llu", currentTag);
            //Serial.printf("lastTag = %s | currentTag = %s \n",lastTagS,currentTagS);    
            //Serial.printf("Tag is %s\n",currentTagS);		
            if (lastTag != 0 && currentTag != 0) {
                if (dbgNormal) Serial.printf("[NOISE------------------------------] old was %llu newTag is %llu \n", lastTag, currentTag);
                // Tag changed from one to another
                if (onChange) {
                    uint32_t tagNumber = (uint32_t)currentTag;
                    onChange(tagNumber,lastTag);
                }
            } else if (lastTag == 0 && currentTag != 0) {
                // New tag inserted
                if (onInsert) {
                    uint32_t tagNumber = (uint32_t)currentTag;
                    onInsert(tagNumber);
                }
            }   
            consecutiveSameTagCount = 0;
            lastTag = currentTag;
			updated = true;
        }

        if (currentTag == lastTag) {
            consecutiveSameTagCount++;
        }

        

        if (dbgNormal) Serial.println();		
    }
    else if (!haveTag && lastTag!=0) {
      //if i get lastMicrosInterrupt value , and don't assign it to a variable first, IT WILL NOT WORK , Race condition will happen and currentMicros will be Smaller
      uint32_t lastMicrosCopy = lastMicrosInterrupt;
      uint32_t currentMicros = micros();
      uint32_t noInterruptsTimePassed = (currentMicros >= lastMicrosCopy) ? (currentMicros - lastMicrosCopy) : 0;                   

	  /*
	    There can be a point where Interrupt are coming , but are wrong or not enough to detect a Valid Rfid , so we need
		validRfidTimePassed < 1000mS , meaning at least read it 1 time succesfully in 1000ms , 
		In the best Position validRfidTimePassed is always at the worst 130ms
	  */
	  
	  bool isRfidOutside = validRfidTimePassed > validRfidTimeout;
	  if (!useValidRfidTimeout) isRfidOutside = false;
	  
	  
      if (noInterruptsTimePassed > microsConsiderTagIsOut || isRfidOutside) {
          //Serial.printf("Time without Interrupt: %u in uS \n", noInterruptsTimePassed);
		  		
          //Serial.printf("Tag is out \n");    
		  memset(currentTagStr,0,sizeof(currentTagStr));
          currentTag = 0;
          
          // Tag removed
          if (onRemove) {
              onRemove(lastTag);
          }
          
          lastTag = 0;	  
		  updated = true;
      }
    }
	
	/*
	static unsigned long printValidRfid = millis();
	if (millis()-printValidRfid>1000) {
		printValidRfid = millis();
		Serial.printf("Time without validRFID: %u in mS \n", validRfidTimePassed);
	}	
	*/

	
	return updated;
}

// Private methods

void Rfid_125KHz::runClock() {
    int dutyBits = 1;
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    ledcAttach(rfidClock, (int)(rfidFreq * 1000), dutyBits); 
    ledcWrite(rfidClock, 1);
#else
    int rfidChannel = 4;
    ledcSetup(rfidChannel, (int)(rfidFreq * 1000), dutyBits); 
    ledcAttachPin(rfidClock, rfidChannel);
    ledcWrite(rfidChannel, 1);
#endif
}

void Rfid_125KHz::showRawData(bool haveTag) {
    if (dbgVerbose) {
        Serial.printf("------------------------------------------\n");
        Serial.printf("pulses: %d\n", pulseIndex);
        Serial.printf("slBoundary: %d\n", slBoundary);
        Serial.printf("pulselen: ");
        for (int i = 0; i < pulseIndex; i++) Serial.printf("%d/", pulses[i]);
        Serial.printf("\ndecoded: ");
        for (int i = 0; i < captureIndex; i++) Serial.printf("%u", bitBuffer[i]);
        if (haveTag) {
            Serial.printf("\ntag: ");
            for (int i = 0; i < minLength; i++) Serial.printf("%u", bitBuffer[headerPos + i]);
        }
        Serial.printf("\n------------------------------------------\n\n");
    }
}

bool Rfid_125KHz::findHeader() {
    uint8_t theBit;
    uint8_t countBit = 0;
    uint8_t countEnd = (encodeFDX) ? 10 : 9;

    for (uint16_t i = 0; i <= codeLength; i++) {
        theBit = bitBuffer[i];
        countBit = theBit ? (encodeFDX ? 0 : countBit + 1) : (encodeFDX ? countBit + 1 : 0);
        if (countBit == countEnd) {
            if (encodeFDX) return parseBitBufferFDX(i);
            else return parseBitBufferEM(i);
        }
    }
    if (dbgNormal) Serial.printf("No %s header found\n", codeStr[encodeFDX]);
    return false;
}

bool Rfid_125KHz::parseBitBufferEM(uint8_t currIndex) {
    uint8_t theBit;
    for (uint8_t i = 0; i < 4; i++) columnSums[i] = 0;
    uint16_t countryId = 0;
    uint64_t cardTag = 0;

    uint32_t offset = currIndex - 8;
    if (dbgNormal) Serial.printf("%s header start at: %d \n", codeStr[encodeFDX], offset);
    headerPos = offset;

    uint32_t index = offset + 63;
    theBit = bitBuffer[index];
    if (theBit) {
        if (dbgNormal) Serial.printf("EM stopbit not 0 \n");
        return false;
    }

    uint8_t nibble;
    for (uint8_t count = 0; count < 2; count++) {
        if (!parseNibble(offset + 9 + 5 * count, &nibble)) {
            return false;
        }
        countryId |= nibble << (4 * (1 - count));
    }

    for (uint8_t count = 0; count < 8; count++) {
        if (!parseNibble(offset + 19 + 5 * count, &nibble)) {
            if (dbgNormal) Serial.printf("EM Wrong parity for card UID, nibble %d \n", count);
            return false;
        }
        cardTag |= ((long)nibble) << (4 * (7 - count));
    }

    index = offset + 9 + 10 * 5;
    for (uint8_t i = 0; i < 4; i++) {
        theBit = bitBuffer[index];
        if ((columnSums[i] & 1) != theBit) {
            if (dbgNormal) Serial.printf(" - Wrong parity for column %d \n", i);
            return false;
        }
        index++;
    }

    currentTag = countryId;
    currentTag = (currentTag << 32) + cardTag;
    return true;
}

bool Rfid_125KHz::parseNibble(word offset, uint8_t *nibble) {
    uint8_t theBit;
    uint8_t bitSum = 0;

    *nibble = 0;
    for (uint8_t i = 0; i < 4; i++) {
        theBit = bitBuffer[offset];
        columnSums[i] += theBit;
        bitSum += theBit;
        *nibble |= (theBit << (3 - i));
        offset++;
    }

    theBit = bitBuffer[offset];
    if ((bitSum & 1) != theBit) return false;

    return true;
}

bool Rfid_125KHz::parseBitBufferFDX(uint8_t currIndex) {
    uint8_t theBit;
    uint16_t countryId = 0;
    uint64_t cardTag = 0;

    currIndex++;
    theBit = bitBuffer[currIndex];
    if (!theBit) {
        if (dbgNormal) Serial.printf("FDX-B header not terminated");
        return false;
    }
    headerPos = currIndex - 10;
    if (dbgNormal) Serial.printf("%s header start at: %d", codeStr[encodeFDX], headerPos);

    uint8_t tagId[48];
    uint8_t idCntr = 0;
    word count = 0;
    uint8_t offset = currIndex + 1;
    for (uint8_t i = offset; i < offset + 53; i++) {
        theBit = bitBuffer[i];
        if (count < 8) {
            tagId[idCntr] = theBit;
            count++;
            idCntr++;
        } else {
            if (!theBit) {
                if (dbgNormal) Serial.printf("FDX-B control bit wrong");
                return false;
            } else count = 0;
        }
    }

    for (uint8_t i = 38; i > 0; i--) {
        cardTag = cardTag << 1;
        if (tagId[i - 1]) cardTag |= 1;
    }

    for (uint8_t i = 48; i > 38; i--) {
        countryId = countryId << 1;
        if (tagId[i - 1]) countryId |= 1;
    }

    currentTag = countryId;
    currentTag = (currentTag << 38) + cardTag;
    return true;
}

void Rfid_125KHz::tagStr() {
    uint64_t thisCardTag;
    uint16_t thisCountryId;
    uint64_t bitmask = 0x00000000FFFFFFFF;
    int cardTagBits = 32;
    int cardTagLen = 10;
    if (encodeFDX) {
        cardTagBits = 38;
        cardTagLen = 12;
        bitmask = 0x0000003FFFFFFFFF;
    }
    thisCardTag = (currentTag & bitmask);
    thisCountryId = (uint16_t)(currentTag >> cardTagBits);
    sprintf(currentTagStr, "%03u", thisCountryId);
    for (size_t i = cardTagLen; i--; thisCardTag /= 10) currentTagStr[i + 3] = '0' + (thisCardTag % 10);
    currentTagStr[cardTagLen + 3] = 0;
}

bool Rfid_125KHz::getTag(bool retry) {
    if (findHeader()) {
		lastValidRead = millis();
        tagStr();
        if (dbgNormal) Serial.printf("Read tag - %s \n", currentTagStr);
        return true;
    } else {
        if (retry) {
            if (dbgNormal) Serial.printf("Tag not read \n");
        }
        currentTag = currentTagStr[0] = 0;
        return false;
    }
}

void Rfid_125KHz::demodSim() {
    uint16_t pulseCount = 0;
    const char* delimiter = "/";
    const char* pulseLensEM = "200/310/457/308/203/306/204/307/...";
    const char* pulseLensFDX = "90/115/100/225/110/110/95/115/...";
    char pulseLens[2048];
    encodeFDX ? strcpy(pulseLens, pulseLensFDX) : strcpy(pulseLens, pulseLensEM);
    for (int i = 0; pulseLens[i] != '\0'; ++i) if (pulseLens[i] == '/') ++pulseCount;
    char* token = strtok(pulseLens, delimiter);
    for (int i = 0; i < pulseCount; i++) {
        simPulse = static_cast<uint16_t>(atoi(token));
        demodISR();
        token = strtok(nullptr, delimiter);
        if (token == nullptr) break;
    }
    captured = true;
}

void Rfid_125KHz::createRDM6300Frame(uint32_t tagNumber, char* frameBuffer, size_t frameBufferSize) {

    // Constants
    const int BUFFER_SIZE   = 14; // total frame size: 1 STX + 10 data + 2 checksum + 1 ETX
    const int DATA_SIZE     = 10;   // 2 hex chars for version + 8 hex chars for tag
    const int CHECKSUM_SIZE = 2;

    // frameBuffer must hold 14 ASCII bytes plus a null terminator if printing as a string
    // Ensure frameBufferSize >= 15
    if (frameBufferSize < (BUFFER_SIZE + 1)) return;

    // 1. Define a version byte: let's use 0x00
    uint8_t versionByte = 0x01;

    // 2. Convert tag number to a 4-byte value (little endian or big endian doesn't matter for XOR,
    //    but for consistency, let's consider big-endian hex formatting).
    //    We'll produce 8 hex chars representing the tag.
    //    Example: if tagNumber = 4660 decimal, hex is 0x00001234.
    char versionHex[3];
    char tagHex[9];
    sprintf(versionHex, "%02X", versionByte);
    sprintf(tagHex, "%08X", (unsigned int)tagNumber);

    // Combine to form the 10 ASCII hex chars of data
    char dataHex[DATA_SIZE + 1]; // 10 + null terminator
    snprintf(dataHex, sizeof(dataHex), "%s%s", versionHex, tagHex); 
    // dataHex now has something like "0000001234" for our example.

    // 3. Convert these 10 ASCII hex chars (5 bytes) into raw bytes for checksum calculation.
    // Each pair of ASCII hex chars represents one byte.
    uint8_t rawData[5];
    for (int i = 0; i < 5; i++) {
        char byteStr[3] = { dataHex[i*2], dataHex[i*2+1], '\0' };
        rawData[i] = (uint8_t)strtol(byteStr, NULL, 16);
    }

    // 4. Compute checksum by XOR of all 5 bytes
    uint8_t checksumByte = rawData[0] ^ rawData[1] ^ rawData[2] ^ rawData[3] ^ rawData[4];

    // Convert checksum to 2 ASCII hex chars
    char checksumHex[3];
    sprintf(checksumHex, "%02X", checksumByte);

    // 5. Construct the final frame:
    // STX (0x02), then dataHex(10 chars), then checksumHex(2 chars), then ETX(0x03)
    // ASCII characters 0x02 and 0x03 are control characters, not part of the ASCII hex strings
    frameBuffer[0] = 0x02; // STX
    memcpy(&frameBuffer[1], dataHex, 10);
    memcpy(&frameBuffer[11], checksumHex, 2);
    frameBuffer[13] = 0x03; // ETX
    //frameBuffer[14] = '\0'; // Null terminator if printing as a string

    // frameBuffer now contains the full RDM6300 format frame

}
