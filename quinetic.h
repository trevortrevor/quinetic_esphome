#pragma once
#include "esphome.h"
#include <set>

// CRC-16/AUG-CCITT (Poly: 0x1021, Init: 0x1D0F, No Reflection)
inline uint16_t quinetic_crc16(const uint8_t *data, size_t len) {
  uint16_t crc = 0x1D0F;
  for (size_t i = 0; i < len; i++) {
    crc ^= (uint16_t)data[i] << 8;
    for (uint8_t b = 0; b < 8; b++) {
      if (crc & 0x8000) {
        crc = (crc << 1) ^ 0x1021;
      } else {
        crc <<= 1;
      }
    }
  }
  return crc;
}

struct QuineticStorage {
  uint16_t ids[32];
  uint8_t count;
};

class QuineticHandler {
 public:
  uint32_t last_packet_time = 0;
  uint16_t last_packet_id = 0;
  uint8_t last_packet_action = 0xFF;

  uint16_t candidate_id = 0;
  int press_count = 0;
  uint32_t last_press_time = 0;

  std::set<uint16_t> learned_switches;
  esphome::ESPPreferenceObject pref;

  void setup() {
    pref = esphome::global_preferences->make_preference<QuineticStorage>(28491024);
    QuineticStorage data;
    if (pref.load(&data)) {
      for (uint8_t i = 0; i < data.count && i < 32; i++) {
        learned_switches.insert(data.ids[i]);
      }
      ESP_LOGI("quinetic", "Loaded %d learned switch(es) from flash storage", learned_switches.size());
    }
  }

  bool is_learned(uint16_t id) const {
    return learned_switches.find(id) != learned_switches.end();
  }

  void save_learned(uint16_t id) {
    learned_switches.insert(id);
    QuineticStorage data;
    data.count = 0;
    for (uint16_t s_id : learned_switches) {
      if (data.count < 32) {
        data.ids[data.count++] = s_id;
      }
    }
    pref.save(&data);
  }

  bool process_packet(const std::vector<uint8_t> &x, uint16_t &switch_id, uint8_t &action, bool &is_release, bool &just_learned) {
    just_learned = false;
    if (x.size() < 5) return false;

    // Validate Checksum
    uint16_t received_crc = ((uint16_t)x[3] << 8) | x[4];
    if (received_crc != quinetic_crc16(x.data(), 3)) return false;

    switch_id = ((uint16_t)x[0] << 8) | x[1];
    action = x[2];
    is_release = (action == 0xC0);
    uint32_t now = millis();

    // Debounce duplicate transmissions from the SAME action (bursts are ~50ms)
    // If the state changes from press to release, process it immediately
    if (switch_id == last_packet_id && action == last_packet_action && (now - last_packet_time) < 250) {
      return false;
    }
    last_packet_time = now;
    last_packet_id = switch_id;
    last_packet_action = action;

    // Only count physical PRESSES toward 5-click learning
    if (!is_release) {
      if (switch_id == candidate_id && (now - last_press_time) < 3500) {
        press_count++;
      } else {
        candidate_id = switch_id;
        press_count = 1;
      }
      last_press_time = now;

      if (press_count >= 5) {
        just_learned = true;
        press_count = 0;
        save_learned(switch_id);
      }
    }

    return true;
  }
};

static QuineticHandler quinetic;
