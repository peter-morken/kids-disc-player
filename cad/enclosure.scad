// Player enclosure, styled as a portable cassette player: piano keys along the
// top front edge, a round "speaker" with the volume knob in its centre, and a
// cassette door on the right where the disc stands, its picture facing you.
//
// The disc drops into an upright slot just behind the front wall. A U-shaped
// cut in the front wall shows the disc's picture and lets a small thumb reach
// down the disc's face, so it can be pinched from the front instead of only
// by the 14 mm that sticks up. The slot mouth is chamfered into a funnel, so a
// disc that's dropped in a little off-centre still finds the slot.
// The PN532 slides into rails right behind the slot. The LED ring glows
// through a thin window around the knob: print the shell in white or another
// light filament.
//
// Print the shell upside down (top face on the bed): no supports needed. The
// piano keys are flush with the top, so they print from the bed too.
// Print the base flat.
// Export each part with:  openscad -D 'part="shell"' -o shell.stl enclosure.scad
// "assembly" and "cutaway" are previews only. tools/render_previews.sh renders them.

include <params.scad>
use <disc.scad>
use <knob.scad>

part = "assembly";  // [shell, base, assembly, cutaway]

// --- Box ---
box = [156, 60, 83];
corner_r = 8;
wall = 2.4;
top_t = 2.4;

// --- Disc slot (disc stands upright, slot runs along X, right behind the front wall) ---
slot_cx = 113;            // centre of the slot, and of the cassette door
slot_w = disc_t + 1.2;
slot_len = disc_d + 1.5;
slot_depth = 40;          // disc sticks out disc_d - slot_depth = 14 mm to grab
slot_y = wall;            // the front wall is the slot's front wall
slot_wall = 2;            // end walls of the slot pocket
slot_wall_reader = 1.2;   // wall between disc and PN532 antenna: keep thin
slot_floor = 2;
funnel_end = 3;           // chamfer at the slot's ends: guides an off-centre drop
funnel_side = 1.5;        // chamfer along its long sides (the back is above the reader)

// --- Thumb notch: U-shaped cut down the front wall in front of the disc ---
notch_w = 32;
notch_depth = 30;         // from the top face; the wall either side and the bottom of the slot hold the disc

// --- Cassette door: recessed panel around the slot, with the tape-shell trapezoid ---
door = [68, 6, 6];        // width, gap to the bottom edge, gap to the top edge
door_r = 5;
door_depth = 0.8;
tape_trap = [44, 34, 9];  // bottom width, top width, height
reel_dx = 15;             // two tape reels, either side of the door's centre line
reel_z = 30;
reel_r = 8;
reel_hub_r = 4;

// --- Speaker: the volume knob sits in the middle of a ring of grille dimples ---
knob_x = 42;
knob_z = 37;              // height of the knob's centre above the shell's bottom edge
grille_rings = [[29, 26], [32.5, 30]];  // [radius, dimple count]
dimple_d = 2.4;
dimple_depth = 0.8;

// --- Piano keys along the top front edge, above the speaker (decorative) ---
keys = 5;
key_x = [10, 74];         // span along the front
key_gap = 1.5;
key_h = 10;               // flush with the top face, so they print from the bed
key_out = 5;              // how far they stand proud of the front face

// --- PN532 V3 module (red/blue "Elechouse" style). Measure yours. ---
pn_board = [42.7, 1.6, 40.4];  // width (x), thickness (y), height (z)
pn_lip = 1.5;                  // how far the rail lips cover the board edge
pn_rail = 2;

// --- ESP32-S3 SuperMini (about 22.5 x 18 mm). Measure yours. ---
// Solder wires straight to its pads (no pin headers) so it can sit low.
// With headers pointing down, raise esp_standoff to about 10.
esp = [22.6, 18.1];
esp_standoff = 4;
esp_block = 4;            // support blocks under the pin-free short ends
usb_hole = [13, 9];       // width (y), height (z)

// --- Base and screws (M3 x 10 self-tapping, or M3 into heat-set inserts) ---
base_t = 3;
boss_d = 8;
pilot_d = 2.5;            // 4.0 for M3 heat-set inserts
pilot_depth = 12;
screw_d = 3.4;
screw_head_d = 6.2;
screw_head_depth = 1.8;

