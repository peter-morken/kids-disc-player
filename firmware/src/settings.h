#pragma once

#include <string>

// Per-household settings, entered on the setup page and kept in flash (NVS).
// Wi-Fi credentials are stored separately by the ESP32 Wi-Fi stack.
struct PlayerSettings {
  std::string room;  // Sonos room name, as shown in the Sonos app
  int startVolume;   // 0-100, or -1 to leave the volume alone
};

PlayerSettings loadSettings();

// Connects to the saved Wi-Fi. On first boot, or if the saved network can't be
// reached, opens the setup portal instead and blocks until someone completes
// it. Restarts the device if nobody does within SETUP_PORTAL_TIMEOUT_S.
void connectWifiOrRunSetup(PlayerSettings& settings);

// Opens the setup portal in the background, e.g. when the Sonos room can't be
// found. The player keeps working meanwhile; call serviceSetupPortal() from
// loop(). Saving the page stores the settings and restarts the device.
void startSetupPortal(PlayerSettings& settings, const char* reason);
void serviceSetupPortal(PlayerSettings& settings);
void stopSetupPortal();
bool setupPortalActive();
