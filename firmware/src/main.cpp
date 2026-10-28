#include <Adafruit_PN532.h>
#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <Wire.h>

// Deliberately not "config.h": ESP-IDF ships headers by that name, and
// __has_include would find one of those instead.
#if __has_include("player_config.h")
#include "player_config.h"
#else
#warning "include/player_config.h not found, building with player_config.example.h placeholders"
#include "player_config.example.h"
#endif

#include "disc_session.h"
#include "ndef.h"
#include "soap.h"
#include "spotify.h"

using namespace discplayer;

namespace {

constexpr uint32_t kPollIntervalMs = 150;
constexpr uint16_t kTagDetectTimeoutMs = 50;
constexpr uint8_t kFirstUserPage = 4;
constexpr uint8_t kMaxPagesToRead = 36;  // 144 bytes: all of NTAG213, plenty for one URL

const char* kAvTransportPath = "/MediaRenderer/AVTransport/Control";
const char* kAvTransportUrn = "urn:schemas-upnp-org:service:AVTransport:1";
const char* kRenderingPath = "/MediaRenderer/RenderingControl/Control";
const char* kRenderingUrn = "urn:schemas-upnp-org:service:RenderingControl:1";

Adafruit_PN532 nfc(PN532_IRQ, PN532_RST, &Wire);
DiscSession session(REMOVE_DEBOUNCE_MS, RESUME_WINDOW_MS);
std::string rinconId;  // speaker UUID, needed to point it at its own queue
uint32_t lastPollMs = 0;

void blink(int times) {
  for (int i = 0; i < times; ++i) {
    digitalWrite(STATUS_LED_PIN, HIGH);
    delay(120);
    digitalWrite(STATUS_LED_PIN, LOW);
    delay(120);
  }
}

String sonosUrl(const char* path) { return String("http://") + SONOS_IP + ":1400" + path; }

bool soap(const char* path, const char* urn, const char* action, const SoapArgs& args) {
  HTTPClient http;
  http.begin(sonosUrl(path));
  http.setTimeout(4000);
  http.addHeader("Content-Type", "text/xml; charset=\"utf-8\"");
  http.addHeader("SOAPACTION", String("\"") + urn + "#" + action + "\"");
  const std::string body = buildSoapEnvelope(urn, action, args);
  const int code = http.POST(reinterpret_cast<uint8_t*>(const_cast<char*>(body.data())), body.size());
  const bool ok = code == 200;
  if (!ok) Serial.printf("Sonos %s failed: HTTP %d %s\n", action, code, http.getString().c_str());
  http.end();
  return ok;
}

bool avTransport(const char* action, SoapArgs args = {}) {
  args.insert(args.begin(), SoapArgs::value_type("InstanceID", "0"));
  return soap(kAvTransportPath, kAvTransportUrn, action, args);
}

bool ensureRinconId() {
  if (!rinconId.empty()) return true;
  HTTPClient http;
  http.begin(sonosUrl("/xml/device_description.xml"));
  http.setTimeout(4000);
  const int code = http.GET();
  const std::string xml = code == 200 ? http.getString().c_str() : "";
  http.end();
  if (!extractRinconId(xml, rinconId)) {
    Serial.printf("Could not read Sonos device description from %s (HTTP %d)\n", SONOS_IP, code);
    return false;
  }
  Serial.printf("Sonos speaker: %s\n", rinconId.c_str());
  return true;
}

bool playSpotify(const SpotifyRef& ref) {
  if (!ensureRinconId()) return false;

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
                   {{"CurrentURI", "x-rincon-queue:" + rinconId + "#0"}, {"CurrentURIMetaData", ""}}))
    return false;
  avTransport("Seek", {{"Unit", "TRACK_NR"}, {"Target", "1"}});

#if SONOS_START_VOLUME >= 0
  soap(kRenderingPath, kRenderingUrn, "SetVolume",
       {{"InstanceID", "0"}, {"Channel", "Master"}, {"DesiredVolume", std::to_string(SONOS_START_VOLUME)}});
#endif
  return avTransport("Play", {{"Speed", "1"}});
}

// Reads the tag's NDEF data page by page until a complete record parses.
bool readTagLink(std::string& link) {
  uint8_t data[kMaxPagesToRead * 4];
  for (uint8_t i = 0; i < kMaxPagesToRead; ++i) {
    if (!nfc.ntag2xx_ReadPage(kFirstUserPage + i, data + i * 4)) return false;
    if (readFirstNdefString(data, (i + 1) * 4, link)) return true;
  }
  return false;
}

std::string pollTagUid() {
  uint8_t uid[7];
  uint8_t uidLen = 0;
  if (!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLen, kTagDetectTimeoutMs)) return "";
  char hex[2 * sizeof(uid) + 1] = {0};
  for (uint8_t i = 0; i < uidLen && i < sizeof(uid); ++i) sprintf(hex + 2 * i, "%02X", uid[i]);
  return hex;
}

bool startDisc() {
  std::string link;
  if (!readTagLink(link)) {
    Serial.println("Disc has no readable NDEF URL/text record");
    return false;
  }
  SpotifyRef ref;
  if (!parseSpotifyLink(link, ref)) {
    Serial.printf("Not a Spotify track/album/playlist link: %s\n", link.c_str());
    return false;
  }
  Serial.printf("Playing spotify:%s:%s\n", kindName(ref.kind), ref.id.c_str());
  return playSpotify(ref);
}

void connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
    delay(250);
  }
  digitalWrite(STATUS_LED_PIN, LOW);
  Serial.printf(" connected, IP %s\n", WiFi.localIP().toString().c_str());
}

}  // namespace

void setup() {
  Serial.begin(115200);
  pinMode(STATUS_LED_PIN, OUTPUT);

  Wire.begin(PN532_SDA, PN532_SCL);
  nfc.begin();
  const uint32_t version = nfc.getFirmwareVersion();
  if (!version) {
    Serial.println("PN532 not found. Check wiring and that the DIP switches select I2C.");
    for (;;) blink(5), delay(1000);
  }
  Serial.printf("PN532 firmware %u.%u\n", (version >> 16) & 0xFF, (version >> 8) & 0xFF);
  nfc.SAMConfig();

  connectWifi();
  ensureRinconId();
}

void loop() {
  const uint32_t now = millis();
  if (now - lastPollMs < kPollIntervalMs) return;
  lastPollMs = now;

  const std::string uid = pollTagUid();
  const DiscAction action = session.update(now, uid);
  if (action == DiscAction::None) return;

  bool ok = false;
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi down, ignoring disc");
  } else if (action == DiscAction::PlayNew) {
    ok = startDisc();
  } else if (action == DiscAction::Resume) {
    Serial.println("Resuming");
    ok = avTransport("Play", {{"Speed", "1"}});
  } else if (action == DiscAction::Pause) {
    Serial.println("Disc removed, pausing");
    ok = avTransport("Pause");
  }

  if (!ok) {
    if (action != DiscAction::Pause) session.markActiveInvalid();
    blink(3);
  }
}