// --- Derived ---
slot_x0 = slot_cx - slot_len / 2;
slot_y1 = slot_y + slot_w;
slot_z0 = box.z - slot_depth;
pn_y0 = slot_y1 + slot_wall_reader;  // board front face rests here
pn_z0 = box.z - top_t - pn_board.z;
esp_x0 = wall + 0.5;
esp_y0 = (box.y - esp.y) / 2;
boss_inset = wall + boss_d / 2 - 1;
boss_positions = [for (x = [boss_inset, box.x - boss_inset], y = [boss_inset, box.y - boss_inset]) [x, y]];
grille_r = max([for (g = grille_rings) g[0]]) + dimple_d / 2;
door_x0 = slot_cx - door[0] / 2;

ring_led_r = (ring_od + ring_id) / 4;  // radius of the LED centres
// A slightly taller LED just leaves the ring PCB a fraction proud of the wall, under glue.
assert(wall - ring_window_t >= ring_led_h - 0.01, "Front wall too thin to hide the LEDs in a pocket");
assert(knob_z + grille_r < box.z - key_h, "Speaker grille runs into the piano keys");
assert(knob_z - grille_r > 1, "Speaker grille runs off the bottom edge");
assert(knob_x - grille_r > corner_r, "Speaker grille runs onto the rounded corner");
assert(knob_x + grille_r < door_x0, "Speaker grille runs into the cassette door");
assert(door_x0 + door[0] < box.x - corner_r, "Cassette door runs onto the rounded corner");
assert(ring_od / 2 < grille_r, "LED ring bigger than the speaker");
assert(knob_x - ring_od / 2 > boss_inset + boss_d / 2, "LED ring hits a screw boss");
assert(knob_x + ring_od / 2 < slot_x0 - slot_wall, "LED ring hits the slot pocket");
assert(slot_x0 + slot_len + slot_wall < box.x - boss_inset - boss_d / 2 + 0.5, "Slot pocket hits a screw boss");
assert(key_x[0] > corner_r && key_x[1] < door_x0, "Piano keys run off the flat front");
assert(notch_w < door[0] - 2 * door_r, "Thumb notch wider than the cassette door");
assert(box.z - notch_depth >= slot_z0 + disc_d / 6, "Thumb notch so deep the disc can tip out");
assert(notch_w < disc_d * 0.75, "Thumb notch so wide the disc can tip out");
assert(reel_z + reel_r < slot_z0 - slot_floor && reel_z - reel_r > door[1] + 4 + tape_trap[2],
       "Tape reels overlap the slot or the trapezoid");
assert(funnel_end <= slot_wall + top_t, "Slot funnel cuts through into the box");
assert(knob_z - 6 - 8 > esp_standoff + 6, "Encoder pins hit the ESP32");
assert(slot_depth + slot_floor + esp_standoff + 8 < box.z, "ESP32 collides with slot floor");
assert(pn_z0 > esp_standoff + 6, "PN532 collides with ESP32");
echo(str("Tag centre is ", disc_d / 2 - (box.z - slot_z0), " mm from the top face; ",
         "PN532 board centre is ", -(top_t + pn_board.z / 2), " mm"));

module rounded_box(size, r) {
  hull() for (x = [r, size.x - r], y = [r, size.y - r])
    translate([x, y, 0]) cylinder(r = r, h = size.z);
}

// Rounded rectangle in the XZ plane, cut into the front face (y = 0) to depth d.
module front_recess(x0, z0, w, h, r, d) {
  translate([0, -0.01, 0]) rotate([-90, 0, 0]) linear_extrude(d + 0.01)
    hull() for (x = [x0 + r, x0 + w - r], z = [z0 + r, z0 + h - r])
      translate([x, -z]) circle(r = r);
}

// One rail, for the board edge at x = 0 with the board extending to -x.
module pn_rail_one() {
  t = pn_board.y + fit / 2;
  difference() {
    translate([-pn_lip, 0, 0]) cube([pn_lip + fit / 2 + pn_rail, t + pn_rail, pn_board.z]);
    translate([-pn_lip - 0.01, -0.01, -0.01]) cube([pn_lip + fit / 2 + 0.01, t + 0.01, pn_board.z + 0.02]);
  }
}

