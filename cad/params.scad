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

// --- Volume knob: EC11 rotary encoder, M7 bushing, 6 mm D-shaft ---
enc_bushing_d = 7;
enc_shaft_d = 6;
enc_shaft_flat = 4.5;   // thickness across the flat of the D
enc_nut_d = 11;         // across corners of the M7 nut, for the knob's underside recess
knob_d = 32;            // big enough for a toddler's whole hand
knob_h = 16;

// --- LED ring around the knob: 12 x WS2812 (5050), 50 mm OD / 36 mm ID. Measure yours. ---
ring_od = 50;
ring_id = 36;
ring_led = 5.2;         // LED package size, plus a little
ring_led_h = 1.6;       // LED height above the ring PCB
ring_window_t = 0.8;    // plastic the light shines through: two 0.4 mm lines
