#pragma once

#include <string>

#include "soap.h"
#include "sonos_discovery.h"
#include "spotify.h"

// Talks to one Sonos player over its local UPnP API (port 1400).
class SonosClient {
 public:
  // Finds the player for `room` on the LAN (SSDP, then the household's zone
  // topology). Returns immediately if it is already known.
  bool ensureSpeaker(const std::string& room);
  // Drops the known player, e.g. after a failed command (its IP may have changed).
  void forget() { speaker_ = {}; }

  // Replaces the queue with the disc's content and plays it from the start,
  // with repeat/shuffle off (the disc stops at its end) and the given volume.
  bool playSpotify(const discplayer::SpotifyRef& ref, int volume);
  bool resume();
  bool pause();

  // Whether the speaker is still on the queue our last playSpotify() filled.
  // False once someone plays something else from the Sonos app, so removing
  // the disc doesn't pause their music and re-inserting it starts it fresh.
  bool stillOurs();

  bool getVolume(int& volume);
  bool setVolume(int volume);

 private:
  bool discover(const std::string& room);
  bool soap(const std::string& host, const char* path, const char* urn, const char* action,
            const discplayer::SoapArgs& args, std::string* response = nullptr);
  bool avTransport(const char* action, discplayer::SoapArgs args = {}, std::string* response = nullptr);
  bool readQueueFingerprint(std::string& out);

  discplayer::SonosSpeaker speaker_;
  bool ownsQueue_ = false;
  std::string queueFingerprint_;  // empty if it couldn't be read; then only the URI is checked
};