// A piano key on the front face, top flush with the top of the box.
// Profile: sloped bottom (prints without support either way up), rounded-off top front edge.
module piano_key(x0, w) {
  translate([x0, 0, box.z - key_h]) rotate([90, 0, 90]) linear_extrude(w)
    hull() {
      polygon([[0, 0], [0, key_h], [-key_out + 2, key_h], [-key_out, key_h - 2], [-key_out, 2]]);
      translate([-0.01, 0]) square([0.01, key_h]);
    }
}

module piano_keys() {
  w = (key_x[1] - key_x[0] - (keys - 1) * key_gap) / keys;
  for (i = [0 : keys - 1]) piano_key(key_x[0] + i * (w + key_gap), w);
}

// Funnel-shaped mouth on the slot: wide at the top face, slot-sized funnel_end below it.
module slot_funnel() {
  hull() {
    translate([slot_x0, slot_y, box.z - funnel_end]) cube([slot_len, slot_w, 0.01]);
    translate([slot_x0 - funnel_end, slot_y - funnel_side, box.z])
      cube([slot_len + 2 * funnel_end, slot_w + 2 * funnel_side, 0.01]);
  }
  translate([slot_x0 - funnel_end, slot_y - funnel_side, box.z]) cube([slot_len + 2 * funnel_end, slot_w + 2 * funnel_side, 1]);
}

module thumb_notch() {
  r = notch_w / 2;
  translate([slot_cx, -0.01, 0]) rotate([-90, 0, 0]) linear_extrude(slot_y + 0.02)
    hull() {
      translate([0, -(box.z - notch_depth + r)]) circle(r = r);
      translate([-r, -box.z - 1]) square([notch_w, 1]);
    }
}

module cassette_door() {
  h = box.z - door[1] - door[2];
  difference() {
    front_recess(door_x0, door[1], door[0], h, door_r, door_depth);
    // the cassette's lower trapezoid stays at full face height, like a raised moulding
    translate([slot_cx, 0, door[1] + 4]) rotate([90, 0, 0]) linear_extrude(1, center = true)
      polygon([[-tape_trap[0] / 2, 0], [tape_trap[0] / 2, 0],
               [tape_trap[1] / 2, tape_trap[2]], [-tape_trap[1] / 2, tape_trap[2]]]);
  }
}

// A tape reel window: a round recess with a six-toothed hub left standing in it.
module reel(x) {
  translate([x, 0, reel_z]) rotate([90, 0, 0]) translate([0, 0, -door_depth * 2])
    linear_extrude(door_depth * 2 + 0.01)
      difference() {
        circle(r = reel_r);
        difference() {
          circle(r = reel_hub_r);
          circle(r = reel_hub_r - 1.6);
          for (i = [0 : 5]) rotate(i * 60) translate([reel_hub_r - 1.8, -0.8]) square([1.2, 1.6]);
        }
      }
}

module speaker_grille() {
  for (g = grille_rings, i = [0 : g[1] - 1])
    translate([knob_x, -0.01, knob_z]) rotate([0, i * 360 / g[1], 0])
      translate([g[0], 0, 0]) rotate([-90, 0, 0]) cylinder(d = dimple_d, h = dimple_depth + 0.01, $fn = 16);
}

module shell() {
  difference() {
    union() {
      difference() {
        rounded_box(box, corner_r);
        translate([wall, wall, -0.01])
          rounded_box([box.x - 2 * wall, box.y - 2 * wall, box.z - top_t + 0.01], corner_r - wall);
      }
      piano_keys();
      // slot pocket: end walls, floor and the thin wall towards the reader
      translate([slot_x0 - slot_wall, slot_y - 0.01, slot_z0 - slot_floor])
        cube([slot_len + 2 * slot_wall, slot_w + slot_wall_reader + 0.01, slot_depth + slot_floor]);
      // PN532 rails: slide the board up from below until it hits the top
      translate([slot_cx + pn_board.x / 2, pn_y0, pn_z0]) pn_rail_one();
      translate([slot_cx - pn_board.x / 2, pn_y0, pn_z0]) mirror([1, 0, 0]) pn_rail_one();
      // screw bosses
      for (p = boss_positions) translate([p.x, p.y, 0]) cylinder(d = boss_d, h = box.z - top_t);
    }
    translate([slot_x0, slot_y, slot_z0]) cube([slot_len, slot_w, slot_depth + 1]);
    slot_funnel();
    thumb_notch();
    cassette_door();
    for (dx = [-reel_dx, reel_dx]) reel(slot_cx + dx);
    speaker_grille();
    for (p = boss_positions) translate([p.x, p.y, -0.01]) cylinder(d = pilot_d, h = pilot_depth);
    // Knob: encoder bushing through the front wall
    translate([knob_x, -0.01, knob_z]) rotate([-90, 0, 0]) cylinder(d = enc_bushing_d + 0.4, h = wall + 0.02);
    // LED ring: a pocket from inside for the LEDs, leaving a thin window in front of them.
    // The ring PCB then lies flat against the inside of the wall.
    translate([knob_x, ring_window_t, knob_z]) rotate([-90, 0, 0])
      difference() {
        cylinder(r = ring_led_r + ring_led / 2 + 0.5, h = wall);
        translate([0, 0, -0.01]) cylinder(r = ring_led_r - ring_led / 2 - 0.5, h = wall + 0.02);
      }
    // USB cable hole in the left wall, in line with the ESP32's connector
    usb_z = esp_standoff + 1.6 + 1.5;
    translate([-0.01, box.y / 2, usb_z]) rotate([0, 90, 0])
      hull() for (dy = [-1, 1]) translate([0, dy * (usb_hole.x - usb_hole.y) / 2, 0])
        cylinder(d = usb_hole.y, h = wall + 0.02);
  }
}

