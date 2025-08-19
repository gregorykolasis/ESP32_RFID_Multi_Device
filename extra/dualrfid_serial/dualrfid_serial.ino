/*
 * Example on how to use the Wiegand reader library with interruptions.
 */

#include <Wiegand.h>
#include <ArduinoJson.h>
#include <FastLED.h>

#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// These are the pins connected to the Wiegand D0 and D1 signals.
#define PIN_D0 D1
#define PIN_D1 D2
#define COIN    D3
#define BUZZER  D5
#define LED_PIN D4

#define DEVICE_ID "dualrfid"
int device_nr = 0;



long lastReconnectAttempt = 0;
String clientId = "";

// The object that handles the wiegand protocol
Wiegand wiegand;


// COIN STUFF
long creditTime = 0;
bool checked = true;
volatile float coinsValue = 0.0;
//Set the coinsValue to a Volatile float
//Volatile as this variable changes any time the Interrupt is triggered
int coinsChange = 0;
//A Coin has been inserted flag
bool pulse = false;
bool pulseDone = false;





// Initialize Wiegand reader
void setup() {
  pinMode(BUZZER,OUTPUT);
  digitalWrite(BUZZER,HIGH);
  delay(1000);
  digitalWrite(BUZZER,LOW);
  Serial.begin(115200);
    
  //Install listeners and initialize Wiegand reader
  wiegand.onReceive(receivedData, "Card read: ");
  wiegand.onReceiveError(receivedDataError, "Card read error: ");
  wiegand.onStateChange(stateChanged, "State changed: ");
  wiegand.begin(Wiegand::LENGTH_ANY, true);

  //initialize pins as INPUT
  pinMode(PIN_D0, INPUT);
  pinMode(PIN_D1, INPUT);

  pinMode(COIN, INPUT_PULLUP);
  pinMode(BUZZER,OUTPUT);
  digitalWrite(BUZZER,LOW);
    
}

// Continuously checks for pending messages and polls updates from the wiegand inputs
void loop() {
  // Checks for pending messages
  wiegand.flush();

  // Check for changes on the the wiegand input pins
  wiegand.setPin0State(digitalRead(PIN_D0));
  wiegand.setPin1State(digitalRead(PIN_D1));

  if(!pulse && !digitalRead(COIN))
  {
    pulse = true;
    creditTime = millis();
    
  }
  else if(!pulseDone && pulse && !digitalRead(COIN) && creditTime > 300)
  {
    coinInserted();
    pulseDone = true;
  }
  else if(pulse && digitalRead(COIN))
  {
    delay(5);
    if(digitalRead(COIN))
    {
      pulse = false;
      pulseDone = false;
    }
    
  }
  
  if (coinsChange == 1)
    //Check if a coin has been Inserted
  {
    coinsChange = 0;
    //unflag that a coin has been inserted

    //Serial.print("Credit: €");
    //Serial.println(coinsValue);
    //Print the Value of coins inserted
  }
  if(!pulse && !checked)
  {
    //Serial.println("check");
    checked = true;
    if(coinsValue == 1)
    {
      newCredit(1);
      coinsValue=0;
    }
    else if(coinsValue == 2)
    {
      newCredit(2);
      coinsValue-=2;
    }
  }
}

// Notifies when a reader has been connected or disconnected.
// Instead of a message, the seconds parameter can be anything you want -- Whatever you specify on `wiegand.onStateChange()`
void stateChanged(bool plugged, const char* message) {
    //Serial.print(message);
    //Serial.println(plugged ? "CONNECTED" : "DISCONNECTED");
}

// Notifies when a card was read.
// Instead of a message, the seconds parameter can be anything you want -- Whatever you specify on `wiegand.onReceive()`
void receivedData(uint8_t* data, uint8_t bits, const char* message) {
    
    //Print value in HEX
    uint8_t bytes = (bits+7)/8;
    String code = "";
    for (int i=0; i<bytes; i++) {
        code=(code+String(data[i] >> 4, 16));
        code=(code+String(data[i] & 0xF, 16));
    }

    StaticJsonDocument<512> out;
    out["status"] = "card_detected";
    out["id"] = code;
    serializeJson(out, Serial);
    Serial.println();

    digitalWrite(BUZZER,HIGH);
    delay(100);
    digitalWrite(BUZZER,LOW);
}
// Notifies when an invalid transmission is detected
void receivedDataError(Wiegand::DataError error, uint8_t* rawData, uint8_t rawBits, const char* message) {
    
}

void coinInserted()
//The function that is called every time it recieves a pulse
{
  coinsValue = coinsValue + 1;
  //As we set the Pulse to represent 5p or 5c we add this to the coinsValue
  //Serial.println(coinsValue);
  coinsChange = 1;
  //Flag that there has been a coin inserted
  //pinMode(D2, INPUT);
  checked = false;
  
}

void newCredit(int credit)
{
  StaticJsonDocument<512> out;
  char buff[512];
  out["status"] = "newCredit";
  out["credits"] = credit;
  
  serializeJson(out, Serial);
  Serial.println();
}
