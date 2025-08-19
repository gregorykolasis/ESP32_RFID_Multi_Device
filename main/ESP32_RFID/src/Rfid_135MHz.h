#ifndef RFID_135MHZ_H
#define RFID_135MHZ_H

#include <Adafruit_PN532.h>

enum READ_TYPES_RFID_READER_135MHZ : uint8_t{
	NFC_SIMPLE_4BYTE_READ = 1,
	NFC_SIMPLE_3BYTE_READ,
	NFC_MAZE_MIFARE
};

class Rfid_135MHz {
public:
    // Constructor
    Rfid_135MHz(uint8_t ss_pin, uint8_t irq_pin, uint8_t led_pin);

    // Initialize the NFC reader
    bool begin();

    // Start listening for an NFC card
    void startListening();

    // Enable the RF field
    void enableRFField();

    // Disable the RF field
    void disableRFField();

    // Power down the PN532
    void powerDown();

    // Wake up and reinitialize the PN532
    void wakeUp();

    // Handle the card detection
    void handleCardDetected();
	
    // Function to extract UUID
    void extractUUID(uint8_t* uid);

    // Function to extract dump from the card
    void extractDump(uint8_t* uid, uint8_t uidLength, uint8_t &success);

    // Function to print tag string
    void tagStr(uint8_t* uid, uint8_t uidLength);	
	
    // Check IRQ and handle card detection if IRQ is triggered
    bool loop();

    // Get the version of the PN532 chip
    bool getFirmwareVersion();

    bool dbgVerbose;
    bool dbgNormal;
	
	uint8_t readType = NFC_SIMPLE_3BYTE_READ;
	uint8_t tagStrdecodedBytesLength = 3;
	
	int delayBetweenCards = 25;
	char currentTagStr[17];
	
	inline void setReadType(uint8_t type) {
		if (type==NFC_SIMPLE_3BYTE_READ) {
			readType = NFC_SIMPLE_3BYTE_READ;
			tagStrdecodedBytesLength = 3;
		}
		if (type==NFC_SIMPLE_4BYTE_READ) {
			readType = NFC_SIMPLE_4BYTE_READ;
			tagStrdecodedBytesLength = 4;
		}		
	}

private:
    Adafruit_PN532 nfc; // PN532 instance
    uint8_t ledPin;     // LED pin for 13.56 MHz indication
    uint8_t irqPin;     
    bool stateLed;      // LED state

    // Time tracking
    
    long timeLastCardRead;
    bool readerDisabled;
	
	void resetTag();
		
	uint32_t currentTag;
	bool tagIsHere = false;
    
    // IRQ management
    int irqCurr;
    int irqPrev;

    // Power-down status
    bool pn532PoweredDown;
};

#endif // RFID_135MHZ_H
