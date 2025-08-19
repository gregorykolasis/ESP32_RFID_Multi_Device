#include <Arduino.h>
#include "driver/rmt.h"

// --- Hardware Pin Definitions (from your schematic) ---
#define CLOCK_125KHZ     25    // In writer mode, this pin outputs the Manchester encoded signal
#define DEMOD_125KHZ     26    // Used for reading (unused during write operation)
#define LED_125KHZ       14    // LED indicator (e.g., active during writing)

// --- Writer Settings ---
#define CLK_DIV          80    // Divider for RMT clock (80 MHz / 80 = 1 µs per tick)
#define BIT_DURATION     400   // Bit period in microseconds (adjust to your tag's protocol)
#define TX_CHANNEL       RMT_CHANNEL_0

// --- Manchester Encoding Function ---
// Converts a single bit into an RMT item. For EM4305‑style write (Manchester encoding):
//   - Logical 1: output high for the first half of BIT_DURATION, then low.
//   - Logical 0: output low first, then high.
rmt_item32_t manchesterEncodeBit(bool bit) {
  rmt_item32_t item;
  uint16_t halfTicks = BIT_DURATION / 2;
  if (bit) {
    item.level0 = 1;
    item.duration0 = halfTicks;
    item.level1 = 0;
    item.duration1 = halfTicks;
  } else {
    item.level0 = 0;
    item.duration0 = halfTicks;
    item.level1 = 1;
    item.duration1 = halfTicks;
  }
  return item;
}

// --- Function to Send Manchester-Encoded Data via RMT ---
// The data is sent MSB first; each byte is converted into 8 Manchester encoded RMT items.
void sendManchesterData(const uint8_t *data, size_t len) {
  size_t numItems = len * 8; // Each byte requires 8 RMT items (one per bit)
  rmt_item32_t *items = (rmt_item32_t *) malloc(numItems * sizeof(rmt_item32_t));
  if (!items) {
    Serial.println("Memory allocation for RMT items failed!");
    return;
  }
  
  size_t index = 0;
  for (size_t i = 0; i < len; i++) {
    // Process each bit (from MSB to LSB)
    for (int bit = 7; bit >= 0; bit--) {
      bool bitVal = ((data[i] >> bit) & 0x01) ? true : false;
      items[index++] = manchesterEncodeBit(bitVal);
    }
  }
  
  // Transmit the RMT items; this will output the Manchester encoded signal on CLOCK_125KHZ.
  rmt_write_items(TX_CHANNEL, items, numItems, true);
  rmt_wait_tx_done(TX_CHANNEL, portMAX_DELAY);
  
  free(items);
}

// --- RMT Transmitter Initialization ---
// Configures the RMT peripheral for transmitting on the CLOCK_125KHZ pin.
void setupRMTWriter() {
  rmt_config_t rmtTx;
  rmtTx.channel = TX_CHANNEL;
  rmtTx.gpio_num = (gpio_num_t)CLOCK_125KHZ;
  rmtTx.clk_div = CLK_DIV;
  rmtTx.mem_block_num = 1;
  rmtTx.rmt_mode = RMT_MODE_TX;
  
  // Transmitter configuration.
  // In this configuration, carrier generation is set to 0 because the external RF front‑end must impose a 125 kHz carrier.
  rmtTx.tx_config.loop_en = false;
  rmtTx.tx_config.carrier_freq_hz = 125000;
  rmtTx.tx_config.carrier_level = RMT_CARRIER_LEVEL_LOW;
  rmtTx.tx_config.carrier_duty_percent = 50;
  rmtTx.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
  rmtTx.tx_config.idle_output_en = true;
  
  rmt_config(&rmtTx);
  rmt_driver_install(rmtTx.channel, 0, 0);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting RFID Writer (ESP32) using provided RDM6300-derived schematic");
  
  // --- Configure Indicator LED ---
  pinMode(LED_125KHZ, OUTPUT);
  digitalWrite(LED_125KHZ, HIGH);  // Turn LED on to indicate write mode

  // --- Initialize RMT for writer output ---
  setupRMTWriter();
  
  // Allow hardware to stabilize; adjust delay as necessary.
  delay(500);
  
  // --- Build the Write Command Frame ---
  // This command frame is an example; many 125 kHz tags (like EM4305) require a frame structure
  // that includes a preamble, a start bit, an opcode, address pointer, a 32-bit data block, etc.
  // Adjust the frame below as needed based on your tag’s protocol.
  uint8_t commandFrame[8] = {
    0xFF, 0xE0,   // Example preamble bytes
    0xC0,         // Example write opcode
    0x02,         // Example address pointer
    0xDE, 0xAD, 0xBE, 0xEF  // Example 32-bit data payload
  };
  
  Serial.println("Sending write command...");
  sendManchesterData(commandFrame, sizeof(commandFrame));
  Serial.println("Write command sent.");
}

void loop() {
  // You can add logic here to repeat the write operation, trigger it via a button, or any other event.
}
