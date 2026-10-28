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

  bool playSpotify(const discplayer::SpotifyRef& ref, int startVolume);
  bool resume();
  bool pause();

 private:
  bool discover(const std::string& room);
  bool soap(const std::string& host, const char* path, const char* urn, const char* action,
            const discplayer::SoapArgs& args, std::string* response = nullptr);
  bool avTransport(const char* action, discplayer::SoapArgs args = {});

  discplayer::SonosSpeaker speaker_;
};
