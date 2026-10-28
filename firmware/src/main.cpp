#include <Adafruit_PN532.h>
#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>

#include <functional>

#include "board.h"
#include "controls.h"
#include "disc_session.h"
#include "input.h"
#include "ndef.h"
#include "ota.h"
#include "ring.h"
#include "settings.h"
#include "sonos.h"
#include "spotify.h"

using namespace discplayer;

namespace {

constexpr uint32_t kPollIntervalMs = 150;
constexpr uint16_t kTagDetectTimeoutMs = 50;
constexpr uint8_t kFirstUserPage = 4;
constexpr uint8_t kMaxPagesToRead = 36;  // 144 bytes: all of NTAG213, plenty for one URL
constexpr int kTagReadAttempts = 3;
// Knob turns are sent to Sonos at most this often; detents in between add up.
constexpr uint32_t kVolumeSendIntervalMs = 150;
// After this long without turning, re-read the volume before the next turn
// (it may have changed from the Sonos app or the speaker's buttons).
constexpr uint32_t kVolumeStaleMs = 3000;

Adafruit_PN532 nfc(PN532_IRQ, PN532_RST, &Wire);
DiscSession session(INSERT_SETTLE_MS, REMOVE_DEBOUNCE_MS, RESUME_WINDOW_MS);
SonosClient sonos;
PlayerSettings settings;
uint32_t lastPollMs = 0;

int knobDetents = 0;
int knownVolume = -1;  // -1: unknown, read before the next change
uint32_t lastKnobMs = 0;
uint32_t lastVolumeSendMs = 0;

RingState idleRing() { return setupPortalActive() ? RingState::Setup : RingState::Idle; }

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
bool readTagLinkOnce(std::string& link) {
  uint8_t data[kMaxPagesToRead * 4];
  for (uint8_t i = 0; i < kMaxPagesToRead; ++i) {
    if (!nfc.ntag2xx_ReadPage(kFirstUserPage + i, data + i * 4)) return false;
    if (readFirstNdefString(data, (i + 1) * 4, link)) return true;
  }
  return false;
}

bool readTagLink(std::string& link) {
  for (int attempt = 0; attempt < kTagReadAttempts; ++attempt) {
    if (readTagLinkOnce(link)) return true;
    delay(50);
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
  knownVolume = -1;
  return withSpeaker([&] { return sonos.playSpotify(ref, startVolumeFor(settings.maxVolume)); });
}

bool resumeDisc() {
  // If someone played something else from the Sonos app meanwhile, "resume"
  // would continue their music: start the disc over instead.
  if (sonos.ensureSpeaker(settings.room) && sonos.stillOurs()) {
    Serial.println("Resuming");
    return withSpeaker([] { return sonos.resume(); });
  }
  Serial.println("Speaker has moved on; starting the disc over");
  return startDisc();
}

bool pauseDisc() {
  return withSpeaker([] {
    if (!sonos.stillOurs()) {
      Serial.println("Disc removed, but the speaker is playing something else; leaving it");
      return true;
    }
    Serial.println("Disc removed, pausing");
    return sonos.pause();
  });
}

void handleKnob(uint32_t now) {
  knobDetents += takeKnobDetents();
  if (knobDetents == 0 || now - lastVolumeSendMs < kVolumeSendIntervalMs) return;

  if (knownVolume < 0 || now - lastKnobMs > kVolumeStaleMs) {
    if (!sonos.ensureSpeaker(settings.room) || !sonos.getVolume(knownVolume)) {
      Serial.println("Knob turned, but the speaker can't be reached");
      knobDetents = 0;
      knownVolume = -1;
      ringFlashError();
      return;
    }
  }

  bool atCap = false;
  const int next = applyKnob(knownVolume, knobDetents, settings.maxVolume, atCap);
  knobDetents = 0;
  lastKnobMs = lastVolumeSendMs = now;
  if (next != knownVolume && !sonos.setVolume(next)) {
    knownVolume = -1;
    sonos.forget();
    ringFlashError();
    return;
  }
  knownVolume = next;
  ringShowVolume(next, settings.maxVolume, atCap);
}

// Parent function: hold the knob 5 s to make the speaker's current volume the
// new max. Can only raise the max if the speaker was first turned up from the
// Sonos app or its own buttons, which a toddler at the knob can't do.
void handleLongPress() {
  if (!takeLongPress()) return;
  int current = -1, newMax = 0;
  if (!sonos.ensureSpeaker(settings.room) || !sonos.getVolume(current) || !maxFromCurrentVolume(current, newMax)) {
    Serial.printf("Long press: not saving max volume (speaker volume %d)\n", current);
    ringFlashError();
    return;
  }
  settings.maxVolume = newMax;
  saveSettings(settings);
  knownVolume = current;
  Serial.printf("New max volume %d\n", newMax);
  ringFlashMaxSaved(newMax);
}

void handleDisc(uint32_t now) {
  const std::string uid = pollTagUid();
  const DiscAction action = session.update(now, uid);
  if (action == DiscAction::None) {
    ringSetState(session.settling() ? RingState::Reading : idleRing());
    return;
  }

  bool ok = false;
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi down, ignoring disc");
  } else if (action == DiscAction::PlayNew) {
    ringSetState(RingState::Reading);
    ok = startDisc();
  } else if (action == DiscAction::Resume) {
    ringSetState(RingState::Reading);
    ok = resumeDisc();
  } else if (action == DiscAction::Pause) {
    ok = pauseDisc();
  }

  ringSetState(idleRing());
  if (ok && action != DiscAction::Pause) ringFlashOk();
  if (!ok) {
    if (action != DiscAction::Pause) session.markActiveInvalid();
    ringFlashError();
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  Serial.printf("\nKids Disc Player %s\n", FIRMWARE_VERSION);
  ringBegin();

  settings = loadSettings();
  connectWifiOrRunSetup(settings);

  // Before anything else that could crash, so a broken release can still be
  // replaced by a fixed one without physical access.
  checkForUpdate();

  inputBegin();
  Wire.begin(PN532_SDA, PN532_SCL);
  nfc.begin();
  const uint32_t version = nfc.getFirmwareVersion();
  if (!version) {
    Serial.println("PN532 not found. Check wiring and that the DIP switches select I2C.");
    for (;;) ringFlashError(), delay(2000);
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
  handleKnob(now);
  handleLongPress();

  if (now - lastPollMs < kPollIntervalMs) return;
  lastPollMs = now;
  handleDisc(now);
}
