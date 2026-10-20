# Thermal Probe

ESP32-S3 + MAX31865 three-wire PT100 thermometer with a local display. Local Wi-Fi, 10 Hz acquisition, smoothed temperature and first/second derivatives. Works with [Thermal Scope](https://github.com/nenipresotReta/thermal-scope) or Artisan over Modbus TCP.

**You MUST fill `config/device.json` before compiling or flashing.** Copy the empty template first:

```sh
cp config/device.example.json config/device.json
```

Edit the copy locally with your **2.4 GHz WPA2** network name and password. Do not commit it. The build stops if configuration is missing, blank or invalid. CMake also validates configuration for direct builds. `flash.sh` validates and rebuilds before flashing to avoid stale credentials.

**Compiled firmware contains the Wi-Fi credentials. Never upload firmware images, generated headers, build directories, backups or your private config.** This repository distributes source only; it deliberately has no downloadable firmware image. `.gitignore` is a safeguard, not a way to remove secrets from existing Git history. Do not force-add ignored files.

## Hardware

Tested hardware: T-Display-S3-compatible ESP32-S3 board with 16 MB flash and a 170 × 320 ST7789 display (HD19006C30 replacement). This is a custom board definition, not a guarantee that every T-Display variant has the same wiring. GPIO15 is driven high for this board's power enable. No built-in keyboard code is included.

| ESP32-S3 | MAX31865 module |
|---|---|
| 3V3 | VIN, **only on the pictured module supporting 3 V input** |
| GND | GND |
| GPIO12 | CLK |
| GPIO11 | SDI / MOSI |
| GPIO13 | SDO / MISO |
| GPIO10 | CS |
| — | RDY and 3V3 output left unconnected |

PT100 reference resistance is **430 ohms**. For the module with F+/RTD+/RTD−/F− terminals: connect the two leads belonging to the same sensor end to F+ and RTD+; connect the single opposite lead to RTD−. Close the `2/3 Wire` bridge, leave `2 Wire` open; on the three-pad selector disconnect center from `24` and connect center to `3`. Confirm continuity with power disconnected: module layouts can differ. The probe connects to this module only, not simultaneously to an old transmitter. Firmware selects three-wire and 50 Hz rejection.

### Local display

The landscape display uses a GitHub Dark-inspired palette and shows temperature, D1 in °C/s, D2 in °C/s², connection address, and a local temperature curve. Sampling and the network continue independently of the screen.

- Tap GPIO14 to hold/resume the local curve. Numerical readings remain live.
- Hold GPIO14 for at least 2 s, then release to clear the local curve and its elapsed time.
- History retains 600 points at 3 s intervals (about 30 minutes) in RAM. A reboot clears it. The default temperature range is 0–250 °C and expands for values outside that range.
- LCD refresh is approximately 2 Hz; sensor/network sampling remains 10 Hz.

The LCD uses an 8-bit 8080 bus: D0–D7 = GPIO39/40/41/42/45/46/47/48, reset = 5, CS = 6, DC = 7, WR = 8, RD = 9, backlight = 38. The driver renders four rows at a time to limit RAM use. This build disables PSRAM and has little unused static DRAM; compile and inspect the memory report when increasing buffer or thread sizes.

## Build and flash

Requires Python, CMake, Ninja, west, Zephyr **4.1.0** and Zephyr SDK **0.17.0** with the ESP32-S3 Xtensa toolchain. Install Zephyr's host dependencies for your OS. A minimal workspace can be prepared using this repo's manifest:

```sh
# From the parent of a checkout named thermal-probe:
python3 -m venv .venv
. .venv/bin/activate
pip install west
west init -l thermal-probe
west update
west zephyr-export
pip install -r zephyr/scripts/requirements.txt
west blobs fetch hal_espressif
# Install SDK 0.17.0 separately and export ZEPHYR_SDK_INSTALL_DIR if needed.
cd thermal-probe
cp config/device.example.json config/device.json
# Fill config/device.json locally, then:
sh build.sh
sh flash.sh
```

Use `sh build.sh -p always` for a pristine rebuild after SDK/board changes. Build output is `build/zephyr/zephyr.bin`. The board uses ESP simple boot with a complete image at offset 0; prefer the configured west flash runner. If automatic download mode fails, hold BOOT, tap RESET, release BOOT, then flash. Board button labels may differ.

The source was derived from a working Zephyr 4.1-based local firmware. Host algorithm/configuration/protocol tests can run without the SDK. A clean upstream dependency installation and every hardware variant require their own verification. This repository contains source only.

## Data and network

DHCP assigns the address. Read the USB serial log or the router's device list; both clients and board need a mutually reachable LAN. There is no fixed personal address, SSID or MAC in this repository. Network interfaces have no authentication/TLS and are intended for a trusted home LAN, not internet exposure.

Acquisition runs every 100 ms. The last 101 continuous valid samples (about 10 s) are fitted to a quadratic using actual sample times. Temperature and derivatives are evaluated at the newest sample. This smooths noise but changes transient response; displayed decimals do not imply accuracy. Derivatives become ready after about 10 s; faults reset the window. Units: °C, °C/s, °C/s². For per-minute display multiply D1 by 60 and D2 by 3600.

`GET http://DEVICE_IP/api/temperature` on port 80 returns `valid`, `derivatives_ready`, `time_s`, `raw_c`, `temperature_c`, `d1_c_per_s`, `d2_c_per_s2`. Invalid numeric values become null. `time_s` is uptime, not wall-clock time.

Modbus TCP port **502**, unit **1**, functions **3/4**; zero-based registers and **big-endian Float32** (two registers per value):

| Register | Value |
|---:|---|
| 0 | Raw temperature |
| 2 | Smoothed temperature |
| 4 | D1 |
| 6 | D2 |

Unavailable data, readings older than 2 s or unready requested derivatives return exception 4. Up to four Modbus clients and two HTTP clients are served independently, with 3 s idle/IO timeouts. HTTP requests may span multiple TCP packets; headers must fit within 512 bytes and arrive within 3 s. Wi-Fi power saving is disabled for responsiveness, increasing battery consumption. No heating control, cloud upload or persistent logging is implemented.

### Artisan

Select MODBUS as the main device; select **TCP**, use the board's DHCP IP and port 502. For **BT**, configure Input 1: unit 1, function 3, register 2, Float32, °C, no scaling, big-endian bytes/words (disable little-endian words). Disable unused ET/Input 2 with device ID 0. Optional MODBUS_34 extra inputs use registers 4/6 as numerical derivatives, not temperature conversion. Artisan's own ΔBT is computed separately.

Start with a 1 s polling interval and a 0.3–1 s timeout. Tune to LAN conditions; retries increase the time a failed poll can take. Board acquisition remains 10 Hz regardless of client polling. In Thermal Scope use DEVICE → DISCOVER or enter the board's IP.

## Tests and source-only publication

```sh
python3 -m unittest discover -s tests -p 'test_*.py'
cc -Wall -Wextra -Werror -Isrc src/filter.c tests/filter_test.c -lm -o /tmp/filter-test
/tmp/filter-test
cc -Wall -Wextra -Werror -Isrc src/modbus_codec.c tests/modbus_test.c -o /tmp/modbus-test
/tmp/modbus-test
cc -Wall -Wextra -Werror -Isrc src/http_request.c tests/http_request_test.c -o /tmp/http-request-test
/tmp/http-request-test
python3 tools/check_release.py
# Optional hardware checks (explicit target required):
python3 tools/check_wireless.py DEVICE_IP
python3 tools/check_multiclient.py DEVICE_IP --stress
```

The release audit accepts only an explicit list of source/template/documentation files and checks for common credential patterns. It never prints credential values. No binary releases or build-artifact uploads are configured.
