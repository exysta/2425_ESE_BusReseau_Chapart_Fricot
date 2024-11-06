#file to read data from the serial port on raspberry pi
import serial

# Configure the serial port
ser = serial.Serial('/dev/serial0', baudrate=115200, timeout=10)

try:
    while True:
        # Read a line from the serial port
        line = ser.readline().decode('utf-8').rstrip()
        if line:
            print(f"Received: {line}")
except KeyboardInterrupt:
    print("Exiting...")
finally:
    ser.close()
