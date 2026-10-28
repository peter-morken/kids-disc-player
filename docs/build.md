# Build guide

## Printing

Export each part to STL from the `cad/` directory:

```bash
openscad -D 'part="shell"' -o shell.stl enclosure.scad
```

```bash
openscad -D 'part="base"' -o base.stl enclosure.scad
```

```bash
openscad -o disc.stl disc.scad
```

The `assembly` part (the default) is a preview only; don't print it.

| Part | Orientation | Notes |
|---|---|---|
| Shell | Exported upside down (top face on the bed) | No supports needed. The slot floor bridges about 5 mm. |
| Base | Flat | |
| Disc | Flat | With `embed_tag = true`, pause at the Z height OpenSCAD prints to the console (2.0 mm by default), drop the sticker in, then resume. Print the first layers in a dark colour and switch filament at the pause for a two-tone disc. |

PLA or PETG, 0.2 mm layers, 3 walls. Before printing a full shell, print one
disc and check that it slides freely in a test slot. `fit` and `slot_w`
control the clearance.

**Measure your modules first.** PN532 and ESP32 boards vary by a millimetre
or two between sellers. Set `pn_board` and `esp` in `enclosure.scad` to your
measurements.

## Wiring

Set the PN532's DIP switches to **I²C** (on the common V3 board: switch 1 ON,
switch 2 OFF).

| PN532 | ESP32 |
|---|---|
| VCC | 3V3 |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |
| IRQ | GPIO 4 |
| RSTO | GPIO 5 |

Pins can be changed in `config.h`.

## Assembly

1. Slide the PN532 up into the rails behind the slot from the open bottom of
   the shell, **antenna side towards the slot** and the pin header at the
   bottom. It stops against the underside of the top.
2. Flash and test the ESP32 *before* closing the box (see the README).
3. Lay the ESP32 on the base's cradle, USB end against the left wall, and fix
   it with foam tape or a dot of hot glue. Its header pins point down between
   the support blocks.
4. Connect the jumpers, lower the shell over the base, and screw it on from
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
