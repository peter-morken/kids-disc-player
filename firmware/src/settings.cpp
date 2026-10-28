#include "settings.h"

#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiManager.h>

#include <cctype>
#include <vector>

#include "board.h"
#include "controls.h"
#include "ring.h"

using discplayer::kMinMaxVolume;

namespace {

const char* kNamespace = "player";

WiFiManager wm;
char roomValue[64];
char volumeValue[8];
WiFiManagerParameter roomParam("room", "Sonos room name (exactly as in the Sonos app)", "", sizeof(roomValue) - 1);
WiFiManagerParameter volumeParam("maxvol", "Max volume 5-100 (each disc starts at half of it)", "", sizeof(volumeValue) - 1);
bool paramsAdded = false;
bool paramsSaved = false;

void prepareParams(const PlayerSettings& s) {
  snprintf(roomValue, sizeof(roomValue), "%s", s.room.c_str());
  snprintf(volumeValue, sizeof(volumeValue), "%d", s.maxVolume);
  roomParam.setValue(roomValue, sizeof(roomValue) - 1);
  volumeParam.setValue(volumeValue, sizeof(volumeValue) - 1);
  if (paramsAdded) return;
  wm.addParameter(&roomParam);
  wm.addParameter(&volumeParam);
  wm.setSaveParamsCallback([] { paramsSaved = true; });
  wm.setTitle("Disc Player setup");
  // "param" is the Setup page (room/max volume without re-entering Wi-Fi). No
  // "update": that page would let anyone on the open setup network flash code.
  static std::vector<const char*> menu = {"wifi", "param", "info", "restart", "exit"};
  wm.setMenu(menu);
  wm.setAPCallback([](WiFiManager*) { ringSetState(RingState::Setup); });
  wm.setConfigPortalTimeout(SETUP_PORTAL_TIMEOUT_S);
  wm.setConnectTimeout(20);
  paramsAdded = true;
}

// Copies the form values into settings; persists them if anything changed.
void storeParams(PlayerSettings& s) {
  PlayerSettings next = s;
  next.room = roomParam.getValue();
  while (!next.room.empty() && isspace(static_cast<unsigned char>(next.room.back()))) next.room.pop_back();
  next.maxVolume = constrain(atoi(volumeParam.getValue()), kMinMaxVolume, 100);
  if (next.room == s.room && next.maxVolume == s.maxVolume) return;
  s = next;
  saveSettings(s);
  Serial.printf("Saved settings: room \"%s\", max volume %d\n", s.room.c_str(), s.maxVolume);
}

}  // namespace

void saveSettings(const PlayerSettings& s) {
  Preferences prefs;
  prefs.begin(kNamespace, false);
  prefs.putString("room", s.room.c_str());
  prefs.putInt("maxvol", s.maxVolume);
  prefs.end();
}

PlayerSettings loadSettings() {
  Preferences prefs;
  PlayerSettings s{"", DEFAULT_MAX_VOLUME};
  if (prefs.begin(kNamespace, true)) {  // fails harmlessly on first boot
    s.room = prefs.getString("room", "").c_str();
    s.maxVolume = constrain(prefs.getInt("maxvol", DEFAULT_MAX_VOLUME), kMinMaxVolume, 100);
    prefs.end();
  }
  return s;
}

void connectWifiOrRunSetup(PlayerSettings& settings) {
  WiFi.mode(WIFI_STA);
  prepareParams(settings);
  Serial.println("Connecting to Wi-Fi (setup portal opens if that fails)");
  const bool connected = wm.autoConnect(SETUP_AP_NAME);
  storeParams(settings);  // no-op unless the portal ran and values changed
  if (!connected) {
    Serial.println("No Wi-Fi and setup not completed, restarting");
    ESP.restart();
  }
  Serial.printf("Wi-Fi connected, IP %s\n", WiFi.localIP().toString().c_str());
  ringSetState(RingState::Idle);
}

void startSetupPortal(PlayerSettings& settings, const char* reason) {
  if (wm.getConfigPortalActive()) return;
  Serial.printf("Opening setup portal \"%s\": %s\n", SETUP_AP_NAME, reason);
  prepareParams(settings);
  paramsSaved = false;
  wm.setConfigPortalBlocking(false);
  wm.startConfigPortal(SETUP_AP_NAME);
  ringSetState(RingState::Setup);
}

void serviceSetupPortal(PlayerSettings& settings) {
  if (!wm.getConfigPortalActive()) return;
  wm.process();
  if (!paramsSaved) return;
  storeParams(settings);
  Serial.println("Setup saved, restarting");
  delay(1000);  // let the "saved" page reach the phone
  ESP.restart();
}

void stopSetupPortal() {
  if (!wm.getConfigPortalActive()) return;
  Serial.println("Closing setup portal");
  wm.stopConfigPortal();
  ringSetState(RingState::Idle);
}

bool setupPortalActive() { return wm.getConfigPortalActive(); }
