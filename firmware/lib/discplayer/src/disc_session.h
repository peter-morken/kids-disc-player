#pragma once

#include <cstdint>
#include <string>

namespace discplayer {

enum class DiscAction { None, PlayNew, Resume, Pause };

// Turns a stream of "which tag UID is on the reader right now" samples into
// player commands:
//   - disc inserted            -> PlayNew, once it has stayed put for the
//                                 settle time (a disc sliding into the slot
//                                 passes the antenna before it is seated)
//   - disc removed             -> Pause (after a debounce, since reads flicker)
//   - same disc back in time   -> Resume instead of restarting
//   - disc swapped for another -> PlayNew
class DiscSession {
 public:
  DiscSession(uint32_t insertSettleMs, uint32_t removeDebounceMs, uint32_t resumeWindowMs)
      : insertSettleMs_(insertSettleMs),
        removeDebounceMs_(removeDebounceMs),
        resumeWindowMs_(resumeWindowMs) {}

  // uid is empty when no tag was detected in this poll.
  DiscAction update(uint32_t nowMs, const std::string& uid);

  // Call after a PlayNew/Resume that could not be carried out (blank disc,
  // Sonos unreachable). The disc stays "inserted" so it is not retried in a
  // loop, but removing it will not pause whatever else is playing.
  void markActiveInvalid() { activeValid_ = false; }

  const std::string& activeUid() const { return active_; }

  // A disc has been seen but is not yet settled; useful for instant feedback.
  bool settling() const { return !candidate_.empty(); }

 private:
  uint32_t insertSettleMs_;
  uint32_t removeDebounceMs_;
  uint32_t resumeWindowMs_;
  std::string active_;
  bool activeValid_ = false;
  std::string paused_;
  uint32_t lastSeenMs_ = 0;
  uint32_t removedAtMs_ = 0;
  std::string candidate_;
  uint32_t candidateSinceMs_ = 0;
  uint32_t candidateLastSeenMs_ = 0;
};

}  // namespace discplayer
