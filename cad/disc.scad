// Music disc. Print flat, no supports.
//
// embed_tag = false: the tag pocket is open on the back; stick the tag in and
//                    cover it with the back-side label or a dab of glue.
// embed_tag = true:  the pocket is sealed inside the disc. Pause the print at
//                    the height printed in the console, drop the tag in, resume.
//                    Toddler-proof: the tag cannot be peeled off.

include <params.scad>

embed_tag = true;
floor_t = 1.2;  // plastic under an embedded tag

module chamfered_cylinder(d, h, c) {
  hull() {
    translate([0, 0, c]) cylinder(d = d, h = h - 2 * c);
    cylinder(d = d - 2 * c, h = h);
  }
}

module disc() {
  pocket_z = embed_tag ? floor_t : -0.01;
  difference() {
    chamfered_cylinder(disc_d, disc_t, disc_chamfer);
    translate([0, 0, pocket_z]) cylinder(d = tag_d, h = tag_pocket + 0.01);
    translate([0, 0, disc_t - label_depth]) cylinder(d = label_d, h = label_depth + 0.01);
  }
}

assert(!embed_tag || floor_t + tag_pocket + label_depth + 0.8 <= disc_t,
       "Disc too thin to embed the tag");
if (embed_tag) echo(str("Pause print at Z = ", floor_t + tag_pocket, " mm to insert the tag"));

disc();
