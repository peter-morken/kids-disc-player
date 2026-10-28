# Build guide

## Printing

Ready-to-print STLs are in [`cad/stl/`](../cad/stl), already in print
orientation. If you change any parameters, re-export them all with
`tools/render_previews.sh`, or export single parts from the `cad/` directory:

```bash
openscad -D 'part="shell"' -o shell.stl enclosure.scad
```

```bash
openscad -D 'part="base"' -o base.stl enclosure.scad
```

```bash
openscad -o disc.stl disc.scad
```

```bash
openscad -o knob.stl knob.scad
```

The `assembly` part (the default) is a preview only; don't print it.

| Part | Orientation | Notes |
|---|---|---|
| Shell | Exported upside down (top face on the bed) | No supports needed. The slot floor bridges about 5 mm. |
| Base | Flat | |
| Knob | Exported top face down | No supports. Friction fit on the D-shaft; if it's loose, lower `shaft_fit` in `knob.scad`. |
| Disc | Flat | With `embed_tag = true`, pause at the Z height OpenSCAD prints to the console (2.0 mm by default), drop the sticker in, then resume. Print the first layers in a dark colour and switch filament at the pause for a two-tone disc. |

**Print the shell in white or another light colour.** The LED ring shines
through a 0.8 mm window in the front wall, and dark filament blocks it.

PLA or PETG, 0.2 mm layers, 3 walls. Before printing a full shell, print one
disc and check that it slides freely in a test slot. `fit` and `slot_w`
control the clearance.

**Measure your modules first.** PN532 and ESP32 boards vary by a millimetre
or two between sellers. Set `pn_board` and `esp` in `enclosure.scad` to your
measurements.

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

**Volume knob:** a bare EC11 rotary encoder with a push switch. Solder wires directly to it.

| Encoder | ESP32-S3 SuperMini |
|---|---|
| A | 4 |
| B | 5 |
| C (middle pin of the three) | GND |
| Switch (one of the two pins) | 10 |
| Switch (the other pin) | GND |

If turning clockwise makes it quieter, set `KNOB_REVERSED 1` in `board.h`
(or swap A and B).

**LED ring:** 12 × WS2812 (NeoPixel), 50 mm across.

| Ring | ESP32-S3 SuperMini |
|---|---|
| 5V / VCC | 5V (from USB) |
| GND | GND |
| DIN (data in) | 2, ideally through a 330 Ω resistor |

GPIO 3 is left free on purpose: it's a strapping pin that affects how the
chip boots. GPIO 1, 11, 12 and 13 are free for later additions.

The firmware keeps the ring dim (`RING_BRIGHTNESS`), so it draws well under
the ~500 mA a USB port provides.

Pins can be changed in `firmware/include/board.h`.

## Assembly

1. Slide the PN532 up into the rails behind the slot from the open bottom of
   the shell, **antenna side towards the slot** and the pin header at the
   bottom. It stops against the underside of the top.
2. Mount the encoder through the hole in the front face and tighten its nut on
   the outside. Twist the encoder so its pins point down before tightening.
3. Glue the LED ring to the inside of the front wall, LEDs facing the wall so
   they sit in the circular pocket. Put LED 0 (next to DIN) at the
   7 o'clock position: the volume gauge then fills clockwise from bottom left,
   like a dial.
4. Push the knob onto the shaft.
5. Flash, set up and test the ESP32-S3 *before* closing the box (see the
   README). If the first USB flash can't find the board, hold its **BOOT**
   button while plugging in the USB cable, then flash again.
6. Lay the ESP32-S3 on the base's cradle, USB-C end against the left wall,
   and fix it with foam tape or a dot of hot glue. The cradle holds it by
   its two ends; the wires leave between the support blocks.
7. Lower the shell over the base, and screw it on from
   underneath.

## Checking read range

Watch the serial monitor while you insert a disc in both orientations. If
reads are flaky:

- Check the antenna lines up with the tag. OpenSCAD prints both positions when
  you render `enclosure.scad`. The tag centre sits 13 mm below the top face by
  default. Change `slot_depth` to move it.
- Keep `slot_wall_reader` thin (1.2 mm). Each extra millimetre of plastic
  costs read range.
- Keep metal (screws, USB plugs) away from the antenna.
