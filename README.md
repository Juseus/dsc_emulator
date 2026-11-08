# BMW DSC Emulator

An Arduino Uno sketch that emulates the BMW ASC/DSC module's ASC1 CAN message (`0x153`). It converts a vehicle speed pulse signal into a speed value and sends an 8-byte CAN frame every 20 ms at 500 kbit/s using an MCP2515 CAN controller and the MCP2515 library.

The speed signal comes from the ABS unit as a **nominal 12 V square-wave signal**, with speed encoded by its **frequency**. External signal-conditioning hardware is required to reduce it to **0–5 V** before connecting it to **digital pin 8 (D8)**. Do not connect the signal directly to the Arduino.

On the Arduino Uno, FreqMeasure uses D8 as the Timer1 input capture pin (ICP1), as specified in its [hardware requirements](https://www.pjrc.com/teensy/td_libs_FreqMeasure.html).

## Libraries

- [FreqMeasure](https://github.com/PaulStoffregen/FreqMeasure) — measures the input pulse frequency.
- [TimerEvent](https://github.com/cygig/TimerEvent) — schedules CAN messages and the measurement timeout.
- [MCP2515 (arduino-mcp2515)](https://github.com/autowp/arduino-mcp2515) — controls the MCP2515 over Arduino's built-in SPI library.

## Speed formula

The input signal produces 6.92 Hz per km/h:

```text
speed_kmh = frequency_hz / 6.92
raw_speed = round(speed_kmh * 127 + 352)

CAN byte 1 = raw_speed & 0xF8
CAN byte 2 = raw_speed >> 8
```

Byte 1 holds the low byte and byte 2 holds the high byte. The `0xF8` mask clears byte 1's lowest three bits because they contain ASC/MSR status flags, keeping those flags at zero.

Speeds below 1 km/h are reported as zero. The speed also returns to zero after 150 ms without a new measurement.

See [MS4X revision 3534](https://www.ms4x.net/index.php?title=CAN_Bus_ID_0x153_ASC1&oldid=3534) for the speed formula and the [current page](https://www.ms4x.net/index.php?title=CAN_Bus_ID_0x153_ASC1) for bit definitions only.
