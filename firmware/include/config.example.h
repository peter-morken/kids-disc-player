#pragma once
// Copy this file to config.h (git-ignored) and fill in your values.

// --- Wi-Fi (2.4 GHz only; the ESP32 has no 5 GHz radio) ---
#define WIFI_SSID "your-wifi"
#define WIFI_PASSWORD "your-password"

// --- Sonos ---
// IP of the speaker in the kid's room. Give it a DHCP reservation in your
// router so it never changes.
#define SONOS_IP "192.168.1.50"

// 1 = pull the speaker out of any group before playing. Sonos only accepts
// playback commands on a group's coordinator, so leave this on unless you
// know the speaker is always standalone.
#define SONOS_UNGROUP 1

// Volume (0-100) to set whenever a new disc starts. -1 leaves volume alone.
// Also set a volume limit for the room in the Sonos app; that is the real cap.
#define SONOS_START_VOLUME 20

// Sonos identifies Spotify accounts by region: 2311 (EU/most) or 3079 (US).
// The other value is tried automatically if this one is rejected.
#define SPOTIFY_SERVICE_TYPE 2311

// --- PN532 wiring (I2C mode: set the board's DIP switches to I2C) ---
#define PN532_SDA 21
#define PN532_SCL 22
#define PN532_IRQ 4
#define PN532_RST 5

// Onboard LED on most ESP32 DevKit boards. Blinks on errors.
#define STATUS_LED_PIN 2

// --- Behaviour ---
// A disc must be missing this long before playback pauses (reads flicker).
#define REMOVE_DEBOUNCE_MS 1500
// Re-inserting the same disc within this window resumes instead of restarting.
#define RESUME_WINDOW_MS (10UL * 60UL * 1000UL)
