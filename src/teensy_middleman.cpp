#include <Arduino.h>
#include "ODriveCAN.h"

// Documentation for this example can be found here:
// https://docs.odriverobotics.com/v/latest/guides/arduino-can-guide.html


/* Configuration of example sketch -------------------------------------------*/

// CAN bus baudrate. Make sure this matches for every device on the bus
#define CAN_BAUDRATE 1000000

// ODrive node_ids
#define ODRV1_NODE_ID 1
#define ODRV2_NODE_ID 2
#define ODRV3_NODE_ID 3
#define ODRV4_NODE_ID 4

// See https://github.com/tonton81/FlexCAN_T4
// clone https://github.com/tonton81/FlexCAN_T4.git into /src
#include <FlexCAN_T4.h>
#include "ODriveFlexCAN.hpp"
struct ODriveStatus; // hack to prevent teensy compile error

// Function prototypes
void onHeartbeat(Heartbeat_msg_t& msg, void* user_data);
void onFeedback(Get_Encoder_Estimates_msg_t& msg, void* user_data);
void onCanMessage(const CanMsg& msg);

FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_32> can_intf;

bool setupCan() {
  can_intf.begin();
  can_intf.setBaudRate(CAN_BAUDRATE);
  can_intf.setMaxMB(16);
  can_intf.enableFIFO();
  can_intf.enableFIFOInterrupt();
  can_intf.onReceive(onCanMessage);
  return true;
}

/* Example sketch ------------------------------------------------------------*/

// Instantiate ODrive objects
ODriveCAN odrv1(wrap_can_intf(can_intf), ODRV1_NODE_ID); // Standard CAN message ID
ODriveCAN odrv2(wrap_can_intf(can_intf), ODRV2_NODE_ID); 
ODriveCAN odrv3(wrap_can_intf(can_intf), ODRV3_NODE_ID); 
ODriveCAN odrv4(wrap_can_intf(can_intf), ODRV4_NODE_ID); 
ODriveCAN* odrives[] = {&odrv1, &odrv2, &odrv3, &odrv4}; // Make sure all ODriveCAN instances are accounted for here

struct ODriveUserData {
  Heartbeat_msg_t last_heartbeat;
  bool received_heartbeat = false;
  Get_Encoder_Estimates_msg_t last_feedback;
  bool received_feedback = false;
};

// Keep some application-specific user data for every ODrive.
ODriveUserData odrv1_user_data;
ODriveUserData odrv2_user_data;
ODriveUserData odrv3_user_data;
ODriveUserData odrv4_user_data;

// Called every time a Heartbeat message arrives from the ODrive
void onHeartbeat(Heartbeat_msg_t& msg, void* user_data) {
  ODriveUserData* odrv_user_data  = static_cast<ODriveUserData*>(user_data);
  odrv_user_data->last_heartbeat = msg;
  odrv_user_data->received_heartbeat = true;
}

// Called every time a feedback message arrives from the ODrive
void onFeedback(Get_Encoder_Estimates_msg_t& msg, void* user_data) {
  ODriveUserData* odrv_user_data = static_cast<ODriveUserData*>(user_data);
  odrv_user_data->last_feedback = msg;
  odrv_user_data->received_feedback = true;
}

// Called for every message that arrives on the CAN bus
void onCanMessage(const CanMsg& msg) {
  for (auto odrive: odrives) {
    onReceive(msg, *odrive);
  }
}

float t0;
Get_Encoder_Estimates_msg_t feedback1;
Get_Encoder_Estimates_msg_t feedback2;
Get_Encoder_Estimates_msg_t feedback3;
Get_Encoder_Estimates_msg_t feedback4;

// SETUP()
//
// S     EEE   TTT   U   U PPP
// S    E   E   T   U   U P  P
// S    EEEE    T   U   U PPP
// S    E   E   T   U   U P
// SSS   EEE    T    UUU  P

