import serial
import time
import math
import re

# Configuration
SERIAL_PORT = 'COM17'  # Change this to match your Teensy's serial port
BAUD_RATE = 500000
UPDATE_RATE = 0.002  # Send commands every 2ms (500 Hz)

def send_motor_commands(ser, P1=None, P2=None, P3=None, P4=None,
                        V1=None, V2=None, V3=None, V4=None,
                        T1=None, T2=None, T3=None, T4=None):
    """
    Send motor commands to ODrives via serial.
    
    Parameters:
    - ser: Serial port object
    - P1-P4: Position commands for ODrives 1-4
    - V1-V4: Velocity commands for ODrives 1-4
    - T1-T4: Torque commands for ODrives 1-4
    
    All provided commands are sent in order: P, V, T for each axis.
    """
    
    # Dictionary to store all commands per axis
    commands = {
        1: {'P': P1, 'V': V1, 'T': T1},
        2: {'P': P2, 'V': V2, 'T': T2},
        3: {'P': P3, 'V': V3, 'T': T3},
        4: {'P': P4, 'V': V4, 'T': T4},
    }
    
    command_string = ""
    
    for axis in range(1, 5):
        axis_commands = commands[axis]
        
        # Add commands in priority order: P, V, T
        for cmd_type in ['P', 'V', 'T']:
            if axis_commands[cmd_type] is not None:
                if command_string:
                    command_string += ", "
                command_string += f"{cmd_type}{axis}:{axis_commands[cmd_type]:.2f}"
    
    if command_string:
        print("Command sent from python:" + command_string)
        command_string += " <<<"
        ser.write((command_string + '\n').encode())



def parse_odrive_messages(ser):
    successfulParse = False
    if ser.in_waiting > 0:  # Check if there is data available to read
        line = ser.readline().decode('utf-8').strip()  # Read the line and decode it
        if line:
            print(f"Received line from teensy: {line}")

            """Parses OD values from a given line of text and returns them as a dictionary."""
            pattern = r"OD(\d):\s*(-?\d+\.\d+)"
            matches = re.findall(pattern, line)
            if len(matches) > 0:
                # Convert matches to a dictionary with ODx as the key
                odrive_values = {f"OD{match[0]}": float(match[1]) for match in matches}
                print(f"Parsed ODrive states: {odrive_values}")
                successfulParse = True
    if successfulParse:
        return odrive_values
    else:
        return None



        



# Sine wave parameters
FREQUENCY = 0.5  # Hz
AMPLITUDE1 = .2  # Velocity amplitude for ODrive 1
AMPLITUDE2 = .4  # Velocity amplitude for ODrive 2
AMPLITUDE3 = .6  # Velocity amplitude for ODrive 3
AMPLITUDE4 = .8  # Velocity amplitude for ODrive 4



def main():
    fb0 = None
    try:
        # Open serial connection
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        print(f"Connected to {SERIAL_PORT} at {BAUD_RATE} baud")

        while fb0 is None:
            fb0 = parse_odrive_messages(ser)

        start_time = time.time()
        while True:
            current_time = time.time() - start_time
            
            while parse_odrive_messages(ser) is not None:
                pass
        
            
            homingPeriod = 2/FREQUENCY
            if (current_time<homingPeriod):
                pos1 = fb0.get("OD1", 0)*(homingPeriod - current_time)/homingPeriod
                pos2 = fb0.get("OD2", 0)*(homingPeriod - current_time)/homingPeriod
                pos3 = fb0.get("OD3", 0)*(homingPeriod - current_time)/homingPeriod
                pos4 = fb0.get("OD4", 0)*(homingPeriod - current_time)/homingPeriod
                print("Homing...")
                send_motor_commands(ser, P1=pos1, P2=pos2, P3=pos3, P4=pos4)
            else:
                print("sinusoidaling...")
                pos1 = AMPLITUDE1 * math.sin(2 * math.pi * FREQUENCY * current_time)
                vel1 = AMPLITUDE1 * 2 * math.pi * FREQUENCY * math.cos(2 * math.pi * FREQUENCY * current_time)
                
                pos2 = AMPLITUDE2 * math.sin(2 * math.pi * FREQUENCY * current_time)
                vel2 = AMPLITUDE2 * 2 * math.pi * FREQUENCY * math.cos(2 * math.pi * FREQUENCY * current_time)
                
                pos3 = AMPLITUDE3 * math.sin(2 * math.pi * FREQUENCY * current_time)
                vel3 = AMPLITUDE3 * 2 * math.pi * FREQUENCY * math.cos(2 * math.pi * FREQUENCY * current_time)
                
                pos4 = AMPLITUDE4 * math.sin(2 * math.pi * FREQUENCY * current_time)
                vel4 = AMPLITUDE4 * 2 * math.pi * FREQUENCY * math.cos(2 * math.pi * FREQUENCY * current_time)

                # Send position and velocity commands
                send_motor_commands(ser, P1=pos1, P2=pos2, P3=pos3, P4=pos4, V1=vel1, V2=vel2, V3=vel3, V4=vel4)

            # Wait before next update
            time.sleep(UPDATE_RATE)

    except KeyboardInterrupt:
        print("\nStopping...")
    except Exception as e:
        print(f"Error: {e}")
    finally:
        if 'ser' in locals():
            ser.close()
            print("Serial connection closed")

if __name__ == "__main__":
    main()