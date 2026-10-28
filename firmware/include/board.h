#pragma once
// Build-time constants. Nothing here is secret or specific to one household:
// Wi-Fi, the Sonos room and volume are entered on the device's setup page and
// stored in flash, so a single firmware build works everywhere.

// --- Firmware version and updates ---
// CI release builds pass -DFIRMWARE_VERSION_RAW=v1.2.3 (the git tag). Local
// builds are "dev" and never auto-update, so a USB-flashed test build is not
// overwritten on the next boot.
#define KDP_STR2(x) #x
#define KDP_STR(x) KDP_STR2(x)
#ifdef FIRMWARE_VERSION_RAW
#define FIRMWARE_VERSION KDP_STR(FIRMWARE_VERSION_RAW)
#else
#define FIRMWARE_VERSION "dev"
#endif

// GitHub repo whose latest release holds firmware.bin and version.txt.
#define UPDATE_REPO "peter-morken/kids-disc-player"

// --- Setup portal ---
// Open Wi-Fi network the device creates when it needs setting up.
#define SETUP_AP_NAME "DiscPlayer-Setup"
// How long the portal stays up before the device gives up and retries.
#define SETUP_PORTAL_TIMEOUT_S 300
// Knob cap until a parent changes it (setup page, or a 5 s knob press).
#define DEFAULT_MAX_VOLUME 40

// --- Sonos ---
// 1 = pull the speaker out of any group before playing. Sonos only accepts
// playback commands on a group's coordinator.
#define SONOS_UNGROUP 1
// Sonos identifies Spotify accounts by region: 2311 (EU/most) or 3079 (US).
// The other value is tried automatically if this one is rejected.
#define SPOTIFY_SERVICE_TYPE 2311

// --- PN532 wiring (I2C mode: set the board's DIP switches to I2C) ---
#define PN532_SDA 21
#define PN532_SCL 22
#define PN532_IRQ 4
#define PN532_RST 5

// Onboard LED on most ESP32 DevKit boards (inside the box; for debugging).
#define STATUS_LED_PIN 2

// --- Volume knob: EC11 rotary encoder with push switch, common pin to GND ---
#define KNOB_A_PIN 32
#define KNOB_B_PIN 33
#define KNOB_SWITCH_PIN 25
// Set to 1 if turning clockwise lowers the volume (A/B wired the other way round).
#define KNOB_REVERSED 0
// Hold the knob this long to save the speaker's current volume as the new max.
#define KNOB_LONG_PRESS_MS 5000

// --- LED ring around the knob: WS2812/NeoPixel, 5 V, data in ---
#define RING_PIN 27
#define RING_LED_COUNT 12
// 0-255. The ring sits in a bedroom; keep it dim. Also limits current draw.
#define RING_BRIGHTNESS 40

// --- Behaviour ---
// A disc must sit in the slot this long before it starts (it passes the
// antenna while sliding in, and a quick in-and-out shouldn't do anything).
#define INSERT_SETTLE_MS 400
// A disc must be missing this long before playback pauses (reads flicker).
#define REMOVE_DEBOUNCE_MS 1500
// Re-inserting the same disc within this window resumes instead of restarting.
#define RESUME_WINDOW_MS (10UL * 60UL * 1000UL)
