#include "settings.h"

#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiManager.h>

#include <cctype>
#include <vector>

#include "board.h"

namespace {

const char* kNamespace = "player";

WiFiManager wm;
char roomValue[64];
char volumeValue[8];
WiFiManagerParameter roomParam("room", "Sonos room name (exactly as in the Sonos app)", "", sizeof(roomValue) - 1);
WiFiManagerParameter volumeParam("volume", "Start volume 0-100 (-1 = don't change)", "", sizeof(volumeValue) - 1);
bool paramsAdded = false;
bool paramsSaved = false;

void saveSettings(const PlayerSettings& s) {
  Preferences prefs;
  prefs.begin(kNamespace, false);
  prefs.putString("room", s.room.c_str());
  prefs.putInt("volume", s.startVolume);
  prefs.end();
}

void prepareParams(const PlayerSettings& s) {
  snprintf(roomValue, sizeof(roomValue), "%s", s.room.c_str());
  snprintf(volumeValue, sizeof(volumeValue), "%d", s.startVolume);
  roomParam.setValue(roomValue, sizeof(roomValue) - 1);
  volumeParam.setValue(volumeValue, sizeof(volumeValue) - 1);
  if (paramsAdded) return;
  wm.addParameter(&roomParam);
  wm.addParameter(&volumeParam);
  wm.setSaveParamsCallback([] { paramsSaved = true; });
  wm.setTitle("Disc Player setup");
  // "param" is the Setup page (room/volume without re-entering Wi-Fi). No
  // "update": that page would let anyone on the open setup network flash code.
  static std::vector<const char*> menu = {"wifi", "param", "info", "restart", "exit"};
  wm.setMenu(menu);
  wm.setConfigPortalTimeout(SETUP_PORTAL_TIMEOUT_S);
  wm.setConnectTimeout(20);
  paramsAdded = true;
}

// Copies the form values into settings; persists them if anything changed.
void storeParams(PlayerSettings& s) {
  PlayerSettings next = s;
  next.room = roomParam.getValue();
  while (!next.room.empty() && isspace(static_cast<unsigned char>(next.room.back()))) next.room.pop_back();
  next.startVolume = constrain(atoi(volumeParam.getValue()), -1, 100);
  if (next.room == s.room && next.startVolume == s.startVolume) return;
  s = next;
  saveSettings(s);
  Serial.printf("Saved settings: room \"%s\", start volume %d\n", s.room.c_str(), s.startVolume);
}

}  // namespace

PlayerSettings loadSettings() {
  Preferences prefs;
  PlayerSettings s{"", DEFAULT_START_VOLUME};
  if (prefs.begin(kNamespace, true)) {  // fails harmlessly on first boot
    s.room = prefs.getString("room", "").c_str();
    s.startVolume = prefs.getInt("volume", DEFAULT_START_VOLUME);
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
}

void startSetupPortal(PlayerSettings& settings, const char* reason) {
  if (wm.getConfigPortalActive()) return;
  Serial.printf("Opening setup portal \"%s\": %s\n", SETUP_AP_NAME, reason);
  prepareParams(settings);
  paramsSaved = false;
  wm.setConfigPortalBlocking(false);
  wm.startConfigPortal(SETUP_AP_NAME);
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
}

bool setupPortalActive() { return wm.getConfigPortalActive(); }
