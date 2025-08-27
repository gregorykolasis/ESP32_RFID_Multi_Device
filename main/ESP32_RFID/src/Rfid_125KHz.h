#ifndef RFID_125KHZ_H
#define RFID_125KHZ_H

#include <Arduino.h>

class Rfid_125KHz {
public:
    // Callback function types
    typedef void (*TagCallback)(uint32_t tagNumber);
    typedef void (*TagCallbackChange)(uint32_t tagNumber, uint32_t oldTagNumber);
    
    // Constructor to initialize the class with specific settings
    Rfid_125KHz(float frequency, int demod_pin, int clock_pin, int led_pin , bool modulationType);

    bool dbgVerbose;
    bool dbgNormal;
	char currentTagStr[17];

     uint32_t consecutiveSameTagCount = 0;
    
    // Callback functions
    TagCallback onInsert = nullptr;
    TagCallback onRemove = nullptr;
    TagCallbackChange onChange = nullptr;

    // Public methods
    void setup();
    bool loop();
    void createRDM6300Frame(uint32_t tagNumber, char* frameBuffer, size_t frameBufferSize);

    static void IRAM_ATTR demodISR();
    
    uint32_t getCurrentValue() {
       return (uint32_t)currentTag;
    }

    
private:
    // Private attributes
	int ledPin    = -1;
    int rfidDemod = -1;
    int rfidClock = -1;
    float rfidFreq;
    bool encodeFDX;
    uint64_t currentTag;
    uint64_t previousTag = 0;  // Track previous tag for change detection
    char encodingStr[17];
    
    bool buttonPressed;
    
    bool simulation = false;
    void demodSim();

   

    uint32_t microsConsiderTagIsOut = 150e3; //ms
	unsigned long validRfidTimeout = 1e3; //ms
	bool useValidRfidTimeout = true;
    /* 
     * From the Measurements that i take when Rfid is ON it gives interrupts on 560uS when rfid is in Perfect position
     * so in Perfect position 1000uS , could be perfect to consider an RFID Tag that left
     * I put 5000uS so i can be perfectly safe on every position , which might not be perfect
     */

    volatile uint32_t lastMicrosInterrupt = 0;
	
	
	unsigned long lastValidRead = 0;
	
    // Buffer and state variables
    static const uint16_t bitBufferSize = 256;
    static const char* codeStr[2];

    void demod(); // The actual processing method
    uint16_t slBoundary, minPulse, maxPulse;
    int codeLength, captureSize;
    const int minLength = 64;
    int captureIndex;
    uint8_t columnSums[4];
    uint8_t bitBuffer[bitBufferSize];
    volatile bool captured;
    int headerPos;
    uint16_t pulses[bitBufferSize * 2];
    int pulseIndex;
    uint32_t simPulse;

    // Private methods
    void runClock();
    void showRawData(bool haveTag);
    bool findHeader();
    bool parseBitBufferEM(uint8_t currIndex);
    bool parseBitBufferFDX(uint8_t currIndex);
    bool parseNibble(word offset, uint8_t *nibble);
    bool getTag(bool retry);
    void tagStr();

    
    // Static instance pointer for ISR access
    static Rfid_125KHz* instance;
};

#endif // RFID_125KHZ_H
