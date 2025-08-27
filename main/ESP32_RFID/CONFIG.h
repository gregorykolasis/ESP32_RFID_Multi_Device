#include "src/Rfid_135MHz.h"
#include "src/Rfid_125KHz.h"
#include <SPIFFS.h>

// Scanning Modes (only apply to 125KHz tag reader, not NFC)
enum ScanningMode : uint8_t {
  SIMPLE_SCANNING = 0,
  VERIFIED_SCANNING = 1
};

// Mode Configuration for 125KHz tag reader
#define MODE_CONFIG_FILE "/mode.txt"
#define VERIFIED_SCAN_TIME 2500  // 2.5 seconds for verified scanning
#define VERIFIED_SCAN_MIN_COUNT 10  // Minimum consecutive scans required

// Current scanning mode (125KHz only)
ScanningMode currentMode = SIMPLE_SCANNING;

// Verified scanning tracking variables (125KHz only)
uint32_t lastVerifiedTag125KHz = 0;
uint32_t verifiedStartTime125KHz = 0;
uint32_t consecutiveScans125KHz = 0;
bool led125KHzState = false;
bool led135MHzState = false;  // NFC LED always simple mode

// LED blink timing variables

bool ledStartupFinished = false;

bool smallBlinkVerificationFlag = false;
unsigned long smallBlinkVerificationTimer = 0;

unsigned long blinkStartTime = 0;
uint8_t blinkCount = 0;
uint8_t targetBlinks = 0;
bool blinkState = false;
uint8_t currentBlinkLed = 0;
#define BLINK_DURATION 200
#define BLINK_INTERVAL 200
#define LED_SEQUENCE_DELAY 500

#define PN532_SS         5
#define PN532_IRQ        13
#define LED_135MHZ       15

#define CLOCK_125KHZ     25
#define DEMOD_125KHZ     26
#define LED_125KHZ       14
#define ENCODE_EM4100    false
#define FREQUENCY_125KHZ 125.0

bool dbgRdm6300 = false;
#define RDM6300_BAUDRATE   9600

//DEL
//#define RDM6300_RX_PIN   18
//#define RDM6300_TX_PIN   21       //is 10 on Test , Will be 18 in Production , 10 is Broken on Vlasis will put it to 

//VLASIS
#define RDM6300_RX_PIN   23
#define RDM6300_TX_PIN   18 

#define RDM6300_SEND_EVERY 25

#define BUTTON_PIN 0  // GPIO 0 for BOOT button
const uint8_t I2C_ADDR_PINS[] = {34, 35, 36, 39};  // LSB (34) to MSB (39)

Rfid_135MHz nfc(PN532_SS, PN532_IRQ, LED_135MHZ);
Rfid_125KHz tag(FREQUENCY_125KHZ,DEMOD_125KHZ,CLOCK_125KHZ,LED_125KHZ,ENCODE_EM4100);

char tag125KHzStr[17];
char tag135MHzStr[17];
uint32_t tag125KHzNumber;
uint32_t tag135MHzNumber;

bool nfcReady = false;
bool tagReady = false;

void dummmyI2C();
void receiveEventI2C(int howMany);
void requestEventi2C();
void setupSlaveI2C();
void readAddrI2C();
void prepareJsonI2C();

#include "I2C-Slave.h"

#if defined(ESP32)
  #define USE_SPIFFS            true
  #define ESP_DRD_USE_EEPROM    true
#else
  #error This code is intended to run on the ESP32 platform! Please check your Tools->Board setting.
#endif
#define DRD_TIMEOUT             2
#define DRD_ADDRESS             0
#include <ESP_DoubleResetDetector.h>            //https://github.com/khoih-prog/ESP_DoubleResetDetector
DoubleResetDetector* drd;
