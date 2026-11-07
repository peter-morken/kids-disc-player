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

// Pins below are for the ESP32-S3 SuperMini, which exposes GPIO 1-13. GPIO 3
// is a strapping pin and stays unused. GPIO 8/9 are the S3's default I2C pins.

// --- PN532 wiring (I2C mode: set the board's DIP switches to I2C) ---
#define PN532_SDA 8
#define PN532_SCL 9
#define PN532_IRQ 7
#define PN532_RST 6

// --- Volume knob: KY-040 rotary encoder module (threaded M7 bushing) ---
// Wire CLK -> A, DT -> B, SW -> switch, + -> 3V3 (never 5V: the module's own
// 10k pull-ups on CLK and DT would then pull the ESP32's pins to 5 V), GND.
// The internal pull-ups are switched on too; in parallel with the module's
// they're harmless, and SW needs them (most KY-040s fit no pull-up there).
// Needs a 20-pulse / 20-detent encoder (one full A/B cycle per click).
#define KNOB_A_PIN 4
#define KNOB_B_PIN 5
#define KNOB_SWITCH_PIN 10
// Set to 1 if turning clockwise lowers the volume (A/B wired the other way round).
#define KNOB_REVERSED 0
// Hold the knob this long to save the speaker's current volume as the new max.
#define KNOB_LONG_PRESS_MS 5000

// --- LED ring around the knob: WS2812B/NeoPixel, 24 LEDs, 75 mm, 5 V, data in ---
#define RING_PIN 2
#define RING_LED_COUNT 24
// Where the volume gauge starts and which way it fills, so it reads like a
// dial however the ring ended up glued in. The gauge starts at physical LED
// RING_FIRST_LED_OFFSET (LED 0 is the one next to DIN) and fills clockwise
// as seen from the front. Set RING_REVERSED to 1 if it fills anticlockwise.
// See docs/build.md, "Lining up the LED ring".
#define RING_FIRST_LED_OFFSET 0
#define RING_REVERSED 0
// 0-255. The ring sits in a bedroom; keep it dim. Also limits current draw:
// at 40, all 24 LEDs on full white draw about 250 mA (checked at compile
// time in ring.cpp).
#define RING_BRIGHTNESS 40

// --- Behaviour ---
// A disc must sit in the slot this long before it starts (it passes the
// antenna while sliding in, and a quick in-and-out shouldn't do anything).
#define INSERT_SETTLE_MS 400
// A disc must be missing this long before playback pauses (reads flicker).
#define REMOVE_DEBOUNCE_MS 1500
// Re-inserting the same disc within this window resumes instead of restarting.
#define RESUME_WINDOW_MS (10UL * 60UL * 1000UL)
