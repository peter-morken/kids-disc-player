#pragma once

#include <string>

namespace discplayer {

// SSDP search for Sonos players; send to 239.255.255.250:1900 over UDP.
extern const char kSonosSsdpSearch[];

// Host part of an http:// URL, e.g. "192.168.1.20" for
// "http://192.168.1.20:1400/xml/device_description.xml".
bool urlHost(const std::string& url, std::string& host);

// Host from the LOCATION header of an SSDP response, if it came from a Sonos player.
bool parseSsdpSonosHost(const std::string& response, std::string& host);

std::string xmlUnescape(const std::string& s);

struct SonosSpeaker {
  std::string host;  // IP address
  std::string uuid;  // RINCON_..., needed to point the speaker at its own queue
};

// Finds the controllable player for a room (Sonos app room name, matched
// case-insensitively) in a ZoneGroupTopology#GetZoneGroupState SOAP response.
// Skips invisible members, i.e. the second speaker of a stereo pair, which
// does not accept playback commands.
bool findRoomInZoneGroupState(const std::string& soapResponse, const std::string& room,
                              SonosSpeaker& out);

}  // namespace discplayer
