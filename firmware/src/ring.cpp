#include "ring.h"

#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

#include <atomic>

#include "board.h"
#include "controls.h"

namespace {

enum class Overlay : int { None, Ok, Error, Volume, MaxSaved };

constexpr uint32_t kFrameMs = 33;
constexpr uint32_t kOkMs = 700;
constexpr uint32_t kErrorMs = 900;
constexpr uint32_t kVolumeMs = 2500;
constexpr uint32_t kMaxSavedFlashMs = 800;

Adafruit_NeoPixel pixels(RING_LED_COUNT, RING_PIN, NEO_GRB + NEO_KHZ800);

std::atomic<RingState> state{RingState::Idle};
std::atomic<Overlay> overlay{Overlay::None};
std::atomic<uint32_t> overlaySinceMs{0};
std::atomic<int> gaugeVolume{0};
std::atomic<int> gaugeMax{1};
std::atomic<bool> gaugeAtCap{false};

void startOverlay(Overlay o) {
  overlaySinceMs = millis();
  overlay = o;
}

uint32_t scale(uint8_t r, uint8_t g, uint8_t b, float k) {
  return Adafruit_NeoPixel::Color(static_cast<uint8_t>(r * k), static_cast<uint8_t>(g * k),
                                  static_cast<uint8_t>(b * k));
}

void spinner(uint32_t now, uint8_t r, uint8_t g, uint8_t b) {
  const int head = (now / 70) % RING_LED_COUNT;
  for (int tail = 0; tail < 3; ++tail)
    pixels.setPixelColor((head - tail + RING_LED_COUNT) % RING_LED_COUNT, scale(r, g, b, 1.0f / (1 + 2 * tail)));
}

void gauge(uint32_t now) {
  const int lit = discplayer::gaugeLeds(gaugeVolume, gaugeMax, RING_LED_COUNT);
  for (int i = 0; i < lit; ++i) pixels.setPixelColor(i, Adafruit_NeoPixel::Color(255, 120, 20));
  // At the cap, the last lit LED blinks red: "that's as loud as it goes".
  if (gaugeAtCap && lit > 0 && (now / 150) % 2 == 0) pixels.setPixelColor(lit - 1, Adafruit_NeoPixel::Color(255, 0, 0));
}

// Returns false once the overlay has finished.
bool drawOverlay(Overlay o, uint32_t elapsed, uint32_t now) {
  switch (o) {
    case Overlay::Ok:
      if (elapsed >= kOkMs) return false;
      pixels.fill(scale(0, 255, 60, 1.0f - static_cast<float>(elapsed) / kOkMs));
      return true;
    case Overlay::Error:
      if (elapsed >= kErrorMs) return false;
      if ((elapsed / 150) % 2 == 0) pixels.fill(Adafruit_NeoPixel::Color(255, 0, 0));
      return true;
    case Overlay::Volume:
      if (elapsed >= kVolumeMs) return false;
      gauge(now);
      return true;
    case Overlay::MaxSaved:
      if (elapsed >= kMaxSavedFlashMs + kVolumeMs) return false;
      if (elapsed < kMaxSavedFlashMs) {
        if ((elapsed / 200) % 2 == 0) pixels.fill(Adafruit_NeoPixel::Color(255, 255, 255));
      } else {
        gauge(now);
      }
      return true;
    case Overlay::None:
      break;
  }
  return false;
}

void drawState(RingState s, uint32_t now) {
  switch (s) {
    case RingState::Idle:
      break;
    case RingState::Reading:
      spinner(now, 255, 255, 255);
      break;
    case RingState::Updating:
      spinner(now, 255, 100, 0);
      break;
    case RingState::Setup: {
      const float phase = (now % 3000) / 3000.0f;
      const float k = 0.1f + 0.9f * (phase < 0.5f ? phase * 2 : (1 - phase) * 2);
      pixels.fill(scale(0, 60, 255, k));
      break;
    }
  }
}

void ringTask(void*) {
  for (;;) {
    const uint32_t now = millis();
    pixels.clear();
    Overlay o = overlay;
    if (o == Overlay::None || !drawOverlay(o, now - overlaySinceMs, now)) {
      // Clear only if no newer overlay was started meanwhile.
      overlay.compare_exchange_strong(o, Overlay::None);
      drawState(state, now);
    }
    pixels.show();
    vTaskDelay(pdMS_TO_TICKS(kFrameMs));
  }
}

}  // namespace

void ringBegin() {
  pixels.begin();
  pixels.setBrightness(RING_BRIGHTNESS);
  pixels.clear();
  pixels.show();
  xTaskCreatePinnedToCore(ringTask, "ring", 3072, nullptr, 1, nullptr, 0);
}

void ringSetState(RingState s) { state = s; }

void ringFlashOk() { startOverlay(Overlay::Ok); }

void ringFlashError() { startOverlay(Overlay::Error); }

void ringShowVolume(int volume, int maxVolume, bool atCap) {
  gaugeVolume = volume;
  gaugeMax = maxVolume;
  gaugeAtCap = atCap;
  startOverlay(Overlay::Volume);
}

void ringFlashMaxSaved(int maxVolume) {
  gaugeVolume = maxVolume;
  gaugeMax = maxVolume;
  gaugeAtCap = false;
  startOverlay(Overlay::MaxSaved);
}
