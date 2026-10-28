#include "disc_session.h"

namespace discplayer {

DiscAction DiscSession::update(uint32_t nowMs, const std::string& uid) {
  if (!uid.empty()) {
    lastSeenMs_ = nowMs;
    if (uid == active_) return DiscAction::None;

    const bool resume = active_.empty() && uid == paused_ &&
                        static_cast<uint32_t>(nowMs - removedAtMs_) <= resumeWindowMs_;
    active_ = uid;
    activeValid_ = true;
    paused_.clear();
    return resume ? DiscAction::Resume : DiscAction::PlayNew;
  }

  if (active_.empty() || static_cast<uint32_t>(nowMs - lastSeenMs_) < removeDebounceMs_)
    return DiscAction::None;

  const bool wasValid = activeValid_;
  if (wasValid) {
    paused_ = active_;
    removedAtMs_ = nowMs;
  }
  active_.clear();
  activeValid_ = false;
  return wasValid ? DiscAction::Pause : DiscAction::None;
}

}  // namespace discplayer
