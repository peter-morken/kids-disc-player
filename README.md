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

- **No cloud service, no app, no accounts on the device.** The ESP32 talks to
  the speaker directly on the home network (port 1400), the same local API that
  [SoCo](https://github.com/SoCo/SoCo) and Home Assistant use. The only internet
  traffic is a firmware update check against this repo's GitHub releases at boot.
- **Set up from a phone, updated remotely.** One firmware build works in any
  home: Wi-Fi and the Sonos room are entered on the device's setup page, and
  new releases install themselves on the next power-up.
- **The disc carries its own link.** Each tag stores a Spotify URL as a
  standard NDEF record, so you never reflash the firmware to add a disc. Write a
  new one with any NFC phone app.
- **Both sides work, any rotation.** The tag sits in the centre of the disc.

## The design

The enclosure is a SolidCraft design in [`cad/`](cad): a 130 × 90 × 90 mm box with the disc slot
on top, a 24-LED ring glowing through the white front around a sun-faced
knob, and the name "Henriks musikk" inlaid in the top. It comes as one
multi-colour 3MF, an STL per part, STEP, and the SolidCraft source. Printing,
colours and assembly are in [docs/build.md](docs/build.md).

Click any STL in [`cad/stl/`](cad/stl) on GitHub to turn it in 3D.

## Repository layout

| Path | Contents |
|---|---|
| `firmware/` | PlatformIO project for the ESP32-S3 SuperMini (Arduino framework) |
| `firmware/lib/discplayer/` | Hardware-free logic: NDEF parsing, Spotify links, Sonos SOAP and speaker discovery, version comparison, insert/remove state machine. Unit-tested on the host. |
| `cad/` | The enclosure: multi-colour 3MF, one STL per part in `stl/`, STEP, and the SolidCraft source (`.sc`) |
| `docs/` | [Build guide](docs/build.md), [making discs](docs/discs.md), [setup guide for the household](docs/setup.md) |
| `tools/` | `gen_root_cas.sh` regenerates the CA certificates the updater trusts |

## Bill of materials

Per player. "In stock" means already bought for this project; the pack sizes
are what was ordered. When buying, match the spec in the Notes column.

| Part | Qty | Status | Notes |
|---|---|---|---|
| ESP32-S3 SuperMini | 1 | In stock (5-pack) | Solder wires directly to the pads; no pin headers needed |
| PN532 NFC module "V3" (red/blue, 42.7 × 40.4 mm) | 1 | In stock (5 pcs) | Must have I²C DIP switches |
| WS2812B ring, 24 LEDs, 75 mm outside / 65 mm inside | 1 | In stock | Glued into the recess behind the front wall |
| KY-040 rotary encoder module, threaded M7 bushing | 1 | In stock | Pins CLK, DT, SW, +, GND. Onboard 10 kΩ pull-ups on CLK and DT, so **+ goes to 3V3**, not 5V. Needs 20 pulses / 20 detents per turn (most KY-040s). Held by its own M7 nut and washer |
| NTAG215 round NFC stickers, at most 26.5 mm across | 1 per disc | **To buy** | NTAG213 works too. Not MIFARE Classic |
| Niimbot R4949 labels, 49 mm round | 1 per disc | **To buy** | Printed on a Niimbot B21. PET or waterproof ones last longer |
| 330 Ω resistor | 1 | Optional | In the LED ring's data line |
| Hook-up wire | about 15 short lengths | | |
| M3 machine screws, about 6–10 mm long | 10 | | Cut their own thread in 2.7 mm pilot holes: 4 countersunk for the base, 2 each for the PN532, the ESP32 clamp and the cable clamp |
| USB-C cable and 5 V phone charger | 1 | | Plug and boot must fit within **30 mm**; the cable runs through a notch sized for a 4 mm thick cable |
| PLA or PETG: white or natural | about 200 g | | Housing. Must be light: the LED ring shines through it |
| PLA or PETG: sky blue, yellow, pink, dark brown | | | Sky blue about 50 g (base and clamps), yellow about 30 g (knob), pink and dark brown a few grams (name and knob-face inlays) |
| PLA or PETG: bright colours | 15–20 g per disc | | One colour per kind of disc |

## Quick start

1. **Print** the parts in `cad/` and some discs.
   See [docs/build.md](docs/build.md#printing).
2. **Wire** the PN532 to the ESP32 over I²C. See [docs/build.md](docs/build.md#wiring).
3. **Flash once over USB** with [PlatformIO](https://platformio.org/install/cli):
   ```bash
   cd firmware && PLATFORMIO_BUILD_FLAGS=-DFIRMWARE_VERSION_RAW=v0.0.0 pio run -e esp32s3 -t upload
   ```
   Version `v0.0.0` is older than any release, so on its first boot with Wi-Fi
   the device installs the latest release by itself. That tests the update
   path at home, before the device leaves. A plain `pio run -e esp32s3 -t upload`
   gives a `dev` build, which never updates itself. Use that one for tinkering.
4. **Set it up** from a phone: join the `DiscPlayer-Setup` Wi-Fi network, then
   choose the home Wi-Fi and enter the Sonos room name. See [docs/setup.md](docs/setup.md).
   Spotify must be linked in the Sonos app.
5. **Make discs.** Write a Spotify link to each tag. See [docs/discs.md](docs/discs.md).

## Behaviour

| Event | What the player does |
|---|---|
| Disc inserted | Waits 0.4 s for it to settle, spins the ring while it starts the disc from the beginning at **half the max volume**, then flashes green |
| Disc removed for more than 1.5 s | Pauses, unless someone has since played something else from the Sonos app (then it leaves their music alone) |
| Same disc back within 10 min | Resumes where it stopped, or starts over if the speaker has played something else in between |
| Disc pulled in and out quickly | Nothing: too short to start, or too short to count as removed |
| Different disc | Starts the new one |
| End of the disc | Stops (repeat and shuffle are turned off when a disc starts) |
| Knob turned | Changes the volume in steps of 2, never above the max; the ring shows the level as a fraction of the max, and the last lit LED blinks red at the cap |
| Knob held 5 s | **Parent function:** the speaker's current volume becomes the new max (white flashes). See below |
| Blank disc, or speaker unreachable | Ring flashes red; nothing else changes |

When nothing is happening, the ring is off, because the player lives in a bedroom.

### Setting the max volume

Holding the knob for 5 s saves the speaker's **current** volume as the max.
The knob itself can't go above the max, so a toddler holding the knob can
only ever lower it. To raise it:

1. Turn the speaker up in the Sonos app, or with its own buttons.
2. Hold the knob for 5 s, until the ring flashes white.

Max volumes below 5 are rejected. The max can also be set on the setup page.
The knob's cap doesn't stop the Sonos app or the speaker's buttons. For a
hard limit, also set **Settings → System → [room] → Volume Limit** in the
Sonos app.

Pins and timings are in `firmware/include/board.h`.

## Releasing and remote updates

```bash
git tag v1.2.0 && git push origin v1.2.0
```

CI builds the firmware with that version baked in and publishes `firmware.bin`
and `version.txt` as the latest GitHub release. At every boot, a device running
a release build compares its version with `version.txt`. If the release is
newer, it downloads the image over HTTPS, writes it to its spare app slot,
verifies it, and restarts into it. Unplugging and replugging the device is
enough to update it.

Rules that keep a device in another city reachable:

- **Test every release on a second device at home first.** The update check
  runs right after Wi-Fi connects, before the NFC and Sonos code, so a release
  that crashes later in startup can still be replaced. A release that breaks
  Wi-Fi or the updater itself cannot be fixed without USB.
- **Versions only go up.** A device never installs an older version. To roll
  back, release the old code under a higher version number.
- **Don't change the partition table** (`board_build.partitions`) in a
  release. That only works over USB.
- **Update failures are harmless.** If GitHub is unreachable, or the download is
  interrupted, the device keeps running its current firmware and tries again on
  the next boot.

## Development

```bash
cd firmware
pio test -e native     # unit tests for lib/discplayer, no hardware needed
pio run -e esp32s3    # compile firmware
```

CI runs both on every push.

## Known limitations and risks

- **Not yet tested end to end on hardware.** The logic is unit-tested, and the
  Sonos URIs and metadata match SoCo's `ShareLinkPlugin` byte for byte (the
  tests check this). Still to check on a first real build: the PN532 timing,
  the KY-040's clicks (one volume step each), which LED the ring's gauge
  starts on and which way it fills, and the print fit (disc in the slot,
  modules in their mounts, the USB-C cable in its 4 mm notch).
- **The Sonos local API is undocumented.** Sonos has kept it stable for years,
  and Home Assistant depends on it, but they could change it.
- **One speaker, found by room name.** The device looks the room up on each
  boot, and again after a failed command, so the speaker's IP may change. With
  `SONOS_UNGROUP` on (the default), the speaker leaves any group before
  playing, because Sonos only accepts commands on a group's coordinator.
- **The setup network is open (no password).** Anyone in Wi-Fi range while it
  is up could join it and read the Wi-Fi password being typed in. It only
  appears when the device needs setting up, and closes after 5 minutes.
- **Updates trust whoever controls this GitHub repo.** Turn on two-factor
  authentication for the account.
- **"Is the speaker still playing our disc?" is a heuristic.** It compares
  the queue's length and first track with what the disc put there. If someone
  replaces the queue with exactly the same album, the device can't tell the
  difference. Not yet verified against a real speaker.
- **Spotify only.** Short `spotify.link/...` URLs are not supported, because
  they need an HTTP redirect to resolve. Use the full `open.spotify.com` link.
  Artist pages can't be queued.

## License

[MIT](LICENSE). Covers the firmware, CAD models and docs.
