#include <Adafruit_PN532.h>
#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>

#include <functional>

#include "board.h"
#include "disc_session.h"
#include "ndef.h"
#include "ota.h"
#include "settings.h"
#include "sonos.h"
#include "spotify.h"

using namespace discplayer;

namespace {

constexpr uint32_t kPollIntervalMs = 150;
constexpr uint16_t kTagDetectTimeoutMs = 50;
constexpr uint8_t kFirstUserPage = 4;
constexpr uint8_t kMaxPagesToRead = 36;  // 144 bytes: all of NTAG213, plenty for one URL

Adafruit_PN532 nfc(PN532_IRQ, PN532_RST, &Wire);
DiscSession session(REMOVE_DEBOUNCE_MS, RESUME_WINDOW_MS);
SonosClient sonos;
PlayerSettings settings;
uint32_t lastPollMs = 0;

void blink(int times) {
  for (int i = 0; i < times; ++i) {
    digitalWrite(STATUS_LED_PIN, HIGH);
    delay(120);
    digitalWrite(STATUS_LED_PIN, LOW);
    delay(120);
  }
}

// Runs a Sonos command. If it fails, the speaker may have a new IP address,
// so look it up again and retry once.
bool withSpeaker(const std::function<bool()>& command) {
  for (int attempt = 0; attempt < 2; ++attempt) {
    if (!sonos.ensureSpeaker(settings.room)) return false;
    if (command()) {
      stopSetupPortal();  // speaker reachable after all; no setup needed
      return true;
    }
    sonos.forget();
  }
  return false;
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
  return withSpeaker([&] { return sonos.playSpotify(ref, settings.startVolume); });
}

}  // namespace

void setup() {
  Serial.begin(115200);
  pinMode(STATUS_LED_PIN, OUTPUT);
  Serial.printf("\nKids Disc Player %s\n", FIRMWARE_VERSION);

  settings = loadSettings();
  connectWifiOrRunSetup(settings);

  // Before anything else that could crash, so a broken release can still be
  // replaced by a fixed one without physical access.
  checkForUpdate();

  Wire.begin(PN532_SDA, PN532_SCL);
  nfc.begin();
  const uint32_t version = nfc.getFirmwareVersion();
  if (!version) {
    Serial.println("PN532 not found. Check wiring and that the DIP switches select I2C.");
    for (;;) blink(5), delay(1000);
  }
  Serial.printf("PN532 firmware %u.%u\n", (version >> 16) & 0xFF, (version >> 8) & 0xFF);
  nfc.SAMConfig();

  if (settings.room.empty()) {
    startSetupPortal(settings, "no Sonos room configured");
  } else if (!sonos.ensureSpeaker(settings.room)) {
    // Maybe the speaker is just switched off: keep running and retry on the
    // next disc, but let someone on site correct the room name meanwhile.
    startSetupPortal(settings, "Sonos room not found");
  }
}

void loop() {
  serviceSetupPortal(settings);

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
    ok = withSpeaker([] { return sonos.resume(); });
  } else if (action == DiscAction::Pause) {
    Serial.println("Disc removed, pausing");
    ok = withSpeaker([] { return sonos.pause(); });
  }

  if (!ok) {
    if (action != DiscAction::Pause) session.markActiveInvalid();
    blink(3);
  }
}
