# Kids Disc Player

A screen-free music player for small kids. Drop a 3D-printed disc into the slot
and the Sonos speaker in their room starts playing that disc's album, playlist
or song. Pull the disc out and it pauses. Put the same disc back and it resumes.

```
 disc (NTAG215 sticker holding a Spotify link)
   │  NFC
   ▼
 PN532 reader ──I²C── ESP32 ──Wi-Fi, local UPnP/SOAP──▶ Sonos speaker ──▶ Spotify
```

- **No cloud, no app, no accounts on the device.** The ESP32 talks to the
  speaker directly on your LAN (port 1400), the same local API that
  [SoCo](https://github.com/SoCo/SoCo) and Home Assistant use.
- **The disc carries its own link.** Each tag stores a Spotify URL as a
  standard NDEF record, so you never reflash the firmware to add a disc. Write a
  new one with any NFC phone app.
- **Both sides work, any rotation.** The tag sits in the centre of the disc.

## Repository layout

| Path | Contents |
|---|---|
| `firmware/` | PlatformIO project for the ESP32 (Arduino framework) |
| `firmware/lib/discplayer/` | Hardware-free logic: NDEF parsing, Spotify link parsing, Sonos SOAP building, insert/remove state machine. Unit-tested on the host. |
| `cad/` | Parametric OpenSCAD models: `disc.scad`, `enclosure.scad`, shared `params.scad` |
| `docs/` | [Build guide](docs/build.md), [making discs](docs/discs.md) |

## Bill of materials (about $15 plus filament)

| Part | Notes |
|---|---|
| ESP32 DevKit (38-pin DevKitC or 30-pin DevKit V1) | Any classic ESP32 works; adjust `esp` in `enclosure.scad` if yours differs |
| PN532 NFC module "V3" (red/blue, 42.7 × 40.4 mm) | Must support I²C (DIP switches) |
| NTAG215 or NTAG213 round stickers, 25 mm | One per disc |
| 4–6 female-female jumper wires | |
| 4 × M3 × 10 self-tapping screws | Or M3 heat-set inserts (set `pilot_d = 4.0`) |
| USB cable and 5 V phone charger | |

## Quick start

1. **Print** `cad/enclosure.scad` (shell and base) and some `cad/disc.scad`.
   See [docs/build.md](docs/build.md).
2. **Wire** the PN532 to the ESP32 over I²C. See [docs/build.md](docs/build.md#wiring).
3. **Configure**:
   ```bash
   cd firmware && cp include/config.example.h include/config.h
   ```
   Then set your Wi-Fi details and the speaker's IP. Reserve that IP in your
   router so it never changes. Also check that Spotify is linked in the Sonos app.
4. **Flash** with [PlatformIO](https://platformio.org/install/cli):
   ```bash
   pio run -e esp32dev -t upload && pio device monitor
   ```
5. **Make discs.** Write a Spotify link to each tag. See [docs/discs.md](docs/discs.md).

## Behaviour

| Event | What the player does |
|---|---|
| Disc inserted | Clears the speaker's queue, queues the disc's track, album or playlist, sets the start volume, and plays |
| Disc removed for more than 1.5 s | Pauses |
| Same disc back within 10 min | Resumes where it stopped |
| Different disc | Starts the new one |
| Blank or unreadable disc, or Sonos unreachable | LED blinks 3×; nothing else changes |

Timings and volume are in `config.h`.

## Development

```bash
cd firmware
pio test -e native     # unit tests for lib/discplayer, no hardware needed
pio run -e esp32dev    # compile firmware
```

CI runs both on every push and exports STLs for every CAD part as build artifacts.

## Known limitations and risks

- **Not yet tested end to end on hardware.** The logic is unit-tested, and the
  Sonos URIs and metadata match SoCo's `ShareLinkPlugin` byte for byte (the
  tests check this). The PN532 timing and the CAD fit still need a first real build.
- **The Sonos local API is undocumented.** Sonos has kept it stable for years,
  and Home Assistant depends on it, but they could change it.
- **One speaker, set by IP.** With `SONOS_UNGROUP` on (the default), the
  speaker leaves any group before playing, because Sonos only accepts commands
  on a group's coordinator.
- **Resume continues whatever the speaker last had queued.** If someone
  changes the music from the Sonos app in between, re-inserting the disc
  resumes their music, not the disc's.
- **Spotify only.** Short `spotify.link/...` URLs are not supported, because
  they need an HTTP redirect to resolve. Use the full `open.spotify.com` link.
  Artist pages can't be queued.

## License

[MIT](LICENSE). Covers the firmware, CAD models and docs.