module esp_cradle() {
  guide_h = esp_standoff + 3;
  // supports under both short ends; the shell wall is the stop at the USB end
  for (x = [esp_x0, esp_x0 + esp.x - esp_block])
    translate([x, esp_y0, 0]) cube([esp_block, esp.y, esp_standoff]);
  // end stop at the antenna end
  translate([esp_x0 + esp.x + 0.3, esp_y0, 0]) cube([1.2, esp.y, guide_h]);
  // side guides
  for (x = [esp_x0, esp_x0 + esp.x - esp_block], y = [esp_y0 - 1.5, esp_y0 + esp.y + 0.3])
    translate([x, y, 0]) cube([esp_block, 1.2, guide_h]);
}

module base() {
  difference() {
    union() {
      rounded_box([box.x, box.y, base_t], corner_r);
      translate([0, 0, base_t]) esp_cradle();
    }
    for (p = boss_positions) translate([p.x, p.y, -0.01]) {
      cylinder(d = screw_d, h = base_t + 0.02);
      cylinder(d = screw_head_d, h = screw_head_depth);
    }
  }
}

// Everything that goes inside or onto the shell, in shell coordinates.
module components() {
  color("Crimson") translate([slot_cx - pn_board.x / 2, pn_y0, pn_z0]) cube(pn_board);
  color("DarkGreen") translate([esp_x0, esp_y0, esp_standoff]) cube([esp.x, esp.y, 1.6]);
  // LED ring: PCB against the inside of the front wall, LEDs in the pocket
  translate([knob_x, wall, knob_z]) rotate([-90, 0, 0]) {
    color("Black") difference() { cylinder(d = ring_od, h = 1.6); translate([0, 0, -1]) cylinder(d = ring_id, h = 4); }
    color("White") for (i = [0 : 11]) rotate(i * 30) translate([ring_led_r, 0, -ring_led_h / 2]) cube([5, 5, ring_led_h], center = true);
  }
  // encoder body behind the wall, knob in front
  color("Silver") translate([knob_x - 6, wall, knob_z - 6]) cube([12, 6.5, 12]);
  color("DimGray") translate([knob_x, -0.5, knob_z]) rotate([90, 0, 0]) knob();
  // disc, picture side facing out through the cassette door
  color("Gold") translate([slot_cx, slot_y + slot_w / 2 - disc_t / 2, slot_z0 + disc_d / 2]) rotate([-90, 0, 0]) disc();
}

if (part == "shell") {
  translate([0, 0, box.z]) mirror([0, 0, 1]) shell();  // print orientation
} else if (part == "base") {
  base();
} else if (part == "cutaway") {
  // Back half of the shell removed to show the reader, ESP32, encoder and LED ring.
  color("Gainsboro") base();
  translate([0, 0, base_t]) {
    color("WhiteSmoke") difference() {
      shell();
      translate([-1, pn_y0 + pn_board.y + pn_rail + 4, -1]) cube([box.x + 2, box.y, box.z + 2]);
    }
    components();
  }
} else {
  color("Gainsboro") base();
  translate([0, 0, base_t]) {
    color("WhiteSmoke") shell();
    components();
  }
}
