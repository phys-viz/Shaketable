# ESP32 motor controller

Controls a shake table's brushed DC motor through channel A of an L298N module. Adapted from `BT_motor_buttons_presets.py`. The ESP32 hosts the webpage itself and creates its own Wi-Fi network; no school Wi-Fi, internet, Bluetooth, Python, or computer server is needed. Defaults assume a **classic ESP32 DevKit / ESP32-WROOM**, not a C3, S3, or other variant. Confirm the board before wiring these GPIO numbers.

## Wiring (power disconnected)

| Connection | Destination |
| --- | --- |
| ESP32 GPIO25 | L298N ENA; remove the ENA jumper first |
| ESP32 GPIO26 | L298N IN1 |
| ESP32 GPIO27 | L298N IN2 |
| ESP32 GND | L298N GND and 12 V supply negative |
| 12 V supply positive | L298N motor supply terminal, usually marked +12V / Vs |
| Motor wires | L298N OUT1 and OUT2 |
| ESP32 USB | USB power supply or computer |

Keep the ESP32 powered by USB for initial testing. **Do not connect 12 V to the ESP32 or connect the L298N 5 V terminal to an ESP32 GPIO.** The ENA jumper is separate from the module's 5V-EN regulator jumper. Module regulator arrangements vary: check your module's instructions to establish its 5 V logic supply and correct 5V-EN jumper position. With an external regulated 5 V logic supply, disable the onboard regulator as specified by the module manufacturer; avoid joining two 5 V power sources.

Add a 10 kΩ resistor from ENA to GND to keep the driver disabled while the ESP32 resets or is disconnected. All grounds must be connected. The L298 accepts 3.3 V control signals (its specified minimum high level is 2.3 V), while its logic supply is 5 V. A bare L298 requires external flyback diodes; verify the module includes them. Check motor stall current against the module's actual continuous current and thermal limits. The L298 loses some supply voltage and can get hot.

## Upload and use

1. Install the Arduino IDE and the **esp32 by Espressif Systems** board package, version **3.x**. Select your actual board and USB port.
2. Open `MotorController/MotorController.ino`. Change the Wi-Fi password near the top (at least 8 characters).
3. Upload with the motor supply disconnected. Open Serial Monitor at **115200 baud**.
4. Connect your phone or computer to **MotorController**, using the sketch's password (default `motorcontrol`). Stay connected if your device warns that there is no internet.
5. Open **http://192.168.4.1**, or the address printed in Serial Monitor.
6. Connect the motor supply and start with a low manual PWM setting. The slider applies on release; the -5 and +5 buttons apply immediately. STOP / Reset disables the bridge and lets the motor coast. Rotation is fixed, matching the old Python interface; swap motor leads with power disconnected if the mechanism requires the other direction.

The controller starts stopped. The page sends heartbeats approximately every 400 ms; after approximately 2 seconds without a heartbeat, the ESP32 disables the motor and cancels any preset. Hiding or leaving the page also attempts to send a stop command. Reconnection does not restart the motor. Browser background throttling may stop the motor. Use one controlling page at a time; another page's heartbeats can keep the controller alive and multiple clients can overwrite commands. Speed is PWM duty cycle, not measured RPM. A low setting may not overcome the motor's starting friction.

Software STOP is not a physical emergency stop; provide an accessible motor-power disconnect for the shake table.

## Voltage control and presets

Manual controls use the original 0-255 PWM range, including +/-5, reset, and full power. The voltage target converts to PWM using `PWM = round(target / supply * 255)` and clamps to 0-255. For a 12 V supply, an ideal 6 V average target gives PWM 128. The motor receives switched pulses, not a regulated 6 V supply. Actual average voltage will be lower because of L298N losses and depends on load. The supply voltage field changes the estimate only; it does not change or measure the physical supply. Actual PWM is displayed separately from the selected manual target.

| Preset | Sequence | Total |
| --- | --- | --- |
| Great Motown Earthquake of 2024 | Ramp 0 to 125 in 2 s; hold 125 for 10 s; stop | 12 s |
| Japanese Earthquake of 2011 | Ramp 0 to 255 in 5 s; hold 5 s; ramp to 200 in 5 s; hold 5 s; ramp to 255 in 10 s; hold 5 s; ramp to 200 in 5 s; ramp to 255 in 5 s; stop | 45 s |

These preserve the Python program's intended levels and durations. Its serial delays added extra time, and its ramps did not reach their exact final value; the ESP32 uses elapsed-time interpolation and exact segment boundaries. Presets run on the ESP32 without blocking web requests. Stop cancels immediately when received; a manual command cancels the preset, and selecting another preset restarts from zero. The preset names are inherited demonstration labels, not calibrated earthquake waveforms. Keep the controlling page visible throughout the run.

## Validation and references

`node verify-controller.cjs` checks embedded JavaScript syntax and the preset definitions' durations, boundaries, interpolation, and PWM range. It does not compile the firmware. No Arduino compiler is available in this workspace, and hardware testing has not been performed. Verify your board pinout, module power configuration, and motor current before use.

- [Espressif Wi-Fi API](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/wifi.html)
- [Espressif LEDC / PWM API](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/ledc.html)
- [ST L298 datasheet](https://www.st.com/resource/en/datasheet/l298.pdf)
