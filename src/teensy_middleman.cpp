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


void setup() {
  Serial.begin(500000);

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

  for (int i = 0; i < 4; i++) {
    ODriveCAN* odrv = odrives[i];
    if (!odrv->request(vbus, 1000)) {
      Serial.print("vbus on ODrive ");
      Serial.print(i + 1);
      Serial.println(" request failed!");
      vbusErrorFlag = true;
    }
    Serial.print("ODrive ");
    Serial.print(i + 1);
    Serial.print(": DC voltage [V]: ");
    Serial.print(vbus.Bus_Voltage);
    Serial.print(" | DC current [A]: ");
    Serial.println(vbus.Bus_Current);
  }
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
 
  // Report stats every 2 ms
  if (now - lastReportTime >= 10*1000 && count > 0) {  // 2000 us = 2 ms
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



    // Read commands from serial in format "P1:100, V1:120, T1:50, ..."
    static float pos1 = NAN, pos2 = NAN, pos3 = NAN, pos4 = NAN;
    static float vel1 = NAN, vel2 = NAN, vel3 = NAN, vel4 = NAN;
    static float tor1 = NAN, tor2 = NAN, tor3 = NAN, tor4 = NAN;

    if (Serial.available()) {
      String input = Serial.readStringUntil('\n');
      input.trim();
      Serial.print("Echo input command from teensy: ");
      Serial.println(input);

      // Initialize all commands to NAN for this iteration
      pos1 = NAN; pos2 = NAN; pos3 = NAN; pos4 = NAN;
      vel1 = NAN; vel2 = NAN; vel3 = NAN; vel4 = NAN;
      tor1 = NAN; tor2 = NAN; tor3 = NAN; tor4 = NAN;
      
      // Parse the input string in format "P1:100, V2:120, T3:50, ..."
      int idx = 0;
      while (idx < (int)input.length()) {
        // Skip whitespace
        while (idx < (int)input.length() && (input[idx] == ' ' || input[idx] == ',')) {
          idx++;
        }
        
        if (idx >= (int)input.length()) break;
        
        // Extract command type (P, V, or T)
        char cmdType = input[idx];
        idx++;
        
        // Extract axis number (1-4)
        if (idx < (int)input.length() && input[idx] >= '1' && input[idx] <= '4') {
          int axis = input[idx] - '0';
          idx++;
          
          // Expect ':'
          if (idx < (int)input.length() && input[idx] == ':') {
            idx++;
            
            // Extract the value
            int endPos = input.indexOf(',', idx);
            if (endPos == -1) endPos = input.length();
            
            String valueStr = input.substring(idx, endPos);
            valueStr.trim();
            float value = valueStr.toFloat();
            
            // Store the command based on type and axis
            if (cmdType == 'P' || cmdType == 'p') {
              if (axis == 1) pos1 = value;
              else if (axis == 2) pos2 = value;
              else if (axis == 3) pos3 = value;
              else if (axis == 4) pos4 = value;
            } else if (cmdType == 'V' || cmdType == 'v') {
              if (axis == 1) vel1 = value;
              else if (axis == 2) vel2 = value;
              else if (axis == 3) vel3 = value;
              else if (axis == 4) vel4 = value;
            } else if (cmdType == 'T' || cmdType == 't') {
              if (axis == 1) tor1 = value;
              else if (axis == 2) tor2 = value;
              else if (axis == 3) tor3 = value;
              else if (axis == 4) tor4 = value;
            }
            
            idx = endPos;
          } else {
            idx++; // Skip invalid character
          }
        } else {
          idx++; // Skip invalid character
        }
      }
    }

    // Send commands to ODrives
    // If position is specified, use setPosition; otherwise if velocity is specified, use setVelocity; otherwise use torque
    if (!isnan(pos1)) { // first priority is position control
      if (!isnan(vel1)) {
        odrv1.setPosition(pos1, vel1); //optinal use velocity as feedforward command
      } else {
        odrv1.setPosition(pos1);
      }
    } else if (!isnan(vel1)) { // second priority is velocity control
      if (!isnan(tor1)) {
        odrv1.setVelocity(vel1, tor1); // optional use torque as feedforward command
      } else {
        odrv1.setVelocity(vel1);
      }
    } else if (!isnan(tor1)) { // Third priority is torque control
      odrv1.setTorque(tor1);  
    }

    if (!isnan(pos2)) { // first priority is position control
      if (!isnan(vel2)) {
        odrv2.setPosition(pos2, vel2); //optinal use velocity as feedforward command
      } else {
        odrv2.setPosition(pos2);
      }
    } else if (!isnan(vel2)) { // second priority is velocity control
      if (!isnan(tor2)) {
        odrv2.setVelocity(vel2, tor2); // optional use torque as feedforward command
      } else {
        odrv2.setVelocity(vel2);
      }
    } else if (!isnan(tor2)) { // Third priority is torque control
      odrv2.setTorque(tor2);  
    }

    if (!isnan(pos3)) { // first priority is position control
      if (!isnan(vel3)) {
        odrv3.setPosition(pos3, vel3); //optinal use velocity as feedforward command
      } else {
        odrv3.setPosition(pos3);
      }
    } else if (!isnan(vel3)) { // second priority is velocity control
      if (!isnan(tor3)) {
        odrv3.setVelocity(vel3, tor3); // optional use torque as feedforward command
      } else {
        odrv3.setVelocity(vel3);
      }
    } else if (!isnan(tor3)) { // Third priority is torque control
      odrv3.setTorque(tor3);  
    }

    if (!isnan(pos4)) { // first priority is position control
      if (!isnan(vel4)) {
        odrv4.setPosition(pos4, vel4); //optinal use velocity as feedforward command
      } else {
        odrv4.setPosition(pos4);
      }
    } else if (!isnan(vel4)) { // second priority is velocity control
      if (!isnan(tor4)) {
        odrv4.setVelocity(vel4, tor4); // optional use torque as feedforward command
      } else {
        odrv4.setVelocity(vel4);
      }
    } else if (!isnan(tor4)) { // Third priority is torque control
      odrv4.setTorque(tor4);  
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