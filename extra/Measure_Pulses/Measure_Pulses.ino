
// Pin definitions
#define PWM_PIN 18        // Pin for the 125kHz square wave
#define SIGNAL_IN_PIN 19  // Pin to read the RFID signal

// Variables
volatile unsigned char pulseUpDown = LOW;

// Constants
#define BAUDRATE 115200       // UART baud rate

#define LEDC_CHANNEL 0      // PWM channel for 125kHz
#define MAX_BITS 64          // Number of bits to capture
#define THRESHOLD_HIGH 1500  // Threshold for HIGH signal in microseconds
#define THRESHOLD_LOW 500    // Threshold for LOW signal in microseconds


int indexer=0;
int dataBuffer[64]; 
bool printIt = false;
bool wait = false;

// Initialize the 125kHz PWM
void init_PWM() {
    const int freq = 125000;   // 125 kHz
    const int resolution = 8;  // 8-bit resolution

    ledcSetup(LEDC_CHANNEL, freq, resolution);  // Set up the PWM channel
    ledcAttachPin(PWM_PIN, LEDC_CHANNEL);       // Attach the PWM to the specified pin
    ledcWrite(LEDC_CHANNEL, 128);               // Set duty cycle to 50% (128 out of 255)
}

// ISR for handling signal transitions
void IRAM_ATTR signalISR() {
    pulseUpDown = digitalRead(SIGNAL_IN_PIN);  // Update pulse status (HIGH/LOW)
    if (!wait) dataBuffer[indexer++] = pulseUpDown;
    if (indexer>64) {
      printIt = true;  
      wait = true;
    }
}

void setup() {
    // Initialize serial communication for debugging
    Serial.begin(115200);

    // Pin setup
    pinMode(SIGNAL_IN_PIN, INPUT);   // RFID signal input

    // Attach interrupt for RFID signal detection
    attachInterrupt(digitalPinToInterrupt(SIGNAL_IN_PIN), signalISR, CHANGE);

    init_PWM();

    Serial.println("System initialized and waiting for RFID signal...");
}

void loop() {

  if (printIt) {
    Serial.print("[Data] ->");
    for (uint8_t i=0;i<64;i++){
      Serial.print(dataBuffer[i]);
    } 
    Serial.println();
    printIt=false; 
    wait = false;
    indexer = 0;
  }
}
