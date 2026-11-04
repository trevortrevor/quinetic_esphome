# ESPHome Quinetic RF Bridge (ESP32 + CC1101)

An ESPHome firmware component that integrates battery-free, kinetic **Quinetic** wireless switches into **Home Assistant** using an ESP32 and a TI CC1101 transceiver. 

The bridge can both **receive** physical rocker events (differentiating between press and release) and **transmit** RF packets to pair with and control Quinetic wireless receiver relays.

---

## Features

- **Bidirectional RF:** Detects physical Quinetic switches and emulates switches to actuate Quinetic in-line relays.
- **Press & Release Discrimination:** Captures both initial press activations and the spring return release (`0xC0`) packet, enabling hold-to-dim and momentary automation behaviors.
- **Dynamic 5-Click Learning:** Press any Quinetic switch 5 times in quick succession to automatically register it in Home Assistant without editing YAML or restarting.
- **Home Assistant MQTT Discovery:** Learned switches are automatically published as Home Assistant `event` entities (`event.quinetic_0xXXXX`) grouped under a single device.
- **NVS Flash Persistence:** Learned switches are preserved across OTA updates and power cycles in the ESP32's non-volatile storage.

---

## Hardware Wiring

Connect the CC1101 8-pin module to the ESP32 as follows:

| CC1101 Pin | ESP32 GPIO | Description | Notes |
|---|---|---|---|
| **VCC** | **3V3** | Power (3.3V) | **Do not connect to 5V!** |
| **GND** | **GND** | Ground | Common ground |
| **CSN** | **GPIO17** | SPI Chip Select | Configurable in YAML |
| **SCK** | **GPIO18** | SPI Clock | Hardware SPI (SPI2_HOST) |
| **MOSI** | **GPIO23** | SPI Master Out | Hardware SPI |
| **MISO / GD1** | **GPIO19** | SPI Master In | Hardware SPI |
| **GDO0** | **GPIO16** | Packet RX Interrupt | Pin 3 on standard 8-pin CC1101 |
| **GDO2** | *N/C* | Unused | Leave unconnected |

> **Important:** Ensure `GPIO16` is wired to **GDO0** (Pin 3), **not** GDO2 (Pin 8).

---

## Technical RF Protocol Details

Quinetic switches use an electrodynamic micro-generator that powers a short, high-speed 2-FSK burst when pushed down and when released:

* **Nominal Frequency:** `433.30 MHz` (Note: Quinetic operates at ~433.30 MHz, *not* 433.92 MHz).
* **Modulation:** `2-FSK`
* **Symbol Rate:** `100,000 baud` (10 µs bit width)
* **Frequency Deviation:** `50 kHz`
* **Filter Bandwidth:** `325 kHz` (tolerates crystal frequency drift)
* **Sync Word:** `0xA423` (16-bit)
* **Packet Payload (5 bytes):**
  * `Bytes 0–1`: 16-bit Switch ID (Big Endian)
  * `Byte 2`: Action code (`0x01` = Button Press, `0xC0` = Rocker Release)
  * `Bytes 3–4`: CRC-16 Checksum via **CRC-16/AUG-CCITT** (Polynomial `0x1021`, Initial value `0x1D0F`)

---

## Installation & Setup

### 1. Clone & Prepare Secrets
Clone this repository into your ESPHome configuration directory:
```bash
git clone https://github.com/<your-username>/esphome-quinetic-bridge.git
cd esphome-quinetic-bridge
cp secrets.yaml.example secrets.yaml
