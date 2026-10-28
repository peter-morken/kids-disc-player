// Shared dimensions (mm). Disc and enclosure both read these, so changing the
// disc size here resizes the slot too.

$fn = 96;

// --- Disc ---
disc_d = 54;          // diameter
disc_t = 4;           // thickness
disc_chamfer = 0.8;   // edge chamfer, kinder to small hands

// 25 mm round NTAG213/215 sticker. Measure yours.
tag_d = 25.5;
tag_pocket = 0.8;     // pocket depth (sticker + adhesive is ~0.3-0.5)

// Recess on the front for a printed picture sticker.
label_d = 46;
label_depth = 0.4;

// --- Printer ---
fit = 0.6;            // clearance for parts that slide together
