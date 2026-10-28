#pragma once

#include <cstdint>

namespace discplayer {

// Full-step decoder for a mechanical quadrature encoder (EC11 and similar:
// one full A/B cycle per detent). Feed it the pin levels on every sample; it
// returns +1 or -1 when a detent completes, 0 otherwise. Contact bounce and
// half-turned-back detents produce nothing. Based on Ben Buxton's state table.
class QuadratureDecoder {
 public:
  int update(bool a, bool b);

 private:
  uint8_t state_ = 0;
};

// Debounced push button with a long-press detector. `pressed` is the logical
// state (true = held down). Reports LongPress once per hold, as soon as the
// hold reaches longPressMs, so the user gets feedback without letting go.
enum class ButtonEvent { None, LongPress };

class ButtonDetector {
 public:
  ButtonDetector(uint32_t debounceMs, uint32_t longPressMs)
      : debounceMs_(debounceMs), longPressMs_(longPressMs) {}

  ButtonEvent update(uint32_t nowMs, bool pressed);

 private:
  uint32_t debounceMs_;
  uint32_t longPressMs_;
  bool raw_ = false;
  bool stable_ = false;
  uint32_t rawChangedMs_ = 0;
  uint32_t pressedSinceMs_ = 0;
  bool longReported_ = false;
};

// Volume rules for the knob. Volumes are Sonos 0-100.
constexpr int kVolumeStep = 2;   // per detent
constexpr int kMinMaxVolume = 5; // a max below this would make the player useless

inline int startVolumeFor(int maxVolume) { return maxVolume / 2; }

// New volume after turning the knob by `detents` from `current`, never above
// `maxVolume`. If someone set the speaker above the cap from elsewhere,
// turning up leaves it alone and turning down drops it to the cap.
// `atCap` is set when the result is pinned at the cap.
int applyKnob(int current, int detents, int maxVolume, bool& atCap);

// Max volume to store when a parent long-presses at the speaker's current
// volume. False if that volume is too low to be a sensible cap.
bool maxFromCurrentVolume(int current, int& maxVolume);

// LEDs to light for a volume gauge where the full ring means maxVolume.
// Any audible volume lights at least one LED.
int gaugeLeds(int volume, int maxVolume, int ledCount);

}  // namespace discplayer
