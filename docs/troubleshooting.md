# First start and troubleshooting

[English](troubleshooting.md) | [简体中文](troubleshooting.zh-CN.md)

Follow the README build instructions and [interface tables](interfaces.md) first. This repository contains no preconfigured binary. Network credentials are embedded during compilation, so changing Wi-Fi requires rebuilding and flashing.

## First-start acceptance

1. Confirm the board variant, power input, sensor terminals and jumper continuity before applying power.
2. Fill `config/device.json` locally. Run `sh build.sh`; a missing or invalid configuration must stop the build. Flash using `sh flash.sh`.
3. Find the DHCP address in the LCD, USB log or router client list. Use that address as `DEVICE_IP` in commands below.
4. After roughly 10 seconds of uninterrupted valid acquisition, check:

   ```sh
   curl --fail --max-time 5 http://DEVICE_IP/api/temperature
   python3 tools/check_wireless.py DEVICE_IP
   ```

   Expect `valid: true`, then `derivatives_ready: true`, finite temperatures and advancing `time_s`. A reported value alone does not verify temperature accuracy; compare against a known reference at a stable temperature.
5. In Thermal Scope, select DEVICE → TEST → CONNECT. Verify that values and the curve advance. Briefly change the probe temperature within its rated limits and check that the measured trend follows it.

Do not run the multiclient stress tool during an important recording: it intentionally occupies connections and tests malformed traffic/timeouts. Use it during a dedicated test session.

## Diagnose by layer

| Symptom | Check first |
|---|---|
| Build refuses configuration | Copy the template; use a 1–32 UTF-8-byte SSID and an 8–63-byte WPA2 password; no control characters |
| SDK/compiler missing | Zephyr 4.1.0, SDK 0.17.0 with ESP32-S3 Xtensa tools, activated venv, fetched Espressif blobs |
| Flash cannot connect | Correct native USB data cable/port; close serial tools; use the board's BOOT + RESET sequence |
| No Wi-Fi address | 2.4 GHz WPA2 credentials, signal and DHCP; open/enterprise networks are not supported by this configuration |
| Router says online, HTTP unreachable | Correct current address; routing, guest isolation, firewall and power; an online listing does not prove TCP reachability |
| `valid: false` / SENSOR_ERROR | Power and common ground, SPI direction/pins, PT100 continuity, reference resistor and three-wire jumpers |
| Valid temperature, derivatives unavailable | Wait for about 10 continuous seconds; sensor faults reset the filter |
| HTTP works, Modbus fails | TCP 502 reachability; unit 1, function 3/4, zero-based registers and big-endian Float32 |
| Wrong scale/value | PT100 vs PT1000, actual reference resistance, register/word order and client units; do not compensate a wiring error with an offset |
| Intermittent readings | Capture whether uptime resets, sensor validity changes or only one client fails; close unnecessary clients |
| Display blank but network works | Verify the panel and bus match this custom board; a generic ST7789 label does not guarantee compatibility |

The server supports four Modbus and two HTTP clients. Thermal Scope shares one collector among browser viewers. Idle test clients consume slots until timeout. Avoid concluding that the firmware rebooted from a client timeout alone: compare device uptime and, when available, reset logs.

## Reporting a reproducible issue

Include firmware commit/tag, exact board/module variant, supply arrangement, pin mapping, jumper continuity, build/SDK versions, client polling settings, and a short redacted error extract. State whether the problem occurs without additional test clients or serial tools. Report valid/ready state and uptime before/after the event.

Do not attach `config/device.json`, generated headers, firmware binaries, complete build folders or unreviewed logs. Remove network names, passwords, device addresses, MAC addresses and personal paths from diagnostic material.

## Release status

The initial firmware release is an RC. Host tests and short hardware stress tests do not establish long-duration reliability or support for other boards. Earlier offline/invalid-response observations are unresolved; shared formatting concurrency was corrected, but a causal reproduction was not captured. Keep observations and verified fixes distinct when reporting results.
