/******************** Function declarations *******************/
#include <Arduino.h>

// global app specific functions
void rfidSetup();
void rfidRead();
/******************** Global app declarations *******************/

extern const char* appConfig;

extern int rfidDemod; // input - demod pin from RDM6300, pullup high
extern int rfidClock; // output - clock pin to RDM6300
extern int clearPin; // input - clear tag display
extern float rfidFreq; // antenna frequency in kHz
extern bool encodeFDX; // false for EM4100, true for FXD-B
extern char encodingStr[]; // encoding type string
extern uint64_t currentTag;
extern char currentTagStr[]; // holds string representation of tag