void setup() {
  Serial.begin(115200);

  // Wait for up to 10 seconds for the serial port to be opened on the PC side.
  // If no PC connects, continue anyway.
  unsigned long startTime = millis();
  while (!Serial && millis() - startTime < 10000) {
    delay(100);
  }
  delay(200);


  Serial.println("Starting ODriveCAN demo with 4 ODrives");

  // Register callbacks for the heartbeat and encoder feedback messages
  odrv1.onFeedback(onFeedback, &odrv1_user_data);
  odrv2.onFeedback(onFeedback, &odrv2_user_data);
  odrv3.onFeedback(onFeedback, &odrv3_user_data);
  odrv4.onFeedback(onFeedback, &odrv4_user_data);

  odrv1.onStatus(onHeartbeat, &odrv1_user_data);
  odrv2.onStatus(onHeartbeat, &odrv2_user_data);
  odrv3.onStatus(onHeartbeat, &odrv3_user_data);
  odrv4.onStatus(onHeartbeat, &odrv4_user_data);
  

  // Configure and initialize the CAN bus interface.
  if (!setupCan()) {
    Serial.println("CAN failed to initialize: reset required");
    while (true) {delay(100);} // spin indefinitely
  }

  Serial.println("Waiting for ODrive 1 ...");
  while (!odrv1_user_data.received_heartbeat) {
    // pumpEvents(can_intf);
    delay(100);
  }

  Serial.println("Waiting for ODrive 2 ...");
  while (!odrv2_user_data.received_heartbeat) {
    // pumpEvents(can_intf);
    delay(100);
  }

  Serial.println("Waiting for ODrive 3 ...");
  while (!odrv3_user_data.received_heartbeat) {
    // pumpEvents(can_intf);
    delay(100);
  }

  Serial.println("Waiting for ODrive 4 ...");
  while (!odrv4_user_data.received_heartbeat) {
    // pumpEvents(can_intf);
    delay(100);
  }

  Serial.println("found all 4 ODrives");

  // request bus voltage and current (1sec timeout)
  Serial.println("attempting to read bus voltage and current");
  Get_Bus_Voltage_Current_msg_t vbus;
  bool vbusErrorFlag = false;

  if (!odrv1.request(vbus, 1000)) {
    Serial.println("vbus on ODrive 1 request failed!");
    vbusErrorFlag = true;
  }
  Serial.print("ODrive 1: DC voltage [V]: "); Serial.print(vbus.Bus_Voltage); Serial.print(" | DC current [A]: "); Serial.println(vbus.Bus_Current);

  if (!odrv2.request(vbus, 1000)) {
    Serial.println("vbus on ODrive 1 request failed!");
    vbusErrorFlag = true;
  }
  Serial.print("ODrive 2: DC voltage [V]: "); Serial.print(vbus.Bus_Voltage); Serial.print(" | DC current [A]: "); Serial.println(vbus.Bus_Current);

  if (!odrv3.request(vbus, 1000)) {
    Serial.println("vbus on ODrive 1 request failed!");
    vbusErrorFlag = true;
  }
  Serial.print("ODrive 3: DC voltage [V]: "); Serial.print(vbus.Bus_Voltage); Serial.print(" | DC current [A]: "); Serial.println(vbus.Bus_Current);

  if (!odrv4.request(vbus, 1000)) {
    Serial.println("vbus on ODrive 1 request failed!");
    vbusErrorFlag = true;
  }
  Serial.print("ODrive 4: DC voltage [V]: "); Serial.print(vbus.Bus_Voltage); Serial.print(" | DC current [A]: "); Serial.println(vbus.Bus_Current);

  // If there is an error with any of the bus voltages waits forever. 
  if (vbusErrorFlag) {
    Serial.println("Bus voltage issues");
    while (true) {delay(100);}
  }


  Serial.println("Enabling closed loop control...");
  for (int odrv_idx = 0; odrv_idx < 4; ++odrv_idx) {
    ODriveCAN* odrv = odrives[odrv_idx];
    ODriveUserData* user_data = nullptr;
    if (odrv_idx == 0) user_data = &odrv1_user_data;
    else if (odrv_idx == 1) user_data = &odrv2_user_data;
    else if (odrv_idx == 2) user_data = &odrv3_user_data;
    else if (odrv_idx == 3) user_data = &odrv4_user_data;

    int attempts = 0;
    const int max_attempts = 100;  // Limit attempts to avoid infinite loop
    bool success = false;

    while (attempts < max_attempts) {
      // Clear errors and set state
      odrv->clearErrors();
      delay(10);
      odrv->setState(ODriveAxisState::AXIS_STATE_CLOSED_LOOP_CONTROL);

      // Pump events frequently to process incoming heartbeats
      for (int i = 0; i < 20; ++i) {  // Increased pumping
        pumpEvents(can_intf);
        delay(5);  // Shorter delay for more frequent checks
      }

      // Check if heartbeat indicates closed loop
      if (user_data->received_heartbeat && 
          user_data->last_heartbeat.Axis_State == ODriveAxisState::AXIS_STATE_CLOSED_LOOP_CONTROL) {
        Serial.print("ODrive ");
        Serial.print(odrv_idx + 1);
        Serial.println(" is running");
        success = true;
        break;
      }

      attempts++;
    }

    if (!success) {
      Serial.print("Last status for ODrive ");
      Serial.print(odrv_idx + 1);
      Serial.print(": Heartbeat received: ");
      Serial.print(user_data->received_heartbeat ? "yes" : "no");
      if (user_data->received_heartbeat) {
        Serial.print(", Axis State: ");
        Serial.println(user_data->last_heartbeat.Axis_State);
      } else {
        Serial.println();
      }
      Serial.print("Failed to enable closed loop for ODrive ");
      Serial.print(odrv_idx + 1);
      Serial.println(" after max attempts");
    }
  }


  t0 = micros() * 1e-6;


  feedback1 = odrv1_user_data.last_feedback;
  feedback2 = odrv2_user_data.last_feedback;
  feedback3 = odrv3_user_data.last_feedback;
  feedback4 = odrv4_user_data.last_feedback;

} // end of setup()


