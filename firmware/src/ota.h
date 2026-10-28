#pragma once

// Checks the repo's latest GitHub release and, if its version is newer than
// this build, downloads firmware.bin, flashes it and restarts. Does nothing
// for "dev" builds. Any failure is logged and the current firmware keeps
// running; the next boot tries again.
void checkForUpdate();
