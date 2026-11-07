# Build guide

## Printing

The design to print is the SolidCraft enclosure in
[`cad/solidcraft/`](../cad/solidcraft):

| File | What it is |
|---|---|
| `toddler-music-box.3mf` | Every part, coloured, in one file. Start here for a multi-colour print |
| `stl/*.stl` | One file per printable part |
| `toddler-music-box.step` | The geometry for any other CAD program |
| `toddler-music-box.sc` | The editable SolidCraft source |

The box is 130 × 90 × 90 mm with 3 mm walls and the disc slot on top. The
parts are:

| Part | File | Qty | Colour | Print orientation |
|---|---|---|---|---|
| Housing | `housing.stl` | 1 | **White or natural** (the LED ring shines through it) | Top face on the bed, so the name inlay prints in the first layers |
| Name inlay "Henriks musikk" | `name-text.stl` | 1 | Pink | Same object as the housing |
| Knob | `knob.stl` | 1 | Yellow | Face down |
| Knob face (eyes and smile) | `knob-face.stl` | 1 | Dark brown | Same object as the knob |
| Base | `base.stl` | 1 | Sky blue | Flat |
| ESP32 clamp | `esp-clamp.stl` | 1 | Sky blue | Flat, recess side up |
| Cable clamp | `cable-clamp.stl` | 1 | Sky blue | Flat, recess side up |
| Disc | `disc.stl` | 1 per disc | One bright colour per category (say, red for songs, green for stories) | Flat, recess side up |

**The STLs and the 3MF are in their assembled positions, not in print
orientation.** The housing stands upright and the knob faces forwards, so
turn them in the slicer as the table says.

PLA or PETG, 0.2 mm layers, 3 walls. Solid, the parts for one player come to
about 220 cm³ (about 270 g of PLA, most of it the housing); with normal infill
expect a bit less, plus 15–20 g per disc. Before printing the housing, print
one disc and check its size against the slot.

### Inlays: one object, two parts

`name-text.stl` fills a 1 mm deep pocket in the top of the housing, and
`knob-face.stl` a 1.2 mm deep pocket in the knob's face. They print flush, in
the same layers as their host, so each inlay must be a **part of the same
object** as its host, not a separate object. Otherwise the slicer treats them as
two overlapping prints.

In **Bambu Studio** or **OrcaSlicer**, from the 3MF:

1. Open `toddler-music-box.3mf`. Every part arrives as its own object, in
   its assembled position. If the slicer asks whether to load the file as a
   single object with multiple parts, choose **No**: only the inlays belong
   together.
2. In the object list, select **Housing** and **NameText** (Ctrl-click),
   right-click, and choose **Assemble**. They become one object with two
   parts, still exactly in place. Do the same with **Knob** and **KnobFace**.
3. Turn the housing object upside down (rotate 180° about X), so its top is
   on the bed. Turn the knob object 90° about X so its face is on the bed
   (or use **Lay on face** and click the face). Check the inlay is on the bed
   too: it should show in layer 1 of the preview.
4. Assign a filament to each part in the object list (housing white,
   name pink, knob yellow, knob face dark brown). The colours shown in the
   3MF are only a guide; the slicer doesn't map them to your filaments.
5. Move the base, clamps and discs to other plates; they're single colour.

From the STLs instead: import the host and its inlay **together** (select
both files in the same Import dialog). When the slicer asks whether to load
them as a single object with multiple parts, choose **Yes**. Then continue from
step 3. The two files share a coordinate system, so the inlay lands in its
pocket. Importing them one at a time and using **Add part** may not keep that
position.

Without a multi-material printer, print the housing and knob alone. The
pockets then show as an engraved name and face; or print the inlays on their
own and glue them in.

### Previous OpenSCAD design

The parametric OpenSCAD design in `cad/*.scad` (12-LED 50 mm ring, bare EC11
encoder, 54 mm × 4 mm discs) is kept for reference. **Don't mix its parts
with the SolidCraft ones**: the discs, ring and encoder are different sizes.
CI still renders it into [`cad/stl/`](../cad/stl) and
[`cad/preview/`](../cad/preview). After changing a `.scad` file, run
`tools/render_previews.sh` and commit the result. Keep the SolidCraft STLs
out of `cad/stl/`, because that folder is regenerated from OpenSCAD.

## Wiring

Set the PN532's DIP switches to **I²C** (on the common V3 board: switch 1 ON,
switch 2 OFF).

The ESP32-S3 SuperMini's pins are labelled with bare GPIO numbers.
Check them against your board's silkscreen: clones differ.

| PN532 | ESP32-S3 SuperMini |
|---|---|
| VCC | 3V3 |
| GND | GND |
| SDA | 8 |
| SCL | 9 |
| IRQ | 7 |
| RSTO | 6 |

**Volume knob:** a KY-040 rotary encoder module with a threaded M7 bushing.

| KY-040 | ESP32-S3 SuperMini |
|---|---|
| CLK | 4 |
| DT | 5 |
| SW | 10 |
| + | **3V3** |
| GND | GND |

Power the KY-040 from **3V3, not 5V**. The module has 10 kΩ pull-ups from
CLK and DT to its + pin, so on 5V it would drive the ESP32's pins to 5 V.