//
// L     OOO   OOO   PPP
// L    O   O O   O  P  P
// L    O   O O   O  PPP
// L    O   O O   O  P
// LLLL  OOO   OOO   P
//

void loop() {
  static unsigned long lastLoopStart = 0;
  static unsigned long sumIntervals = 0;
  static unsigned long sumSquares = 0;
  static int count = 0;
  static unsigned long lastReportTime = 0;
  
  unsigned long now = micros();
  if (lastLoopStart != 0) {
    unsigned long loopInterval = now - lastLoopStart;
    sumIntervals += loopInterval;
    sumSquares += (unsigned long)loopInterval * loopInterval;
    count++;
  }
  lastLoopStart = now;
 
  // Report stats every 100 ms
  if (now - lastReportTime >= 100000 && count > 0) {  // 100000 us = 100 ms
    float mean = (float)sumIntervals / count;
    float variance = ((float)sumSquares / count) - (mean * mean);
    float std = sqrt(variance);
    Serial.print("Mean loop interval: ");
    Serial.print(mean);
    Serial.print(" us, Std: ");
    Serial.print(std);
    Serial.print(" us ");
    sumIntervals = 0;
    sumSquares = 0;
    count = 0;

    Serial.print("| OD1: "); Serial.print(feedback1.Pos_Estimate);
    Serial.print("| OD2: "); Serial.print(feedback2.Pos_Estimate);
    Serial.print("| OD3: "); Serial.print(feedback3.Pos_Estimate);
    Serial.print("| OD4: "); Serial.println(feedback4.Pos_Estimate);

    lastReportTime = now;
  }
 
  pumpEvents(can_intf); // This is required on some platforms to handle incoming feedback CAN messages
                        // Note that on MCP2515-based platforms, this will delay for a fixed 10ms.
                        //
                        // This has been found to reduce the number of dropped messages, however it can be removed
                        // for applications requiring loop times over 100Hz.

  float SINE_PERIOD = 2.0f; // Period of the position command sine wave in seconds

  float t = (now * 1e-6) - t0;
  
  float phase = t * (TWO_PI / SINE_PERIOD);



  float rampDuration = 2.0f;
  if (t<rampDuration){
    odrv1.setPosition(feedback1.Pos_Estimate*(rampDuration-t)/rampDuration);
    odrv2.setPosition(feedback2.Pos_Estimate*(rampDuration-t)/rampDuration);
    odrv3.setPosition(feedback3.Pos_Estimate*(rampDuration-t)/rampDuration);
    odrv4.setPosition(feedback4.Pos_Estimate*(rampDuration-t)/rampDuration);
  }
  else{
    odrv1.setPosition(0*sin(phase)); // second argument feedforward velocity is optional
    odrv2.setPosition(1*sin(phase)); // second argument feedforward velocity is optional
    odrv3.setPosition(1*sin(phase)); // second argument feedforward velocity is optional
    odrv4.setPosition(0*sin(phase)); // second argument feedforward velocity is optional
  }


  if (odrv1_user_data.received_feedback) {
    feedback1 = odrv1_user_data.last_feedback;
    odrv1_user_data.received_feedback = false;
  }

  if (odrv2_user_data.received_feedback) {
    feedback2 = odrv2_user_data.last_feedback;
    odrv2_user_data.received_feedback = false;
  }

  if (odrv3_user_data.received_feedback) {
    feedback3 = odrv3_user_data.last_feedback;
    odrv3_user_data.received_feedback = false;
  }

  if (odrv4_user_data.received_feedback) {
    feedback4 = odrv4_user_data.last_feedback;
    odrv4_user_data.received_feedback = false;
  }

} // end of loop()