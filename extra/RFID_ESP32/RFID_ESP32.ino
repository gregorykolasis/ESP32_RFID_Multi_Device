#include <Arduino.h>

// Pin definitions
#define PWM_PIN 18        // Pin for the 125kHz square wave
#define SIGNAL_IN_PIN 19  // Pin to read the RFID signal
#define BUZZER_PIN 23     // Pin for the buzzer (will be removed in the next update)
#define TXD_PIN 17        // Pin for UART transmission

#define UP HIGH
#define DOWN LOW

// Constants
#define BAUDRATE 115200       // UART baud rate
#define ONE_BIT_DELAY (1000000 / BAUDRATE)
#define LEDC_CHANNEL 0      // PWM channel for 125kHz

// Variables
volatile bool signal_in = false;
volatile unsigned char pulseUpDown = LOW;
volatile unsigned int pulseCounter = 0;

bool parityStatus = false;
int startBitCounter,i;
int temp;

char dataBuffer[10]; // Buffer for RFID data

// ISR to handle incoming RFID signals
void IRAM_ATTR signalISR() {
    signal_in = true; // Set signal flag when an RFID signal is detected
    // Check if the signal is high or low and update pulseUpDown
    pulseUpDown = digitalRead(SIGNAL_IN_PIN);
}

// Initialize the 125kHz PWM
void init_PWM() {
    const int freq = 125000;   // 125 kHz
    const int resolution = 8;  // 8-bit resolution

    ledcSetup(LEDC_CHANNEL, freq, resolution);  // Set up the PWM channel
    ledcAttachPin(PWM_PIN, LEDC_CHANNEL);       // Attach the PWM to the specified pin
    ledcWrite(LEDC_CHANNEL, 128);               // Set duty cycle to 50% (128 out of 255)
}

void _delay_us(int microSeconds) {
    delayMicroseconds(microSeconds);  
}

// Main RFID tag serial number reading logic
bool readTagSerialNumber (void)
{
  int i,k,m,pulseUpDownBackup,ones,parity;
  /* Bytes 6,7,8 and 9 are used to store the sum of parity bits of the four RFID data columns. */
  dataBuffer[6]=0; /* Clear the parity buffer */ 
  dataBuffer[7]=0;
  dataBuffer[8]=0;
  dataBuffer[9]=0;
  
  parity = true;
  for(i=0;i<11;i++) /* Repeat 10 times the 5-bit lines of RFID data.*/
  {
    ones = 0; /* Clear the ones buffer. */
    for(k=0;k<4;k++)
    {
      _delay_us(1);
      m=0;
      pulseUpDownBackup = pulseUpDown; //Get a backup the current status of pulseUpDown (see ISR(TIM0_OVF_vect) interrupt for this variable). 
      
      _delay_us(1);
      
      dataBuffer[i/2] <<= 1; //Shift one bit to the left the dataBuffer[i/2] byte.
      if(pulseUpDown == UP)  //If level is up (Logic HIGH) then,
      {
        dataBuffer[i/2] |= (1<<0);  //Make logic one the bit 0, 
        ones++;                     //Increase the line one's,  
        dataBuffer[k+6]++;          //and increase the column one's.
      }
      
      _delay_us(1);
      
      while(m<7) //Delay some time (one bit length).
      {
        _delay_us(1);
        m++;
        if(pulseUpDownBackup != pulseUpDown) //If in the meantime we have a change to the pulseUpDown status, break the delay.
          break;
      }
    }
    //Go get the status of the fifth bit (parity bit).
    _delay_us(1); 
    
    m=0;
    pulseUpDownBackup = pulseUpDown; //Get a new backup the current status of pulseUpDown
    
    _delay_us(1);
    
    if(pulseUpDown == UP)
      ones++;
      
    ones %= 2; //See if parity byte is Even.
    if((ones != 0)&&(i < 10)) //Exclude the 11th 4-bit array (column parity).
      parity = false; //If parity bit is odd, then mark it as FALSE.
       
    _delay_us(1);
    
    while(m<7) //Delay some time (one bit length).
    {
      _delay_us(1);
      m++;
      if(pulseUpDownBackup != pulseUpDown)
        break;
    }
  }

/* Check the column parity bits (4 columns). */
  if(((dataBuffer[6] % 2) != 0)||
    ((dataBuffer[7] % 2) != 0)||
    ((dataBuffer[8] % 2) != 0)||
    ((dataBuffer[9] % 2) != 0))
    parity = false;

  return parity ; //Return the parity status (TRUE: if parity is OK, FALSE: is parity is not OK).
}

void setup() {
    // Initialize serial communication for debugging
    Serial.begin(115200);

    // Pin setup
    pinMode(SIGNAL_IN_PIN, INPUT);   // RFID signal input
    pinMode(BUZZER_PIN, OUTPUT);     // Buzzer output (will be removed in the next update)
    pinMode(TXD_PIN, OUTPUT);        // UART TX pin

    // Initialize 125kHz PWM
    init_PWM();

    // Attach interrupt for RFID signal detection
    attachInterrupt(digitalPinToInterrupt(SIGNAL_IN_PIN), signalISR, CHANGE);

    Serial.println("System initialized and waiting for RFID tag...");
}

void loop() {


    while(pulseUpDown == UP) //Wait here until you find a steady Logic ONE. This represents the 9 start bits of RFID tag.
    {
      startBitCounter++;
      _delay_us(8);
    }
    if(startBitCounter >= 12) // If the length of the Logic ONE pulse is ok then, 
    {
      parityStatus = readTagSerialNumber(); //Read the RFID tag serial number and get the parity status.
      if(parityStatus == true) //If parity is ok,
      {
        temp = 0;
        for(i=1;i<5;i++) //Get the 32-bit serial number of the Tag.
        temp += dataBuffer[i]; //Add to temp the sum of the 4-byte serial number.
        if(temp > 0) //If serial number is 0 then serial number was probably produced by electrical noise. Not from an RFID tag.
        {
          Serial.println("Correct TAG");
        }
      }
      else {
        Serial.println("Bad TAG");
        for 
      }
      startBitCounter = 0;
    }
    if(pulseUpDown == DOWN)
      startBitCounter = 0;






}