The firmware also turns on the ESP32's internal pull-ups. In parallel with
the module's resistors they do no harm, and the SW pin needs them, because
most KY-040 boards leave its pull-up unfitted. The decoder expects a
**20-pulse / 20-detent** encoder, one full CLK/DT cycle per click, with both
pins high at rest. That's what most KY-040s are. It ignores contact bounce
and half-turned clicks, so no extra filtering is needed. If the volume only
changes on every second click, the encoder is a 15-pulse / 30-detent one:
replace it.

If turning clockwise makes it quieter, set `KNOB_REVERSED 1` in `board.h`
(or swap CLK and DT).

**LED ring:** 24 × WS2812B (NeoPixel), 75 mm outside, 65 mm inside.

| Ring | ESP32-S3 SuperMini |
|---|---|
| 5V / VCC | 5V (from USB) |
| GND | GND |
| DIN (data in) | 2, ideally through a 330 Ω resistor |

GPIO 3 is left free on purpose: it's a strapping pin that affects how the
chip boots. GPIO 1, 11, 12 and 13 are free for later additions.

**Power.** The firmware keeps the ring dim (`RING_BRIGHTNESS 40` of 255).
At that setting the worst case, all 24 LEDs on full white during the
"max volume saved" flash, is about 250 mA. A compile-time check in `ring.cpp`
fails the build if a change to `RING_LED_COUNT` or `RING_BRIGHTNESS` would
push it over 300 mA. The ESP32-S3 (around 100 mA on average with Wi-Fi, with
short transmit peaks of a few hundred mA) and the PN532 (around 100 mA) take
the rest of the ~500 mA a USB port provides. Normal use, a part-lit orange
gauge or the spinner, is far below that. A 1 A or bigger phone charger has
plenty of headroom.

Pins can be changed in `firmware/include/board.h`.

## Assembly

Screws are M3 machine screws driven into 2.7 mm pilot holes, where they cut
their own thread. Turn them in slowly and stop when they're snug: a
stripped plastic thread doesn't come back. 10 in total:

| Part | Screws |
|---|---|
| Base | 4, countersunk |
| PN532 | 2 |
| ESP32 clamp | 2 |
| Cable clamp | 2 |

Lengths of about 6–10 mm work; pick the shortest that bites a few
millimetres into the pilot hole.

1. **Ring.** Glue the LED ring into the recess inside the front wall, LEDs
   facing out (towards the front), with the DIN wire towards the bottom. Only
   0.8 mm of plastic sits in front of the LEDs, so don't press on the front
   while the glue sets.
2. **Encoder and knob.** Push the KY-040's bushing through the 7.4 mm hole
   in the centre of the ring from inside, pins pointing down. Fit its washer
   and M7 nut on the outside and tighten. Press the knob on: the 14 mm pocket
   behind it clears the nut.
3. **NFC reader.** Screw the PN532 to its two standoffs behind the slot,
   **antenna side towards the slot**.
4. **ESP32.** Lay the ESP32-S3 SuperMini in its cradle on the base and screw
   the ESP32 clamp over it.
5. **USB-C.** Plug the cable into the ESP32 inside the box. Loop the slack,
   clamp the cable under the cable clamp, and lay it in the 4.4 mm U-notch
   at the bottom of the end wall. The design assumes a 4 mm thick cable, and
   the plug and its boot must fit within 30 mm.
6. Flash, set up and test *before* closing the box (see the README). If the
   first USB flash can't find the board, hold its **BOOT** button while
   plugging in the USB cable, then flash again. Check the ring direction
   now (next section).
7. **Base.** Lower the housing over the base and screw it on from
   underneath. The base closes the cable notch.

## Lining up the LED ring

The volume gauge should start at the bottom and fill clockwise, like a dial.
Which LED is at the bottom depends on how the ring ended up glued in, so
`board.h` has two settings:

- `RING_FIRST_LED_OFFSET`: the LED the gauge starts from. LED 0 is the one
  next to DIN, and the numbers count along the data direction.
- `RING_REVERSED`: set to 1 if the gauge fills anticlockwise (seen from the
  front).

To set them: turn the knob all the way down, then one click up. One LED
lights; that's where the gauge starts. Keep turning up and watch which way it
fills.

- Wrong direction: set `RING_REVERSED 1`.
- Wrong start: change `RING_FIRST_LED_OFFSET`. With 24 LEDs, each step is
  15°, so 3 steps move the start an eighth of a turn. If `RING_REVERSED` is
  0, adding 1 moves the start one LED clockwise; if it's 1, one LED
  anticlockwise.

With DIN at the bottom, the defaults (`0` and `0`) start the gauge at the
bottom. The spinner follows the same settings, so it also turns clockwise.

## Checking read range

Watch the serial monitor while you insert a disc both ways round. The tag
reads through the label on one side and through the disc on the other, so the
second way is the harder one. If reads are flaky:

- Check the PN532 is screwed tight against its standoffs, antenna side
  towards the slot.
- Keep metal (screws, the USB plug, the encoder) away from the antenna.
- Use NTAG215 (or NTAG213) stickers, not MIFARE Classic, and stick them flat
  in the centre pocket.
