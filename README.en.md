# ESP32 Ethernet Devboard

[Türkçe](README.md) · [Product page](https://ilimera.com/en/urunler/gelistirme-kartlari/esp32-ethernet-devboard) · [Technical document (PDF, Turkish)](docs/ESP32_Ethernet_teknik_dokuman_v2.pdf)

![ESP32 Ethernet Devboard](docs/images/esp32-ethernet-main.webp)

The İLİMERA ESP32 Ethernet Devboard combines an **ESP32-WROOM-32D** module with a **LAN8720 Ethernet PHY** and an
**RJ45** port. It offers Wi-Fi, Bluetooth and 10/100 Mbps wired networking, and is programmed over USB Type-C
from the Arduino IDE or PlatformIO. It is built for industrial data acquisition, building automation, web-based
control and wired/wireless gateway projects.

This repository contains ready-to-use example sketches.

## Specifications

| Feature | Value |
| --- | --- |
| Module | ESP32-WROOM-32D, dual-core Xtensa LX6, 240 MHz |
| Memory | 520 KB SRAM, 4 MB SPI Flash |
| Ethernet | LAN8720 PHY, 10/100 Mbps, RJ45 (female) |
| Wireless | Wi-Fi 2.4 GHz 802.11 b/g/n, Bluetooth 4.2 BR/EDR + BLE |
| Programming | USB Type-C (auto upload, RST and BOOT buttons) |
| Power | 5 V (USB Type-C or 5V pin), on-board 5 V → 3.3 V regulator |
| Toolchains | Arduino IDE, PlatformIO |

## Ethernet PHY pin definitions

The ESP32 ↔ LAN8720 wiring is fixed on the board. Enter these values **exactly**, otherwise the board will not
get an IP address.

| Parameter | Value |
| --- | --- |
| PHY type | `ETH_PHY_LAN8720` |
| PHY address | `1` |
| MDC | GPIO 23 |
| MDIO | GPIO 18 |
| Clock | `ETH_CLOCK_GPIO17_OUT` (GPIO 17 output) |
| Power pin | `-1` (unused) |

The argument order of `ETH.begin` differs between ESP32 Arduino core 2.x and 3.x. The examples handle both:

```cpp
#include <WiFi.h>
#include <ETH.h>

#if ESP_ARDUINO_VERSION_MAJOR >= 3   // current "esp32" package in the Arduino IDE (3.x)
  ETH.begin(ETH_PHY_LAN8720, 1, 23, 18, -1, ETH_CLOCK_GPIO17_OUT);
#else                                // PlatformIO espressif32 platform (2.x)
  ETH.begin(1, -1, 23, 18, ETH_PHY_LAN8720, ETH_CLOCK_GPIO17_OUT);
#endif
```

RMII data lines (GPIO 19, 21, 22, 25, 26, 27) and GPIO 17, 18, 23 are reserved for Ethernet.

## Available GPIO pins

![Pinout](docs/images/esp32-ethernet-pinout.webp)

| Pins | Note |
| --- | --- |
| IO4, IO13, IO14, IO16, IO32, IO33 | Recommended general-purpose I/O |
| IO34, IO35, IO36, IO39 | **Input only**, no internal pull-up; good for analog readings |
| IO0, IO2, IO5, IO12, IO15 | Strapping pins: must not be pulled externally at boot. IO0 = BOOT button |
| TX, RX | USB serial (Serial), uploading and Serial Monitor |
| 3V3, 5V, GND | Power |

## Quick start

**Arduino IDE**

1. Add `https://espressif.github.io/arduino-esp32/package_esp32_index.json` under
   **File → Preferences → Additional boards manager URLs**.
2. Install the **esp32** (Espressif Systems) package from **Tools → Board → Boards Manager**.
3. Select **Tools → Board: ESP32 Dev Module** and the board's port.
4. Open and upload [`examples/01_Ethernet_Baglanti_Testi`](examples/01_Ethernet_Baglanti_Testi).
5. Plug in the Ethernet cable and open the Serial Monitor at **115200 baud**: you should see
   `IP adresi: 192.168.x.x` and `HTTP kodu: 204`.

**PlatformIO**

Set `src_dir` in [`platformio.ini`](platformio.ini) to the example folder you want
(e.g. `src_dir = examples/03_Web_Sunucu_GPIO_Kontrol`), then:

```bash
pio run -t upload
pio device monitor -b 115200
```

If the upload does not start, hold **BOOT**, start the upload and release it when "Connecting..." appears.

## Examples

| Example | What it does |
| --- | --- |
| [01_Ethernet_Baglanti_Testi](examples/01_Ethernet_Baglanti_Testi) | Gets an IP over DHCP and checks internet access with an HTTP GET every 10 s (the datasheet example) |
| [02_Statik_IP](examples/02_Statik_IP) | Static IP, gateway and DNS for networks without DHCP |
| [03_Web_Sunucu_GPIO_Kontrol](examples/03_Web_Sunucu_GPIO_Kontrol) | Toggle an output pin from a browser, status page and `/durum` JSON endpoint |
| [04_MQTT_Telemetri](examples/04_MQTT_Telemetri) | Publishes telemetry to MQTT over Ethernet, listens for `AC`/`KAPAT` (on/off) commands |
| [05_Ethernet_WiFi_Yedekli](examples/05_Ethernet_WiFi_Yedekli) | Ethernet first; falls back to Wi-Fi when the cable is unplugged and returns when it is back |

Edit IP, Wi-Fi, MQTT and pin values in the **KULLANICI AYARLARI** (user settings) block at the top of each
sketch. `04_MQTT_Telemetri` needs the **PubSubClient** library; the others only use libraries bundled with the
ESP32 package. Code comments are in Turkish with an English summary in each header.

## Project tips

- **Ethernet and Wi-Fi together:** the ESP32 tends to route traffic through whichever interface got an IP most
  recently. Control which interface is used, or keep only one active at a time as in `05_Ethernet_WiFi_Yedekli`.
- **Power:** with Wi-Fi, Bluetooth and Ethernet all active, supply **at least 1 A** of clean power from USB
  Type-C or the 5V pin.
- **MAC address:** the Ethernet MAC is derived from the ESP32 eFuse and is unique per board (`ETH.macAddress()`).

## Troubleshooting

| Symptom | Fix |
| --- | --- |
| `ETH baslatildi` but no IP | Check the cable and the RJ45 LEDs; use `02_Statik_IP` if the network has no DHCP |
| "no matching function" for `ETH.begin` | Argument order does not match your core version; use the `#if ESP_ARDUINO_VERSION_MAJOR` block |
| `HTTP kodu: -1` | IP but no internet: check gateway, DNS or firewall |
| The board keeps rebooting at power-up | Insufficient supply: use a 1 A source and a short cable |
| Upload stuck at "Connecting..." | Hold BOOT while starting the upload; make sure the USB cable carries data |
| The board does not boot with external circuits attached | IO0, IO2, IO5, IO12, IO15 are strapping pins; disconnect circuits from them |

## Support

Documentation, updates and support:
[ilimera.com](https://ilimera.com/en/urunler/gelistirme-kartlari/esp32-ethernet-devboard).
Found a bug or have a suggestion? Open an **Issue** in this repository.

ESP32 Ethernet Devboard is developed by İLİMERA Technology.

## License

The example code is provided under the [MIT License](LICENSE); you are free to use it in your own products.
Technical documents and images are the property of İLİMERA Technology.
