#include <FlexCAN_T4.h>

FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> can1;  // Create CAN1 object

unsigned long previousWriteTime = 0;      // Store the last write time
const unsigned long writeInterval = 100;  // Write interval in milliseconds

void setup() {
  Serial.begin(115200);  // Initialize serial communication
  while (!Serial)
    ;  // Wait for serial port to connect

  can1.begin();
  can1.setBaudRate(250000);
  Serial.println("CAN initialized at 250000 bps");
}

void loop() {
  // Sending a CAN message
  if (millis() - previousWriteTime >= writeInterval) {
    CAN_message_t txMsg;
    txMsg.id = 0x123;     // Message ID
    txMsg.len = 1;        // Data length
    txMsg.buf[0] = 0x01;  // Data byte 1

    if (can1.write(txMsg)) {
      Serial.println("Message sent successfully");
    } else {
      Serial.println("Failed to send message");
    }
    previousWriteTime = millis();
  }

  // Reading a CAN message
  CAN_message_t rxMsg;
  if (can1.read(rxMsg)) {
    Serial.print("Received Message ID: ");
    Serial.print(rxMsg.id, HEX);
    Serial.print(", Data: ");
    for (uint8_t i = 0; i < rxMsg.len; i++) {
      Serial.print(rxMsg.buf[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
  }

  // delay(1000); // Wait for a second before the next loop
}