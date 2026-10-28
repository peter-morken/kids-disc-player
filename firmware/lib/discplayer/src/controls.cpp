#include "controls.h"

namespace discplayer {
namespace {

enum : uint8_t {
  kStart = 0x0,
  kCwFinal = 0x1,
  kCwBegin = 0x2,
  kCwNext = 0x3,
  kCcwBegin = 0x4,
  kCcwFinal = 0x5,
  kCcwNext = 0x6,
  kDirCw = 0x10,
  kDirCcw = 0x20,
};

// Indexed by [state][(b << 1) | a]. Pins idle high (pull-ups), so rest is 0b11.
const uint8_t kTable[7][4] = {
    /* kStart    */ {kStart, kCwBegin, kCcwBegin, kStart},
    /* kCwFinal  */ {kCwNext, kStart, kCwFinal, kStart | kDirCw},
    /* kCwBegin  */ {kCwNext, kCwBegin, kStart, kStart},
    /* kCwNext   */ {kCwNext, kCwBegin, kCwFinal, kStart},
    /* kCcwBegin */ {kCcwNext, kStart, kCcwBegin, kStart},
    /* kCcwFinal */ {kCcwNext, kCcwFinal, kStart, kStart | kDirCcw},
    /* kCcwNext  */ {kCcwNext, kCcwFinal, kCcwBegin, kStart},
};

int clampInt(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

}  // namespace

int QuadratureDecoder::update(bool a, bool b) {
  const uint8_t pins = static_cast<uint8_t>((b ? 2 : 0) | (a ? 1 : 0));
  state_ = kTable[state_ & 0x0F][pins];
  if (state_ & kDirCw) return 1;
  if (state_ & kDirCcw) return -1;
  return 0;
}

ButtonEvent ButtonDetector::update(uint32_t nowMs, bool pressed) {
  if (pressed != raw_) {
    raw_ = pressed;
    rawChangedMs_ = nowMs;
  }
  if (raw_ != stable_ && static_cast<uint32_t>(nowMs - rawChangedMs_) >= debounceMs_) {
    stable_ = raw_;
    if (stable_) {
      pressedSinceMs_ = nowMs;
      longReported_ = false;
    }
  }
  if (stable_ && !longReported_ && static_cast<uint32_t>(nowMs - pressedSinceMs_) >= longPressMs_) {
    longReported_ = true;
    return ButtonEvent::LongPress;
  }
  return ButtonEvent::None;
}

int applyKnob(int current, int detents, int maxVolume, bool& atCap) {
  int next;
  if (current > maxVolume && detents > 0) {
    next = current;  // above the cap (set elsewhere): don't let "up" mean "suddenly quieter"
  } else {
    const int stepped = current + detents * kVolumeStep;
    next = clampInt(stepped < maxVolume ? stepped : maxVolume, 0, 100);
  }
  atCap = detents > 0 && next >= maxVolume;
  return next;
}

bool maxFromCurrentVolume(int current, int& maxVolume) {
  if (current < kMinMaxVolume || current > 100) return false;
  maxVolume = current;
  return true;
}

int gaugeLeds(int volume, int maxVolume, int ledCount) {
  if (maxVolume <= 0 || volume <= 0 || ledCount <= 0) return 0;
  const int lit = (volume * ledCount + maxVolume - 1) / maxVolume;  // round up
  return clampInt(lit, 1, ledCount);
}

}  // namespace discplayer
