#pragma once

// LED ring around the volume knob. Animated by a background task, so it keeps
// moving while the main loop waits on Sonos. Dark unless there is something
// to show: the player lives in a bedroom.

// What the ring shows when nothing more urgent is going on.
enum class RingState {
  Idle,      // off
  Reading,   // spinner: disc seen, waiting for the music to start
  Setup,     // slow blue breathing: join "DiscPlayer-Setup" on a phone
  Updating,  // orange spinner: installing a firmware update, don't unplug
};

void ringBegin();
void ringSetState(RingState state);

// Short overlays, shown on top of the state and then gone.
void ringFlashOk();                            // music started
void ringFlashError();                         // blank disc, speaker unreachable, ...
void ringShowVolume(int volume, int maxVolume, bool atCap);  // gauge, ~2.5 s
void ringFlashMaxSaved(int maxVolume);         // two white flashes, then full gauge
