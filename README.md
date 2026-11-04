# ESPHome Quinetic RF Bridge (ESP32 + CC1101)

An ESPHome firmware component that integrates battery-free, kinetic **Quinetic** wireless switches into **Home Assistant** using an ESP32 and a Texas Instruments CC1101 transceiver module.

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

> **Warning:** Ensure `GPIO16` is wired to **GDO0** (Pin 3), **not** GDO2 (Pin 8).

---

## Technical RF Protocol Details

Quinetic switches use an electrodynamic micro-generator that powers a short, high-speed 2-FSK burst when pushed down and when released:

* **Nominal Frequency:** `433.30 MHz` (Quinetic operates at ~433.30 MHz, *not* 433.92 MHz).
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
```

Edit `secrets.yaml` with your Wi-Fi credentials, MQTT broker details, and API encryption key.

### 2. Compile and Flash
Compile and flash the firmware using the ESPHome CLI or the ESPHome Dashboard:

```bash
esphome run quinetic-bridge.yaml
```

---

## Usage

### Learning a New Switch
1. Push your physical Quinetic switch **5 times rapidly** (within 3.5 seconds).
2. The ESP32 logs `>>> NEW SWITCH LEARNED & REGISTERED <<<` and automatically publishes an MQTT discovery payload to Home Assistant.
3. A new entity named **`event.quinetic_0xXXXX`** will appear in Home Assistant under **Settings → Devices & Services → MQTT → Quinetic Bridge**.

### Home Assistant Automation Example (Hold-to-Dim / Press & Release)

```yaml
alias: "Quinetic Switch Press & Release"
mode: restart
trigger:
  - trigger: state
    entity_id: event.quinetic_0x1b87
    attribute: event_type
    to: "press"
    id: press
  - trigger: state
    entity_id: event.quinetic_0x1b87
    attribute: event_type
    to: "release"
    id: release
action:
  - choose:
      - conditions:
          - condition: trigger
            id: press
        sequence:
          - action: light.turn_on
            target:
              entity_id: light.living_room
      - conditions:
          - condition: trigger
            id: release
        sequence:
          - action: light.turn_off
            target:
              entity_id: light.living_room
```

### Transmitting (Controlling Quinetic Relays)
To pair your ESP32 with a Quinetic receiver relay (e.g. QURW01):
1. Put the Quinetic relay into pairing mode by holding its physical button until the LED flashes.
2. Call the Home Assistant action `esphome.quinetic_bridge_transmit_quinetic` with your chosen virtual Switch ID (e.g., `0xABCD`) and action `1`.
3. The receiver will pair to this virtual switch ID and can now be toggled via automations or scripts.

---

## Troubleshooting

- **`cc1101 is marked FAILED` on boot:** Ensure the CC1101 is receiving a stable 3.3V power source. If this occurs after an OTA update, physically power cycle the board (unplug USB for 5 seconds) to reset the CC1101 state machine.
- **Switches not detected:** Verify that `filter_bandwidth` is set to `325kHz` to account for oscillator drift on self-powered kinetic switches.
- **No packets received:** Double-check that `GPIO16` connects to **GDO0** (Pin 3 on standard 8-pin CC1101 boards) and that the module's crystal is 26.000 MHz.

---

## Credits & Acknowledgments

This project relies on the protocol reverse-engineering work and documentation provided by:
* The **[rtl_433](https://github.com/merbanan/rtl_433)** community for demodulation parameters and packet structure analysis.
* **Cameron Gray** and the RF reverse-engineering community for documenting the Quinetic 433.30 MHz FSK parameters and CRC-16/AUG-CCITT checksum algorithm.
* The **[ESPHome](https://esphome.io)** team and contributors of the native `cc1101` transceiver component.

---

## License

This project is licensed under the [MIT License](LICENSE).
