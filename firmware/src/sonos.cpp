#include "sonos.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiUdp.h>

#include <algorithm>
#include <vector>

#include "board.h"

using namespace discplayer;

namespace {

const char* kAvTransportPath = "/MediaRenderer/AVTransport/Control";
const char* kAvTransportUrn = "urn:schemas-upnp-org:service:AVTransport:1";
const char* kRenderingPath = "/MediaRenderer/RenderingControl/Control";
const char* kRenderingUrn = "urn:schemas-upnp-org:service:RenderingControl:1";
const char* kTopologyPath = "/ZoneGroupTopology/Control";
const char* kTopologyUrn = "urn:schemas-upnp-org:service:ZoneGroupTopology:1";

constexpr uint16_t kSsdpLocalPort = 50000;
constexpr uint32_t kSsdpListenMs = 1500;
constexpr size_t kMaxPlayersToAsk = 3;

}  // namespace

bool SonosClient::soap(const std::string& host, const char* path, const char* urn, const char* action,
                       const SoapArgs& args, std::string* response) {
  HTTPClient http;
  http.begin(String("http://") + host.c_str() + ":1400" + path);
  http.setTimeout(4000);
  http.addHeader("Content-Type", "text/xml; charset=\"utf-8\"");
  http.addHeader("SOAPACTION", String("\"") + urn + "#" + action + "\"");
  const std::string body = buildSoapEnvelope(urn, action, args);
  const int code = http.POST(reinterpret_cast<uint8_t*>(const_cast<char*>(body.data())), body.size());
  const bool ok = code == 200;
  if (ok && response) {
    *response = http.getString().c_str();
  } else if (!ok) {
    Serial.printf("Sonos %s on %s failed: HTTP %d %s\n", action, host.c_str(), code, http.getString().c_str());
  }
  http.end();
  return ok;
}

bool SonosClient::avTransport(const char* action, SoapArgs args) {
  args.insert(args.begin(), SoapArgs::value_type("InstanceID", "0"));
  return soap(speaker_.host, kAvTransportPath, kAvTransportUrn, action, args);
}

bool SonosClient::discover(const std::string& room) {
  std::vector<std::string> hosts;
  WiFiUDP udp;
  if (!udp.begin(kSsdpLocalPort)) return false;
  for (int attempt = 0; attempt < 2 && hosts.empty(); ++attempt) {
    udp.beginPacket(IPAddress(239, 255, 255, 250), 1900);
    udp.write(reinterpret_cast<const uint8_t*>(kSonosSsdpSearch), strlen(kSonosSsdpSearch));
    udp.endPacket();

    const uint32_t start = millis();
    while (millis() - start < kSsdpListenMs) {
      if (udp.parsePacket() <= 0) {
        delay(10);
        continue;
      }
      char buf[1024];
      const int len = udp.read(buf, sizeof(buf) - 1);
      if (len <= 0) continue;
      buf[len] = '\0';
      std::string host;
      if (parseSsdpSonosHost(buf, host) && std::find(hosts.begin(), hosts.end(), host) == hosts.end())
        hosts.push_back(host);
    }
  }
  udp.stop();

  if (hosts.empty()) {
    Serial.println("No Sonos players answered on this network");
    return false;
  }
  // Any player can describe the whole household; ask a few in case one is busy.
  for (size_t i = 0; i < hosts.size() && i < kMaxPlayersToAsk; ++i) {
    std::string state;
    if (!soap(hosts[i], kTopologyPath, kTopologyUrn, "GetZoneGroupState", {}, &state)) continue;
    if (findRoomInZoneGroupState(state, room, speaker_)) {
      Serial.printf("Sonos room \"%s\" is %s (%s)\n", room.c_str(), speaker_.host.c_str(), speaker_.uuid.c_str());
      return true;
    }
    Serial.printf("No Sonos room named \"%s\" in this household\n", room.c_str());
    return false;
  }
  return false;
}

bool SonosClient::ensureSpeaker(const std::string& room) {
  if (!speaker_.host.empty()) return true;
  if (room.empty()) return false;
  return discover(room);
}

bool SonosClient::playSpotify(const SpotifyRef& ref, int startVolume) {
  if (speaker_.host.empty()) return false;

#if SONOS_UNGROUP
  avTransport("BecomeCoordinatorOfStandaloneGroup");  // harmless if already standalone
#endif
  if (!avTransport("RemoveAllTracksFromQueue")) return false;

  bool queued = false;
  for (unsigned svc : {static_cast<unsigned>(SPOTIFY_SERVICE_TYPE),
                       SPOTIFY_SERVICE_TYPE == kSpotifyServiceUS ? kSpotifyServiceEU : kSpotifyServiceUS}) {
    const SonosEnqueue e = buildSpotifyEnqueue(ref, svc);
    queued = avTransport("AddURIToQueue", {{"EnqueuedURI", e.uri},
                                           {"EnqueuedURIMetaData", e.metadata},
                                           {"DesiredFirstTrackNumberEnqueued", "0"},
                                           {"EnqueueAsNext", "0"}});
    if (queued) break;
  }
  if (!queued) return false;

  if (!avTransport("SetAVTransportURI",
                   {{"CurrentURI", "x-rincon-queue:" + speaker_.uuid + "#0"}, {"CurrentURIMetaData", ""}}))
    return false;
  avTransport("Seek", {{"Unit", "TRACK_NR"}, {"Target", "1"}});

  if (startVolume >= 0) {
    soap(speaker_.host, kRenderingPath, kRenderingUrn, "SetVolume",
         {{"InstanceID", "0"}, {"Channel", "Master"}, {"DesiredVolume", std::to_string(startVolume)}});
  }
  return avTransport("Play", {{"Speed", "1"}});
}

bool SonosClient::resume() { return !speaker_.host.empty() && avTransport("Play", {{"Speed", "1"}}); }

bool SonosClient::pause() { return !speaker_.host.empty() && avTransport("Pause"); }
