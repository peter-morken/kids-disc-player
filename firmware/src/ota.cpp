#include "ota.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>

#include "board.h"
#include "github_root_cas.h"
#include "version.h"

namespace {

const char* kVersionUrl = "https://github.com/" UPDATE_REPO "/releases/latest/download/version.txt";
const char* kFirmwareUrl = "https://github.com/" UPDATE_REPO "/releases/latest/download/firmware.bin";

}  // namespace

void checkForUpdate() {
  discplayer::Version current;
  if (!discplayer::parseVersion(FIRMWARE_VERSION, current)) {
    Serial.printf("Firmware %s: development build, skipping update check\n", FIRMWARE_VERSION);
    return;
  }
  Serial.printf("Firmware %s, checking for updates\n", FIRMWARE_VERSION);

  WiFiClientSecure client;
  client.setCACert(kGithubRootCAs);

  // github.com answers with a redirect to its asset CDN; follow it.
  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setUserAgent("kids-disc-player/" FIRMWARE_VERSION);
  http.setTimeout(10000);
  if (!http.begin(client, kVersionUrl)) return;
  const int code = http.GET();
  const std::string latest = code == 200 ? http.getString().c_str() : "";
  http.end();

  if (code != 200) {
    Serial.printf("Update check failed: HTTP %d\n", code);
    return;
  }
  if (!discplayer::isNewerVersion(latest, FIRMWARE_VERSION)) {
    Serial.println("Firmware is up to date");
    return;
  }

  Serial.printf("Updating to %s\n", latest.c_str());
  httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  httpUpdate.setLedPin(STATUS_LED_PIN, HIGH);
  httpUpdate.rebootOnUpdate(true);  // only reached after the image is fully written and verified
  if (httpUpdate.update(client, kFirmwareUrl) == HTTP_UPDATE_FAILED) {
    Serial.printf("Update failed (%d): %s\n", httpUpdate.getLastError(),
                  httpUpdate.getLastErrorString().c_str());
  }
}
