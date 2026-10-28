#include "disc_session.h"

namespace discplayer {
namespace {

// A settling disc may miss a read or two without starting its settle time over.
constexpr uint32_t kCandidateDropoutMs = 400;

}  // namespace

DiscAction DiscSession::update(uint32_t nowMs, const std::string& uid) {
  if (!uid.empty()) {
    lastSeenMs_ = nowMs;
    if (uid == active_) {
      candidate_.clear();
      return DiscAction::None;
    }

    if (uid != candidate_) {
      candidate_ = uid;
      candidateSinceMs_ = nowMs;
    }
    candidateLastSeenMs_ = nowMs;
    if (static_cast<uint32_t>(nowMs - candidateSinceMs_) < insertSettleMs_) return DiscAction::None;

    candidate_.clear();
    const bool resume = active_.empty() && uid == paused_ &&
                        static_cast<uint32_t>(nowMs - removedAtMs_) <= resumeWindowMs_;
    active_ = uid;
    activeValid_ = true;
    paused_.clear();
    return resume ? DiscAction::Resume : DiscAction::PlayNew;
  }

  if (!candidate_.empty() && static_cast<uint32_t>(nowMs - candidateLastSeenMs_) >= kCandidateDropoutMs)
    candidate_.clear();

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
