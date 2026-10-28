#pragma once

// Volume knob (rotary encoder + push switch), sampled every millisecond in a
// background task so turns aren't lost while the main loop waits on Sonos.
void inputBegin();

// Detents turned since the last call; positive = louder.
int takeKnobDetents();

// True once per hold that reached KNOB_LONG_PRESS_MS.
bool takeLongPress();
