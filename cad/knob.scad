// Volume knob for the EC11 encoder. Print top face down, no supports.
// Ridged and wide so a toddler can turn it with a whole hand.

include <params.scad>

ridges = 18;
ridge_d = 2.4;
shaft_depth = 10;   // EC11 with a 20 mm shaft leaves ~12 mm past panel and nut
nut_recess_h = 3;   // clears the M7 nut on the front of the panel
top_chamfer = 1.5;
shaft_fit = 0.15;   // tight: the knob is held by friction on the D-shaft

module knob() {
  body_h = knob_h - top_chamfer;
  difference() {
    union() {
      cylinder(d = knob_d, h = body_h);
      translate([0, 0, body_h]) cylinder(d1 = knob_d, d2 = knob_d - 2 * top_chamfer, h = top_chamfer);
      for (i = [0 : ridges - 1])
        rotate(i * 360 / ridges) translate([knob_d / 2, 0, 0]) cylinder(d = ridge_d, h = body_h, $fn = 24);
    }
    // nut recess, then the D-shaft bore above it
    translate([0, 0, -0.01]) cylinder(d = enc_nut_d + 1, h = nut_recess_h);
    translate([0, 0, nut_recess_h - 0.01])
      intersection() {
        cylinder(d = enc_shaft_d + 2 * shaft_fit, h = shaft_depth);
        // the flat sits enc_shaft_flat - enc_shaft_d / 2 from the centre
        translate([-enc_shaft_d, -enc_shaft_d, 0])
          cube([2 * enc_shaft_d, enc_shaft_d + enc_shaft_flat - enc_shaft_d / 2 + shaft_fit, shaft_depth]);
      }
  }
}

assert(nut_recess_h + shaft_depth < knob_h - 1, "Shaft bore breaks through the top");

// Exported upside down: the top face prints on the bed.
translate([0, 0, knob_h]) mirror([0, 0, 1]) knob();
