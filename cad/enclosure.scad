// Player enclosure: a shell with an upright disc slot, and a base plate that
// carries the ESP32. The PN532 slides into rails right behind the slot. The
// front face carries the volume knob, with the LED ring glowing through a thin
// window around it: print the shell in white or another light filament.
//
// Print the shell upside down (top face on the bed): no supports needed.
// Print the base flat.
// Export each part with:  openscad -D 'part="shell"' -o shell.stl enclosure.scad

include <params.scad>

part = "assembly";  // [shell, base, assembly]

// --- Box ---
box = [85, 60, 70];
corner_r = 8;
wall = 2.4;
top_t = 2.4;

// --- Disc slot (disc stands upright, slot runs along X) ---
slot_w = disc_t + 1.2;
slot_len = disc_d + 1.5;
slot_depth = 40;          // disc sticks out disc_d - slot_depth = 14 mm to grab
slot_y = 20;              // front of slot, measured from the front face
slot_wall = 2;            // front and end walls of the slot pocket
slot_wall_reader = 1.2;   // wall between disc and PN532 antenna: keep thin
slot_floor = 2;

// --- PN532 V3 module (red/blue "Elechouse" style). Measure yours. ---
pn_board = [42.7, 1.6, 40.4];  // width (x), thickness (y), height (z)
pn_lip = 1.5;                  // how far the rail lips cover the board edge
pn_rail = 2;

// --- ESP32 DevKit (38-pin DevKitC: 52 x 28.5; 30-pin DevKit V1: 51.5 x 28.5) ---
esp = [52, 28.5];
esp_standoff = 16;        // clears header pins pointing down
esp_block = 4;            // support blocks under the pin-free short ends
usb_hole = [13, 9];       // width (y), height (z)

// --- Volume knob and LED ring on the front face ---
knob_z = 36;              // height of the knob's centre above the shell's bottom edge

// --- Base and screws (M3 x 10 self-tapping, or M3 into heat-set inserts) ---
base_t = 3;
boss_d = 8;
pilot_d = 2.5;            // 4.0 for M3 heat-set inserts
pilot_depth = 12;
screw_d = 3.4;
screw_head_d = 6.2;
screw_head_depth = 1.8;

// --- Derived ---
slot_x0 = (box.x - slot_len) / 2;
slot_y1 = slot_y + slot_w;
slot_z0 = box.z - slot_depth;
pn_y0 = slot_y1 + slot_wall_reader;  // board front face rests here
pn_z0 = box.z - top_t - pn_board.z;
esp_x0 = wall + 0.5;
esp_y0 = (box.y - esp.y) / 2;
boss_inset = wall + boss_d / 2 - 1;
boss_positions = [for (x = [boss_inset, box.x - boss_inset], y = [boss_inset, box.y - boss_inset]) [x, y]];

ring_led_r = (ring_od + ring_id) / 4;  // radius of the LED centres
// A slightly taller LED just leaves the ring PCB a fraction proud of the wall, under glue.
assert(wall - ring_window_t >= ring_led_h - 0.01, "Front wall too thin to hide the LEDs in a pocket");
assert(knob_z + ring_od / 2 < box.z - top_t, "LED ring hits the top");
assert(box.x / 2 - ring_od / 2 > boss_inset + boss_d / 2, "LED ring hits a screw boss");
assert(slot_y - slot_wall > wall + 12, "No room behind the front wall for the encoder body");
assert(slot_depth + slot_floor + esp_standoff + 8 < box.z, "ESP32 collides with slot floor");
assert(pn_z0 > esp_standoff + 6, "PN532 collides with ESP32");
echo(str("Tag centre is ", disc_d / 2 - (box.z - slot_z0), " mm from the top face; ",
         "PN532 board centre is ", -(top_t + pn_board.z / 2), " mm"));

module rounded_box(size, r) {
  hull() for (x = [r, size.x - r], y = [r, size.y - r])
    translate([x, y, 0]) cylinder(r = r, h = size.z);
}

// One rail, for the board edge at x = 0 with the board extending to -x.
module pn_rail_one() {
  t = pn_board.y + fit / 2;
  difference() {
    translate([-pn_lip, 0, 0]) cube([pn_lip + fit / 2 + pn_rail, t + pn_rail, pn_board.z]);
    translate([-pn_lip - 0.01, -0.01, -0.01]) cube([pn_lip + fit / 2 + 0.01, t + 0.01, pn_board.z + 0.02]);
  }
}

module shell() {
  difference() {
    union() {
      difference() {
        rounded_box(box, corner_r);
        translate([wall, wall, -0.01])
          rounded_box([box.x - 2 * wall, box.y - 2 * wall, box.z - top_t + 0.01], corner_r - wall);
      }
      // slot pocket
      translate([slot_x0 - slot_wall, slot_y - slot_wall, slot_z0 - slot_floor])
        cube([slot_len + 2 * slot_wall, slot_w + slot_wall + slot_wall_reader, slot_depth + slot_floor]);
      // PN532 rails: slide the board up from below until it hits the top
      translate([box.x / 2 + pn_board.x / 2, pn_y0, pn_z0]) pn_rail_one();
      translate([box.x / 2 - pn_board.x / 2, pn_y0, pn_z0]) mirror([1, 0, 0]) pn_rail_one();
      // screw bosses
      for (p = boss_positions) translate([p.x, p.y, 0]) cylinder(d = boss_d, h = box.z - top_t);
    }
    translate([slot_x0, slot_y, slot_z0]) cube([slot_len, slot_w, slot_depth + 1]);
    for (p = boss_positions) translate([p.x, p.y, -0.01]) cylinder(d = pilot_d, h = pilot_depth);
    // Knob: encoder bushing through the front wall
    translate([box.x / 2, -0.01, knob_z]) rotate([-90, 0, 0]) cylinder(d = enc_bushing_d + 0.4, h = wall + 0.02);
    // LED ring: a pocket from inside for the LEDs, leaving a thin window in front of them.
    // The ring PCB then lies flat against the inside of the wall.
    translate([box.x / 2, ring_window_t, knob_z]) rotate([-90, 0, 0])
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

if (part == "shell") {
  translate([0, 0, box.z]) mirror([0, 0, 1]) shell();  // print orientation
} else if (part == "base") {
  base();
} else {
  base();
  translate([0, 0, base_t]) {
    color("SteelBlue", 0.6) shell();
    color("Crimson") translate([box.x / 2 - pn_board.x / 2, pn_y0, pn_z0]) cube(pn_board);
    color("DarkGreen") translate([esp_x0, esp_y0, esp_standoff]) cube([esp.x, esp.y, 1.6]);
    color("Black") translate([box.x / 2, wall, knob_z]) rotate([-90, 0, 0])
      difference() { cylinder(d = ring_od, h = 1.6); translate([0, 0, -1]) cylinder(d = ring_id, h = 4); }
    color("DimGray") translate([box.x / 2, -2, knob_z]) rotate([90, 0, 0]) cylinder(d = knob_d, h = knob_h);
    color("Gold") translate([box.x / 2, slot_y + slot_w / 2 - disc_t / 2, slot_z0 + disc_d / 2])
      rotate([-90, 0, 0]) cylinder(d = disc_d, h = disc_t);
  }
}
