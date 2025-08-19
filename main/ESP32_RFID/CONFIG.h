#include "src/Rfid_135MHz.h"
#include "src/Rfid_125KHz.h"

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
#define RDM6300_RX_PIN   18
#define RDM6300_TX_PIN   21       //is 10 on Test , Will be 18 in Production , 10 is Broken on Vlasis will put it to 

//VLASIS
//#define RDM6300_RX_PIN   23
//#define RDM6300_TX_PIN   18 

#define RDM6300_SEND_EVERY 25

#define BUTTON_PIN 0  // GPIO 0 for BOOT button
const uint8_t I2C_ADDR_PINS[] = {34, 35, 36, 39};  // LSB (34) to MSB (39)

Rfid_135MHz nfc(PN532_SS, PN532_IRQ, LED_135MHZ);
Rfid_125KHz tag(FREQUENCY_125KHZ,DEMOD_125KHZ,CLOCK_125KHZ,LED_125KHZ,ENCODE_EM4100);

char tag125KHzStr[17];
char tag135MHzStr[17];
uint32_t tag125KHzNumber;
uint32_t tag135MHzNumber;

void dummmyI2C();
void receiveEventI2C(int howMany);
void requestEventi2C();
void setupSlaveI2C();
void readAddrI2C();
void prepareJsonI2C();

#include "I2C-Slave.h"
