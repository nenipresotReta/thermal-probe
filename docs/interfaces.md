# Hardware interfaces

This is the interface contract for the current firmware, not a generic ESP32 wiring guide. GPIO numbers are chip GPIO numbers, not connector positions. Check your board schematic before powering it. No wiring photograph is required.

## Supported target

The tested target is an ESP32-S3 display board with 16 MB flash and a 170 × 320 ST7789 panel using an 8-bit 8080 bus. The custom board target is `tdisplay_s3/esp32s3/procpu`. A matching product name alone does not establish electrical compatibility. PSRAM is disabled.

The current application sets GPIO15 high at startup for the tested board's power enable. Do not reuse this firmware on an arbitrary ESP32-S3 board without reviewing this pin and the display bus. There is currently no selectable generic/headless board profile.

## MAX31865 digital interface

Directions below are relative to the ESP32.

| ESP32 signal | Direction | MAX31865 module signal | Requirement |
|---|---|---|---|
| 3V3 | Power | Documented power input | Module must operate from the available 3.3 V rail and use 3.3 V SPI logic |
| GND | Reference | GND | Common ground |
| GPIO10 | Output | CS | Active low chip select |
| GPIO11 | Output | SDI / MOSI | Data into MAX31865 |
| GPIO12 | Output | CLK / SCK | SPI clock; firmware uses mode 1 |
| GPIO13 | Input | SDO / MISO | Data out of MAX31865; must not exceed ESP32 logic voltage |
| — | — | RDY | Unconnected; firmware polls |

**VIN and 3V3 are not interchangeable across breakout boards.** On the tested breakout, VIN accepts 3.3 V and the breakout's 3V3 regulator output is left unconnected. For another board, check its input range, regulator dropout and whether its 3V3 pin permits direct supply. Do not connect both supply pins based only on their labels. The development board's +5 V pin is not used by this design; battery operation does not establish that it outputs 5 V.

## Three-wire PT100 interface

The application assumes PT100, nominal resistance 100 ohms at 0 °C, reference resistor 430 ohms, three-wire compensation and 50 Hz rejection. PT1000, a different reference resistor, two/four-wire probes or 60 Hz rejection require source changes; `config/device.json` configures Wi-Fi only.

Identify the two leads joined at the same sensor end using the probe specification or resistance measurement with the probe disconnected. Their mutual resistance is approximately the lead resistance; each measures the sensor resistance to the remaining lead. Do not rely on wire color alone.

| Probe lead | Module terminal |
|---|---|
| First lead of the same-end pair | F+ / FORCE+ |
| Second lead of the same-end pair | RTD+ / RTDIN+ |
| Single opposite-end lead | RTD− / RTDIN− |
| No additional probe lead | F− / FORCE−, bridged to RTD− on the module |

For the tested module's jumper labels:

| Jumper | Three-wire state |
|---|---|
| `2/3 Wire` | Closed: F− to RTD− |
| `2 Wire` | Open: F+ and RTD+ are not shorted on the module |
| Three-pad `24 / 3` selector | Center connected to `3`; center disconnected from `24` |

A factory copper link between the center and `24` must be opened before bridging center to `3`. Check with a meter with all power and the probe disconnected; a photograph cannot certify continuity. Do not cut anything unless that connection is present on your particular module. The selector must not connect all three pads. Disconnect the probe from any previous transmitter before attaching it here.

## Built-in display and controls

| Signal | GPIO |
|---|---|
| LCD D0, D1, D2, D3 | 39, 40, 41, 42 |
| LCD D4, D5, D6, D7 | 45, 46, 47, 48 |
| LCD reset, CS, DC | 5, 6, 7 |
| LCD WR, RD | 8, 9 |
| LCD backlight | 38 |
| User button | 14 |
| Board power enable | 15 |

The user button controls the local display; it is not a substitute for the board's BOOT or RESET controls. Firmware download and logs use native USB. Opening a serial port may reset the board; close serial tools before diagnosing uninterrupted uptime.

## Configuration boundaries

| Setting | Current location |
|---|---|
| Wi-Fi SSID/password | Local `config/device.json` |
| Sensor pins, reference resistance, nominal PT100 curve, rejection mode, 100 ms loop | `src/main.c` |
| Filter window | `src/filter.h`, `src/filter.c` |
| LCD pins/driver | `src/tdisplay_lcd.c` |
| Display/button behavior | `src/display.c` |
| Flash layout and USB console | `boards/lilygo/tdisplay_s3/` |

Rebuild and repeat hardware checks after changing any board or sensor parameter. Numerical conversion range does not establish the safe temperature range of the probe, cable or enclosure.
