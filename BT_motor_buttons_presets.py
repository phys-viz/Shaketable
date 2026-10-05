import tkinter as tk
from tkinter import messagebox, simpledialog
import serial
from serial.tools import list_ports
import time

# Global variables for the PWM value and serial communication
current_pwm_value = 0
arduino_serial = None
last_sent_value = None  # Track the last value to avoid unnecessary serial writes

# Define baud rate for serial communication
baud_rate = 9600  # Ensure this matches the baud rate of your BlueSMiRF or Arduino

# Function to list available serial ports and their descriptions
def list_serial_ports():
    ports = list_ports.comports()
    available_ports = []
    for port in ports:
        port_info = f"{port.device}: {port.description}"
        available_ports.append((port.device, port.description))
        print(port_info)  # Print the port and its description
    return available_ports

# Function to prompt user to select a COM port
def select_com_port():
    ports = list_serial_ports()
    if not ports:
        messagebox.showerror("Error", "No serial ports found.")
        return None

    # Display available ports in a prompt
    port_list = "\n".join([f"{idx + 1}: {port[0]} ({port[1]})" for idx, port in enumerate(ports)])
    port_input = simpledialog.askstring("Select Port", f"Select a port by number or name (e.g., COM25):\n{port_list}")

    # Try to interpret the user's input as either an index or a COM port name
    try:
        # First, check if it's an index (number)
        port_index = int(port_input) - 1  # Adjust for zero-based index
        if 0 <= port_index < len(ports):
            selected_port = ports[port_index][0]  # Get the COM port name (e.g., COM25)
            return selected_port
    except ValueError:
        # If it's not a number, check if it matches a valid COM port name
        for port in ports:
            if port[0].lower() == port_input.lower():  # Check if the input matches the COM port name (case-insensitive)
                return port[0]

    messagebox.showerror("Error", "Invalid selection. Please enter a valid port number or name.")
    return None

# Function to initialize the serial connection
def initialize_serial():
    global arduino_serial
    selected_port = select_com_port()
    if selected_port:
        try:
            # Open serial connection
            arduino_serial = serial.Serial(selected_port, baud_rate, timeout=1)
            arduino_serial.setRTS(False)  # Disable RTS
            arduino_serial.setDTR(False)  # Disable DTR
            messagebox.showinfo("Success", f"Connected to {selected_port}")
        except serial.SerialException as e:
            messagebox.showerror("Connection Error", f"Failed to connect to {selected_port}. Error: {e}")
            arduino_serial = None
    else:
        arduino_serial = None

# Function to send the PWM value to the Arduino and update the label
def send_pwm_value(pwm_value):
    global last_sent_value, current_pwm_value
    current_pwm_value = pwm_value  # Update global PWM value
    update_pwm_label()  # Update the label on the GUI
    try:
        if arduino_serial and pwm_value != last_sent_value:
            arduino_serial.write(f"{pwm_value}\n".encode())  # Send updated PWM value
            last_sent_value = pwm_value
            time.sleep(0.05)  # Slight delay to avoid overwhelming serial
    except Exception as e:
        print(f"Error sending PWM value: {e}")

# Function to ramp the motor speed gradually to a target PWM
def ramp_pwm(start_pwm, target_pwm, ramp_time):
    step_count = 50  # Number of steps in the ramp
    step_delay = ramp_time / step_count  # Delay between each step
    pwm_step = (target_pwm - start_pwm) / step_count  # PWM increment per step

    for i in range(step_count):
        current_pwm_value = int(start_pwm + (i * pwm_step))
        send_pwm_value(current_pwm_value)
        time.sleep(step_delay)

# Function for the "Great Motown Earthquake of 2024"
def great_motown_earthquake():
    # Ramp up from 0 to 125 PWM
    ramp_pwm(0, 125, 2)  # 2 seconds to reach 125 PWM
    
    # Hold at 125 PWM for 10 seconds
    send_pwm_value(125)
    time.sleep(10)
    
    # Stop the motor (PWM = 0)
    send_pwm_value(0)

# Function for the "Japanese Earthquake of 2011"
def japanese_earthquake():
    # Ramp up from 0 to 255 PWM
    ramp_pwm(0, 255, 5)  # 5 seconds to reach 255 PWM
    
    # Hold at 255 PWM for 5 seconds
    send_pwm_value(255)
    time.sleep(5)
    
    # Ramp down to 200 PWM over 5 seconds
    ramp_pwm(255, 200, 5)
    
    # Hold at 200 PWM for 5 seconds
    send_pwm_value(200)
    time.sleep(5)
    
    # Ramp back up to 255 PWM over 10 seconds
    ramp_pwm(200, 255, 10)
    
    # Hold at 255 PWM for 5 seconds
    send_pwm_value(255)
    time.sleep(5)
    
    # Ramp down to 200, then back up to 255
    ramp_pwm(255, 200, 5)
    ramp_pwm(200, 255, 5)
    
    # Stop the motor (PWM = 0)
    send_pwm_value(0)

# Function to increase the PWM value by 5
def increase_pwm():
    global current_pwm_value
    if current_pwm_value <= 250:  # Ensure it doesn't exceed 255
        current_pwm_value += 5
    send_pwm_value(current_pwm_value)

# Function to decrease the PWM value by 5
def decrease_pwm():
    global current_pwm_value
    if current_pwm_value >= 5:  # Ensure it doesn't go below 0
        current_pwm_value -= 5
    send_pwm_value(current_pwm_value)

# Function to reset the PWM value to 0
def reset_pwm():
    send_pwm_value(0)

# Function to set the PWM value to 255 (full speed)
def set_pwm_to_255():
    send_pwm_value(255)

# Function to update the label with the current PWM value
def update_pwm_label():
    pwm_label.config(text=f"Current PWM: {current_pwm_value}")

# GUI Setup
root = tk.Tk()
root.title("Motor Speed Control")
root.geometry("400x400")

# Button to connect to the selected COM port
connect_button = tk.Button(root, text="Connect to Arduino", font=("Helvetica", 12), command=initialize_serial)
connect_button.pack(pady=10)

# PWM value display label
pwm_label = tk.Label(root, text=f"Current PWM: {current_pwm_value}", font=("Helvetica", 14))
pwm_label.pack(pady=10)

# Increase PWM button
increase_button = tk.Button(root, text="Increase PWM", font=("Helvetica", 12), command=increase_pwm)
increase_button.pack(pady=5)

# Decrease PWM button
decrease_button = tk.Button(root, text="Decrease PWM", font=("Helvetica", 12), command=decrease_pwm)
decrease_button.pack(pady=5)

# Reset PWM button
reset_button = tk.Button(root, text="Reset PWM", font=("Helvetica", 12), command=reset_pwm)
reset_button.pack(pady=5)

# Set PWM to 255 button
set_to_255_button = tk.Button(root, text="Set PWM to 255", font=("Helvetica", 12), command=set_pwm_to_255)
set_to_255_button.pack(pady=5)

# Button for the "Great Motown Earthquake of 2024"
earthquake_button = tk.Button(root, text="Great Motown Earthquake of 2024", font=("Helvetica", 12), command=great_motown_earthquake)
earthquake_button.pack(pady=10)

# Button for the "Japanese Earthquake of 2011"
japanese_earthquake_button = tk.Button(root, text="Japanese Earthquake of 2011", font=("Helvetica", 12), command=japanese_earthquake)
japanese_earthquake_button.pack(pady=10)

# Run the GUI event loop
root.mainloop()

# Close the serial connection when the GUI is closed
if arduino_serial:
    arduino_serial.close()
