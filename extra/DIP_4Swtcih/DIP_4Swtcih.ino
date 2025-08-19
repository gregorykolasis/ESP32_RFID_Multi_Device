// Define the GPIO pins for DIP switch input in an array
int ADDR_PINS[] = {34, 35, 36, 39};  // LSB (34) to MSB (39)
int num_pins = sizeof(ADDR_PINS) / sizeof(ADDR_PINS[0]);  // Number of pins

void setup() {
  // Start the serial communication
  Serial.begin(115200);

  // Set each GPIO pin in ADDR_PINS as input
  for (int i = 0; i < num_pins; i++) {
    pinMode(ADDR_PINS[i], INPUT);  // External pull-up/pull-down resistor
  }
}

void loop() {
  int address = 0;

  // Read the states of the DIP switch pins
  for (int i = 0; i < num_pins; i++) {
    int bit_value = digitalRead(ADDR_PINS[i]);  // Read each pin state
    address |= (bit_value << i);  // Shift the bit according to its position (LSB first)
  }

  // Print the address in binary format
  Serial.print("DIP Switch Address (Binary): ");
  for (int i = num_pins - 1; i >= 0; i--) {  // Print MSB first
    Serial.print(digitalRead(ADDR_PINS[i]));
  }
  Serial.println();

  // Optionally, print the address in decimal for reference
  Serial.print("DIP Switch Address (Decimal): ");
  Serial.println(address);

  // Small delay to avoid flooding the serial monitor
  delay(500);
}
