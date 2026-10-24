# Third-party notices

[English](THIRD_PARTY_NOTICES.md) | [简体中文](THIRD_PARTY_NOTICES.zh-CN.md)

Project application source is licensed under [MIT](LICENSE). No third-party dependency source or binary is vendored in this repository. Dependency code fetched during building retains its upstream license and notices.

| Component | Source and licensing scope |
|---|---|
| Zephyr 4.1.0 | [zephyrproject-rtos/zephyr](https://github.com/zephyrproject-rtos/zephyr/tree/v4.1.0), predominantly Apache-2.0; retain per-file and third-party notices |
| Espressif/Xtensa HAL and Mbed TLS | Revisions selected by Zephyr's imported west manifest; consult each dependency's LICENSE and per-file notices |
| ESP32 Wi-Fi blobs | Downloaded separately by `west blobs fetch hal_espressif`; subject to the blob metadata and upstream Espressif terms |
| Zephyr SDK 0.17.0 | [sdk-ng release](https://github.com/zephyrproject-rtos/sdk-ng/releases/tag/v0.17.0); compiler/runtime components have their own licenses |

The existing LCD driver carries an MIT SPDX marker. GitHub Dark is a palette inspiration; no GitHub logo or brand assets are bundled. Product/chip names identify compatible hardware and do not imply endorsement.

Only application source is distributed here. Before independently distributing a linked binary, review the licenses of all linked components; the project MIT file does not replace them. Credentials are also embedded in locally built firmware.
